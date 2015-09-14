#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include <LiquidCrystal.h>
#include <Time.h>
#include "wifi_creds.h"

#define DISPLAY_BUTTON_PIN 2
#define LCD_LIGHT_PIN      6

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   2  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  9
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

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

void initCC3000() {
  lcd.clear();
  if (!cc3000.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("Couldn't begin()");
    while(1);
  }

  lcd.print("Connecting...");
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    lcd.setCursor(0, 1);
    lcd.print("Failed!");
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

  uint32_t ip, netmask, gateway, dhcpserv, dnsserv;
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

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  lcd.noDisplay();

  pinMode(DISPLAY_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(0, showTime, FALLING);

  // Set the LCD display backlight pin as an output.
  pinMode(LCD_LIGHT_PIN, OUTPUT);

  // Turn off the LCD backlight.
  digitalWrite(LCD_LIGHT_PIN, LOW);

  turnOnDisplay();

  initCC3000();
}

uint32_t ip;

/* void xsetup(void) { */
/*   Serial.begin(115200); */
/*   Serial.println(F("Hello, CC3000!\n"));  */
/*  */
/*   Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC); */
/*    */
/*   #<{(| Initialise the module |)}># */
/*   Serial.println(F("\nInitializing...")); */
/*   if (!cc3000.begin()) */
/*   { */
/*     Serial.println(F("Couldn't begin()! Check your wiring?")); */
/*     while(1); */
/*   } */
/*    */
/*   // Optional SSID scan */
/*   // listSSIDResults(); */
/*    */
/*   Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID); */
/*   if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) { */
/*     Serial.println(F("Failed!")); */
/*     while(1); */
/*   } */
/*     */
/*   Serial.println(F("Connected!")); */
/*    */
/*   #<{(| Wait for DHCP to complete |)}># */
/*   Serial.println(F("Request DHCP")); */
/*   while (!cc3000.checkDHCP()) */
/*   { */
/*     delay(100); // ToDo: Insert a DHCP timeout! */
/*   }   */
/*  */
/*   #<{(| Display the IP address DNS, Gateway, etc. |)}>#   */
/*   while (! displayConnectionDetails()) { */
/*     delay(1000); */
/*   } */
/*  */
/*   ip = 0; */
/*   // Try looking up the website's IP address */
/*   Serial.print(WEBSITE); Serial.print(F(" -> ")); */
/*   while (ip == 0) { */
/*     if (! cc3000.getHostByName(WEBSITE, &ip)) { */
/*       Serial.println(F("Couldn't resolve!")); */
/*     } */
/*     delay(500); */
/*   } */
/*  */
/*   cc3000.printIPdotsRev(ip); */
/*    */
/*   // Optional: Do a ping test on the website */
/*   #<{(| */
/*   Serial.print(F("\n\rPinging ")); cc3000.printIPdotsRev(ip); Serial.print("...");   */
/*   replies = cc3000.ping(ip, 5); */
/*   Serial.print(replies); Serial.println(F(" replies")); */
/*   |)}>#   */
/*  */
/*   #<{(| Try connecting to the website. */
/*      Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read. */
/*   |)}># */
/*   Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80); */
/*   if (www.connected()) { */
/*     www.fastrprint(F("GET ")); */
/*     www.fastrprint(WEBPAGE); */
/*     www.fastrprint(F(" HTTP/1.1\r\n")); */
/*     www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n")); */
/*     www.fastrprint(F("\r\n")); */
/*     www.println(); */
/*   } else { */
/*     Serial.println(F("Connection failed"));     */
/*     return; */
/*   } */
/*  */
/*   Serial.println(F("-------------------------------------")); */
/*    */
/*   #<{(| Read data until either the connection is closed, or the idle timeout is reached. |)}>#  */
/*   unsigned long lastRead = millis(); */
/*   while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) { */
/*     while (www.available()) { */
/*       char c = www.read(); */
/*       Serial.print(c); */
/*       lastRead = millis(); */
/*     } */
/*   } */
/*   www.close(); */
/*   Serial.println(F("-------------------------------------")); */
/*    */
/*   #<{(| You need to make sure to clean up after yourself or the CC3000 can freak out |)}># */
/*   #<{(| the next time your try to connect ... |)}># */
/*   Serial.println(F("\n\nDisconnecting")); */
/*   cc3000.disconnect(); */
/*    */
/* } */
