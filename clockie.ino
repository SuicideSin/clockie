#include <LiquidCrystal.h>
#include <Time.h>

unsigned int showCounter = 0;
unsigned int lastSecond  = 60;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

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

void showTime() {
  lcd.display();
  renderTime();
  showCounter = 5;
}

void loop() {
  if (showCounter > 0) {
    unsigned int sec = second();
    if (sec != lastSecond) {
      lastSecond = sec;
      showCounter--;
      renderTime();
      if (showCounter < 1) {
        lcd.noDisplay();
      }
    }
  }
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");

  lcd.noDisplay();

  pinMode(2, INPUT_PULLUP);
  attachInterrupt(0, showTime, FALLING);
}
