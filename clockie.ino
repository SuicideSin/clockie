#define F_CPU 32768 // 32.768 KHz clock

#include <Time.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <LiquidCrystal_SR.h>

#define BUTTON_PIN_1    4
#define BUTTON_PIN_2    6
#define SR_CLEAR_PIN    5
#define LCD_LIGHT_PIN   2
#define WIFI_ENABLE_PIN 3
#define ESP_CLOCK_PIN   8
#define ESP_DATA_PIN    7

#define SHOW_LCD_TIMEOUT 10

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

typedef void (*stateAction)();
stateAction currentStateAction;

void receiveDataState();
void displayOffState();
void timeDisplayState();
void menuDisplayState();

typedef enum {
  QUIET_TIME,
  LOCK,
  RE_QUERY_TIME,
  TZ_OFFSET,
  BEDTIME,
  WAKEUP_TIME,
  DISPLAY_TIMEOUT
} menuSelectionType;

String menuStrings[7] {
  "Quiet Time",
  "Lock",
  "Re-query Time",
  "TZ Offset",
  "Bedtime",
  "Wakeup Time",
  "Display Timeout"
};

// is the display on or not
volatile bool displayOn = false;
// force update of the display
volatile bool forceDisplayUpdate = true;
// is the menu active or not
volatile bool menuActive = false;
// has the timer interrupt incremented seconds
volatile bool timeUpdated = false;
// current menu selection
volatile menuSelectionType menuSelection = QUIET_TIME;
// how long before the screen turns off
volatile unsigned int showCounter = 0;
// number of clock pulses from the ESP8266
// when this reaches 32 we have our timestamp
volatile unsigned int clockPinCount = 0;
// last time we saw the clock pin toggle from the ESP8266
volatile time_t lastTogglePin = 0;
// timezone offset in seconds (defaults to PST)
volatile int timezoneOffset = -3600 * 8;
// the last time we got from the ESP8266/NTP in GMT
volatile time_t lastSetTime = 0;
// current time
volatile time_t time = 0;

// initialize the library with the numbers of the interface pins
LiquidCrystal_SR lcd(0,1,TWO_WIRE);
//                   | |
//                   | \-- Clock Pin
//                   \---- Data/Enable Pin

byte eyeOpen[8] = {
  B00000,
  B00000,
  B01110,
  B10001,
  B10101,
  B10001,
  B01110,
  B00000
};

byte eyeClose1[8] = {
  B00000,
  B00000,
  B01110,
  B11111,
  B10101,
  B10001,
  B01110,
  B00000
};

byte eyeClose2[8] = {
  B00000,
  B00000,
  B01110,
  B11111,
  B11111,
  B10001,
  B01110,
  B00000
};

byte eyeClose3[8] = {
  B00000,
  B00000,
  B01110,
  B11111,
  B11111,
  B11111,
  B01110,
  B00000
};

byte smileLeft[8] = {
  B00000,
  B00000,
  B01000,
  B01000,
  B00100,
  B00011,
  B00000,
  B00000
};

byte smileRight[8] = {
  B00000,
  B00000,
  B00010,
  B00010,
  B00100,
  B11000,
  B00000,
  B00000
};

byte sleepMouthLeft[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B00000,
  B00000
};

byte sleepMouthRight[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B10100,
  B11100
};

byte smallZ[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B01111,
  B00010,
  B00100,
  B01111
};

byte medZ[8] = {
  B00000,
  B00000,
  B11111,
  B00010,
  B00100,
  B01000,
  B11111,
  B00000
};

void renderTime(bool force) {
  unsigned int hr  = hourFormat12(time);
  unsigned int min = minute(time);
  unsigned int sec = second(time);
  unsigned int off = 0;
  if (force || (sec == 0 && min == 0)) { // update hour
    lcd.setCursor(0, 1);
    lcd.print(hr);
    lcd.print(':');

    off = hr < 10 ? 7 : 8;
    lcd.setCursor(off, 1);
    if (isAM(time)) {
      lcd.print("AM");
    } else {
      lcd.print("PM");
    }
    if (hr < 10) {
      lcd.print(' ');
    }
  }
  if (force || (sec == 0)) {
    off = hr < 10 ? 2 : 3;
    lcd.setCursor(off, 1);
    if (min < 10) {
      lcd.print(0);
    }
    lcd.print(min);
    lcd.print(':');
  }

  off = hr < 10 ? 5 : 6;
  lcd.setCursor(off, 1);
  if (sec < 10) {
    lcd.print(0);
  }
  lcd.print(sec);
}

