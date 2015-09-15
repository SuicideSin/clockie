#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include <LiquidCrystal.h>
#include <Time.h>

#define DISPLAY_BUTTON_PIN 2
#define LCD_LIGHT_PIN      6

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data

#define DEVICE_NAME "CC3000"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   2  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  9
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV4); // you can change this clock speed

unsigned int showCounter = 0;
unsigned int lastSecond  = 60;

uint32_t ip, gateway;

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

void initCC3000() {
  lcd.clear();
  lcd.print("Connecting...");
  if (!cc3000.begin(false, true, DEVICE_NAME)) {
    lcd.setCursor(0, 1);
    lcd.print("Couldn't begin()");
    while(1);
  }

  lcd.clear();
  lcd.print("Connected!");

  /* Wait for DHCP to complete */
  lcd.setCursor(0, 1);
  lcd.print("Requesting DHCP");
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  // don't care about these
  uint32_t netmask, dhcpserv, dnsserv;
  cc3000.getIPAddress(&ip, &netmask, &gateway, &dhcpserv, &dnsserv);

  lcd.clear();
  lcd.print((uint8_t)(ip >> 24));
  lcd.print('.');
  lcd.print((uint8_t)(ip >> 16));
  lcd.print('.');
  lcd.print((uint8_t)(ip >> 8));
  lcd.print('.');
  lcd.print((uint8_t)(ip));
}

void queryTime() {
  String line = "";

  lcd.clear();
  lcd.print("Querying..");

  Adafruit_CC3000_Client www = cc3000.connectTCP(gateway, 80);
  if (www.connected()) {
    www.fastrprint(F("GET / HTTP/1.1\r\n"));
    www.fastrprint(F("HOST: ")); www.fastrprint("192.168.0.1"); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    lcd.clear();
    lcd.print("Connection failed");
    return;
  }

  /* Read data until either the connection is closed, or the idle timeout is reached. */
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      if (c == '\n') {
        if (line.startsWith("Date:")) {
          lcd.clear();
          lcd.print(line);
        }
        line = "";
      } else {
        line += c;
      }
      Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
  Serial.println("");

  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
  cc3000.disconnect();
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n"));

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

  initCC3000();

  queryTime();
}
