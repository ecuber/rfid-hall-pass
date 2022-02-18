/*
  Required libraries (install with Arduino IDE)
      RTClib
      Adafruit Thermal Printer Library
      Adafruit BusIO
      MFRC522
*/

/*******CARD SETUP********/
String uids [] = {"958ed9c3", "b1c51a31"};
String names [] = {"Elijah", "Jesse"};
#define NUM_STUDENTS 2

String teacherUIDs [] = {"223b331c", "6596dcc3"};
String teachers [] = {"Mr. Martinez", "Dr. Wu"};
#define NUM_TEACHERS 2

/*****MISC. VARIABLES*****/
#define LED_PIN 69 // need to wire the LED to a digital pin not the 5V
#include "hallpass.h" // header image
String lastUID = "bruh"; // resets every time a card is scanned

// Reason for getting a pass
#define MODES String[] = {"Bathroom/Water", "Library (10 min max)", "Learning Community", "Appt. w/ Support Staff"}

/********PRINTER*********/
#include "Adafruit_Thermal.h"
#include "SoftwareSerial.h"

#define TX_PIN 5 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 4 // Arduino receive   GREEN WIRE   labeled TX on printer

SoftwareSerial mySerial(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor


/**********RFID*********/
#include <SPI.h>
#include <MFRC522.h>

// pin setup
#define RST_PIN 9
#define SS_PIN 10 // Labeled SDA on chip
// DEFINED BY LIBRARY:
// MOSI: 11
// MISO: 12
// SCK: 13
// IRQ: unconected
// https://github.com/miguelbalboa/rfid

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];


/**********RTC*********/
#include <RTClib.h>
RTC_DS1307 rtc;


void setup() {
  Serial.begin(9600);
  Serial.println("Arduino initializing...");
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  // Initialize key hex byte array
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // SoftwareSerial connection for printer output
  mySerial.begin(19200);
  Wire.begin();

  rtc.begin();
  if (!rtc.isrunning()) {
    Serial.println("Couldn't find RTC.");
  } else {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("RTC updated.");
  }
  Serial.print("Onboard time: ");
  DateTime now = rtc.now();
  Serial.println(getDate(now) + " " + getTime(now));

  Serial.println("Initialization complete.");

  //
  //  Serial.print("indexof jesse: ");
  //  Serial.println(indexOf(names, NUM_STUDENTS, "alksj"));
  //  Serial.println(names[indexOf(uids, NUM_STUDENTS, "B1C51A31")]);
}

bool readerActive = true;


void loop() {
  if (readerActive) {
    // Look for new cards
    if ( ! rfid.PICC_IsNewCardPresent())
      return;

    // Verify if the NUID has been readed
    if ( ! rfid.PICC_ReadCardSerial())
      return;

    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    String userid;
    for (byte i = 0; i < rfid.uid.size; i++) {
      userid += String(rfid.uid.uidByte[i], HEX);
    }
    //
    Serial.print("outer: ");
    Serial.println(userid);

    if (indexOf(uids, NUM_STUDENTS, userid) != -1) {
      Serial.println("found ID");
      unsigned long startingTime = millis();
      while (millis() < startingTime + 15000) {
        // Look for new cards
        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
          for (byte i = 0; i < 4; i++) {
            nuidPICC[i] = rfid.uid.uidByte[i];
          }

          String secondUserId;
          for (byte i = 0; i < rfid.uid.size; i++) {
            secondUserId += String(rfid.uid.uidByte[i], HEX);
          }

          if (indexOf(teacherUIDs, NUM_TEACHERS, secondUserId) != -1) {
            Serial.println("triggering print");
            DateTime now = rtc.now();

            // Activate printer
            pinMode(7, OUTPUT);
            digitalWrite(7, LOW);

            printer.begin();
            printer.printBitmap(hallpass_width, hallpass_height, hallpass_data);
            printer.feed(1);
            printer.justify('L');
            printer.print("Name: ");
            String studentName = names[indexOf(uids, NUM_STUDENTS, userid)];
            printer.boldOn();
            printer.println(studentName);
            printer.boldOff();
            printer.print("Room #: ");
            printer.boldOn();
            printer.print("1225\n");
            printer.boldOff();
            printer.print("Date: ");
            printer.boldOn();
            printer.println(getDate(now));
            printer.boldOff();

            printer.print("Time out: ");
            printer.boldOn();
            printer.println(getTime(now));
            printer.boldOff();
            printer.println("Pass to: ____________________");
            printer.feed(1);
            printer.print("Signature: ");
            String teacherName = teachers[indexOf(teacherUIDs, NUM_STUDENTS, secondUserId)];
            printer.boldOn();
            printer.println(teacherName);
            printer.boldOff();
            printer.setSize('S');
            printer.println("Tag ID: " + userid);
            printer.feed(3);

            printer.sleep();
            delay(3000L);
            printer.wake();
            printer.setDefault();
          }
        }


      }
    }

    //    Serial.println(names[indexOf(uids, NUM_STUDENTS, userid)]);
    //    Serial.print("\n");


  }
}

String getTime(DateTime now) {
  int pm = now.hour() >= 12;
  int hourF = pm ? now.hour() - 12 : now.hour();
  int mins = now.minute();
  int secs = now.second();

  return String(hourF) + ":" + String(mins) + (mins < 10 ? "0" : "") + " " + (pm ? "pm" : "am");
}

String getDate(DateTime now) {
  return String(now.month()) + "/" + String(now.day()) + "/" + String(now.year());
}

int indexOf(String *arr, int len, String search) {
  for (int i = 0; i < len; i++) {
    if (search.equals(arr[i])) {
      return i;
    }
  }
  return -1;
}