void clearTime() {
  // will need to force update next time
  forceDisplayUpdate = true;
  lcd.setCursor(0, 1);
  lcd.print("          ");
}

void turnOnDisplay() {
  digitalWrite(LCD_LIGHT_PIN, LOW);
  // clear the shift register
  digitalWrite(SR_CLEAR_PIN, LOW);
  digitalWrite(SR_CLEAR_PIN, HIGH);
  lcd.display();
  displayOn = true;
}

void turnOffDisplay() {
  lcd.noDisplay();
  digitalWrite(LCD_LIGHT_PIN, HIGH);
  displayOn = false;
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

void createChars() {
  lcd.createChar(0, eyeOpen);
  lcd.createChar(1, eyeClose1);
  lcd.createChar(2, eyeClose2);
  lcd.createChar(3, eyeClose3);
  lcd.createChar(4, smileLeft);
  lcd.createChar(5, smileRight);
}

void receiveDataState() {
  // clear the clock pin counter after 2 seconds
  if (lastTogglePin > 0 && lastTogglePin < time-2) {
    lastTogglePin = 0;
    clockPinCount = 0;
  }
  if (lastTogglePin == 0) {
    // show what we got
    turnOnDisplay();
    forceDisplayUpdate = true;
    currentStateAction = &timeDisplayState;
  }
}

void displayOffState() {
  set_sleep_mode(SLEEP_MODE_IDLE); // Set sleep mode as idle
  sleep_mode(); // System sleeps here

  if (showCounter > 0) {
    turnOnDisplay();
    forceDisplayUpdate = true;
    currentStateAction = &timeDisplayState;
  }
}

void timeDisplayState() {
  if (forceDisplayUpdate) {
    forceDisplayUpdate = false;
    renderTime(true);
  }
  if (timeUpdated) {
    timeUpdated = false;
    renderTime(false);

    showCounter--;
    if (showCounter == 0) {
      clearTime();
      turnOffDisplay();
      currentStateAction = &displayOffState;
    }
  }
}

void menuDisplayState() {
  // TODO
}

void loop() {
  // run the current state
  (*currentStateAction)();
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

  // turn the buttons into pullups,
  // will trigger when they go low
  pinMode(BUTTON_PIN_1, INPUT_PULLUP);
  pinMode(BUTTON_PIN_2, INPUT_PULLUP);

  // turn 6 & 8 into inputs for the esp8266
  pinMode(ESP_CLOCK_PIN, INPUT_PULLUP);
  pinMode(ESP_DATA_PIN,  INPUT_PULLUP);

  // listen to pin changes on pin 4 and 6 (PCINT6)
  sbi(PCMSK0, PCINT4);
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

  createChars();

  // Draw the smiley
  lcd.setCursor(14, 0);
  lcd.write((uint8_t)0);
  lcd.write((uint8_t)0);
  lcd.setCursor(14, 1);
  lcd.write(4);
  lcd.write(5);

  // Now that we're starting up, turn on Wifi
  digitalWrite(WIFI_ENABLE_PIN, HIGH);

  // Start with the display on
  currentStateAction = &timeDisplayState;
}

// Timer 1 interrupt
ISR(TIM1_COMPA_vect) {
  time++;
  timeUpdated = true;
}

// buttons
ISR(PCINT0_vect) {
  bool butt1 = digitalRead(BUTTON_PIN_1);
  bool butt2 = digitalRead(BUTTON_PIN_2);
  if ((butt1 == LOW) || (butt2 == LOW)) {
    showCounter = SHOW_LCD_TIMEOUT;
  }
  if ((butt1 == LOW) && (butt2 == LOW)) {
    menuActive = true;
  }
}

// clock pin
ISR(PCINT1_vect) {
  // only read when the clock is high
  if (digitalRead(ESP_CLOCK_PIN) == HIGH) {
    lastTogglePin = time;
    if (clockPinCount == 0) {
      lastSetTime = 0;
      currentStateAction = &receiveDataState;
    }
    lastSetTime |= (unsigned long)(digitalRead(ESP_DATA_PIN)) << clockPinCount;
    clockPinCount++;
    if (clockPinCount == 32) {
      time = lastSetTime + timezoneOffset;
      if (isDST()) {
        time += 3600;
      }
      // we have our time, turn off wifi
      digitalWrite(WIFI_ENABLE_PIN, LOW);
      clockPinCount = 0;
    }
  }
}
