#define F_CPU 32768 // 32.768 KHz clock

#include <Time.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <LiquidCrystal_SR.h>

#define DISPLAY_BUTTON_PIN 6
#define SR_CLEAR_PIN       5
#define LCD_LIGHT_PIN      2
#define WIFI_ENABLE_PIN    3

#define CLOCK_PIN 8
#define DATA_PIN  7

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

unsigned int showCounter = 0;

volatile unsigned int lastClockPinCount = 0;
volatile unsigned int clockPinCount = 0;
volatile time_t lastTogglePin = 0;
volatile time_t lastSetTime = 0;
volatile time_t time = 0;
volatile bool displayOn = false;

// initialize the library with the numbers of the interface pins
LiquidCrystal_SR lcd(0,1,TWO_WIRE);
//                   | |
//                   | \-- Clock Pin
//                   \---- Data/Enable Pin

void renderTime() {
  unsigned int hr  = hour(time);
  unsigned int min = minute(time);
  unsigned int sec = second(time);
  lcd.setCursor(0, 1);
  if (hr < 10) {
    lcd.print(0);
  }
  lcd.print(hr);
  lcd.print(':');
  if (min < 10) {
    lcd.print(0);
  }
  lcd.print(min);
  lcd.print(':');
  if (sec < 10) {
    lcd.print(0);
  }
  lcd.print(sec);
}

void turnOnDisplay() {
  digitalWrite(LCD_LIGHT_PIN, LOW);
  // clear the shift register
  digitalWrite(SR_CLEAR_PIN, LOW);
  digitalWrite(SR_CLEAR_PIN, HIGH);
  renderTime();
  lcd.display();
  displayOn = true;
}

void turnOffDisplay() {
  lcd.noDisplay();
  digitalWrite(LCD_LIGHT_PIN, HIGH);
  displayOn = false;
}

void showTime() {
  turnOnDisplay();
  showCounter = 5;
}

bool isDST() {
  int mo = month(time);
  //January, february, and december are out.
  if (mo < 3 || mo > 11) { return false; }
  //April to October are in
  if (mo > 3 && mo < 11) { return true; }
  int prevSun = day(previousSunday(time));
  //In march, we are DST if our previous sunday was on or after the 8th.
  if (mo == 3) { return prevSun >= 8; }
  //In november we must be before the first sunday to be dst.
  //That means the previous sunday must be before the 1st.
  return prevSun <= 0;
}

void loop() {
  set_sleep_mode(SLEEP_MODE_IDLE); // Set sleep mode as idle
  sleep_mode(); // System sleeps here
  if (lastClockPinCount != clockPinCount) {
    lastClockPinCount = clockPinCount;
  } else if (displayOn) {
    renderTime();
  }
  // clear the clock pin counter
  if (lastTogglePin > 0 && lastTogglePin < time-2) {
    lastTogglePin = 0;
    lcd.setCursor(0, 0);
    if (clockPinCount == 0) {
      lcd.print("S");
      lcd.print(lastSetTime);
    } else {
      lcd.print(clockPinCount);
    }
    lcd.print("          ");
    clockPinCount = 0;
  }
  if (showCounter > 0) {
    showCounter--;
    if (showCounter == 0) {
      turnOffDisplay();
    }
  }
}

void setup() {
  // Set the LCD display backlight, WiFi Enable
  // and Shift Register pins as outputs.
  pinMode(LCD_LIGHT_PIN,   OUTPUT);
  pinMode(WIFI_ENABLE_PIN, OUTPUT);
  pinMode(SR_CLEAR_PIN,    OUTPUT);

  // Turn on the LCD backlight.
  // (PNP Transistor)
  digitalWrite(LCD_LIGHT_PIN, LOW);

  // Turn off WiFi.
  digitalWrite(WIFI_ENABLE_PIN, LOW);

  // Turn off Shift Register clear
  // (Active LOW)
  digitalWrite(SR_CLEAR_PIN, HIGH);

  // turn the button into a pullup,
  // will trigger when it goes low
  pinMode(DISPLAY_BUTTON_PIN, INPUT_PULLUP);

  // turn 6 & 8 into inputs for the esp8266
  pinMode(CLOCK_PIN, INPUT_PULLUP);
  pinMode(DATA_PIN,  INPUT_PULLUP);

  // listen to pin changes on pin 6 (PCINT6)
  sbi(PCMSK0, PCINT6);
  // listen to pin changes on pin 8 (PCINT10)
  sbi(PCMSK1, PCINT10);

  // Enable PCINT interrupts 0-7
  sbi(GIMSK, PCIE0);
  // Enable PCINT interrupts 8-11
  sbi(GIMSK, PCIE1);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.noDisplay();

  turnOnDisplay();

  sei(); // Turn on interrupts

  // set the 1x timer prescaler
  cbi(TCCR1B, CS12);
  cbi(TCCR1B, CS11);
  sbi(TCCR1B, CS10);

  // Turn off PWM mode for Timer1
  cbi(TCCR1B, WGM13);
  sbi(TCCR1B, WGM12);
  cbi(TCCR1A, WGM11);
  cbi(TCCR1A, WGM10);

  // Trigger when Timer 1 hits 32768 (2^15), our clock speed.
  OCR1AH = 0x80;
  OCR1AL = 0x00;

  // Enable Timer 1 compare interrupt
  sbi(TIMSK1, OCIE1A);

  // Now that we're starting up, turn on Wifi
  digitalWrite(WIFI_ENABLE_PIN, HIGH);
}

// Timer 1 interrupt
ISR(TIM1_COMPA_vect) {
  time++;
}

// buttons
ISR(PCINT0_vect) {
  if (digitalRead(DISPLAY_BUTTON_PIN) == LOW) {
    showTime();
  }
}

// clock pin
ISR(PCINT1_vect) {
  // only read when the clock is high
  if (digitalRead(CLOCK_PIN) == HIGH) {
    lastTogglePin = time;
    if (clockPinCount == 0) {
      lastSetTime = 0;
    }
    lastSetTime |= (unsigned long)(digitalRead(DATA_PIN)) << clockPinCount;
    clockPinCount++;
    if (clockPinCount == 32) {
      time = lastSetTime;
      clockPinCount = 0;
      // we have our time, turn off wifi
      digitalWrite(WIFI_ENABLE_PIN, LOW);
    }
  }
}
