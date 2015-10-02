#define F_CPU 32768 // 32.768 KHz clock

#include <LiquidCrystal.h>
#include <Time.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define DISPLAY_BUTTON_PIN 2
#define LCD_LIGHT_PIN      6

#define CLOCK_PIN 8
#define DATA_PIN  6

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

unsigned int showCounter = 0;

volatile time_t lastSetTime = 0;
volatile time_t time = 0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(0, 1, 2, 3, 4, 7);

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
  renderTime();
  lcd.display();
  /* digitalWrite(LCD_LIGHT_PIN, HIGH); */
}

void turnOffDisplay() {
  /* digitalWrite(LCD_LIGHT_PIN, LOW); */
  lcd.noDisplay();
}

void showTime() {
  turnOnDisplay();
  showCounter = 5;
}

void loop() {
  set_sleep_mode(SLEEP_MODE_IDLE); // Set sleep mode as idle
  sleep_mode(); // System sleeps here
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  lcd.noDisplay();

  /* pinMode(DISPLAY_BUTTON_PIN, INPUT_PULLUP); */
  /* attachInterrupt(0, showTime, FALLING); */

  // Set the LCD display backlight pin as an output.
  /* pinMode(LCD_LIGHT_PIN, OUTPUT); */

  // Turn off the LCD backlight.
  /* digitalWrite(LCD_LIGHT_PIN, LOW); */

  // turn 6 & 8 into inputs for the esp8266
  pinMode(CLOCK_PIN, INPUT);
  pinMode(DATA_PIN,  INPUT);

  // listen to pin changes on pin 8
  sbi(PCMSK1, PCINT8);

  // Enable PCINT interrupts 8-11
  sbi(GIMSK, PCIE1);

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
}

// Timer 1 interrupt
ISR(TIM1_COMPA_vect) {
  time++;
  renderTime();
}

unsigned int clockCount = 0;
// clock pin
ISR(PCINT1_vect) {
  // only read when the clock is high
  if (digitalRead(CLOCK_PIN) == HIGH) {
    clockCount++;
    lastSetTime = (lastSetTime >> 1) | (unsigned long)(digitalRead(DATA_PIN)) << 31;
    if (clockCount == 32) {
      setTime(lastSetTime);
    }
    lcd.setCursor(0, 0);
    lcd.print(lastSetTime);
  }
}
