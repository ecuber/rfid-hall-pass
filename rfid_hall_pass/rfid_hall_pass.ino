/*
  Required libraries (install with Arduino IDE)
    * RTClib
    * Adafruit Thermal Printer Library
    * Adafruit BusIO
    * MFRC522 
 */

#include <MFRC522.h>
#include "Adafruit_Thermal.h"
#include "hallpass.h"
#include <RTClib.h>
RTC_DS1307 rtc;
#include "SoftwareSerial.h"
#define TX_PIN 5 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 4 // Arduino receive   GREEN WIRE   labeled TX on printer

#define LED_PIN 69 // need to wire the LED to a digital pin not the 5V

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

String lastUID = "bruh";

// Init array that will store new NUID
byte nuidPICC[4];

SoftwareSerial mySerial(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor

void setup() {  
  Serial.begin(9600);
  Serial.println("we're online");
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  mySerial.begin(19200);
  Wire.begin();
  rtc.begin();
  if (!rtc.isrunning()) {
    Serial.println("Couldn't find RTC");
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  Serial.println("RTC updated");
}

void loop() {

  // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;



  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }

  // printer
  pinMode(7, OUTPUT); digitalWrite(7, LOW);

  String userid;
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], HEX);
    userid += String(rfid.uid.uidByte[i], HEX);
  }

//  Serial.println(lastUID);
//  Serial.println(userid);
//  Serial.print(hourF);
//  Serial.print(':');
//  Serial.print(mins);
//  Serial.println();

  if (!lastUID.equals(userid)) {
    DateTime now = rtc.now();
    int pm = now.hour() >= 12;
    int hourF = pm ? now.hour() - 12 : now.hour();
    int mins = now.minute();
    int secs = now.second();
    
    printer.begin();
    printer.printBitmap(hallpass_width, hallpass_height, hallpass_data);
    printer.feed(1);
    printer.justify('L');
    printer.println("Name: _______________________");
    printer.print("Room #: ");
    printer.boldOn();
    printer.print("1225\n");
    printer.boldOff();
    printer.print("Date: ");
    printer.boldOn();
    printer.print(now.month());
    printer.print("/");
    printer.print(now.day());
    printer.print("/");
    printer.println(now.year());
    printer.boldOff();
   
    printer.print("Time out: ");
    printer.boldOn();
    printer.print(hourF);
    printer.print(':');
    if (mins < 10) {
      printer.print("0");
    }
    printer.print(mins);
    printer.println(pm ? " pm" : " am");
    printer.boldOff();
//    printer.underlineOn();
    printer.println("Pass to: ____________________");
//    printer.underlineOff();
    printer.feed(1);
    printer.println("Signature: __________________");
    printer.setSize('S');
    printer.println("Tag ID: " + userid);
    printer.feed(3);
    
    printer.sleep();
    delay(3000L);
    printer.wake();
    printer.setDefault();
  }

  lastUID = userid;
  
}
