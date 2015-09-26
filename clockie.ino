#define F_CPU 32768 // 32.768 KHz clock

/* long unsigned int millis() { return 0; } */

#include <LiquidCrystal.h>
#include <Time.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define DISPLAY_BUTTON_PIN 2
#define LCD_LIGHT_PIN      6

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

unsigned int showCounter = 0;
unsigned int lastSecond  = 60;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(0, 1, 2, 3, 4, 7);

void renderTime() {
  unsigned int hr  = hour();
  unsigned int min = minute();
  unsigned int sec = second();
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

  turnOnDisplay();

  sei(); // Turn on interrupts

  // Set prescaler to 128 to give exactly 1 second before an overflow occurs.
  // 128 prescaler x 256 timer bits / 32768 clock = 1 second
  sbi(CLKPR, CLKPCE);
  CLKPR = (0 << CLKPCE) | _BV(CLKPS2) | _BV(CLKPS1) | _BV(CLKPS0);

  // Enable timer 1 overflow interrupt
  sbi(TIMSK0, TOIE0);
}

// Timer 1 interrupt
ISR(TIM0_OVF_vect) {
  adjustTime(1);
  renderTime();
}
