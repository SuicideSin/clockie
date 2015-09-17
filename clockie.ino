#include <LiquidCrystal.h>
#include <Time.h>

#define DISPLAY_BUTTON_PIN 2
#define LCD_LIGHT_PIN      6

unsigned int showCounter = 0;
unsigned int lastSecond  = 60;

// initialize the library with the numbers of the interface pins
//                A0  A1  A2  A3  A4  A5
LiquidCrystal lcd(14, 15, 16, 17, 18, 19);

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
  digitalWrite(LCD_LIGHT_PIN, HIGH);
}

void turnOffDisplay() {
  digitalWrite(LCD_LIGHT_PIN, LOW);
  lcd.noDisplay();
}

void showTime() {
  turnOnDisplay();
  showCounter = 5;
}

void loop() {
  /* if (showCounter > 0) { */
    unsigned int sec = second();
    if (sec != lastSecond) {
      lastSecond = sec;
      showCounter--;
      renderTime();
      /* if (showCounter < 1) { */
      /*   turnOffDisplay(); */
      /* } */
    }
  /* } */
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  lcd.noDisplay();

  /* pinMode(DISPLAY_BUTTON_PIN, INPUT_PULLUP); */
  /* attachInterrupt(0, showTime, FALLING); */

  // Set the LCD display backlight pin as an output.
  pinMode(LCD_LIGHT_PIN, OUTPUT);

  // Turn off the LCD backlight.
  digitalWrite(LCD_LIGHT_PIN, LOW);

  turnOnDisplay();
}
