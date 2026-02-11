#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pins for Ultrasonic Sensors
#define SLOT1_TRIG 2
#define SLOT1_ECHO 3
#define SLOT2_TRIG 4
#define SLOT2_ECHO 5

// RFID pins
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

// Servo
Servo gateServo;
#define SERVO_PIN 6

// Buzzer and LEDs
#define BUZZER_PIN 7
#define RED_LED A0
#define GREEN_LED A1

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Update address if needed

// Variables
int carCount = 0;
bool slot1Occupied = false;
bool slot2Occupied = false;
String lastMessage = "";
byte authorizedUID[4] = {0x8C, 0xAE, 0xF6, 0x04}; // Replace with your RFID card UID

void setup() {
  Serial.begin(9600);

  // Sensor pins
  pinMode(SLOT1_TRIG, OUTPUT);
  pinMode(SLOT1_ECHO, INPUT);
  pinMode(SLOT2_TRIG, OUTPUT);
  pinMode(SLOT2_ECHO, INPUT);

  // RFID and Servo
  SPI.begin();
  rfid.PCD_Init();
  gateServo.attach(SERVO_PIN);
  gateServo.write(0); // Gate closed

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");
  delay(2000);
  lcd.clear();

  // Buzzer & LEDs
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // Make sure LEDs and buzzer are off
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
}

void loop() {
  updateSlotStatus();
  showParkingStatus();

  String currentMessage;

  if (carCount < 2) {
    currentMessage = "Scan your card";
  } else {
    currentMessage = "No Slot";
  }

  if (currentMessage != lastMessage) {
    clearLine(1);
    lcd.setCursor(0, 1);
    lcd.print(currentMessage);
    lastMessage = currentMessage;
  }

  if (carCount < 2) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      if (isAuthorized(rfid.uid.uidByte)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Granted");
        Serial.println("access:granted");

        digitalWrite(GREEN_LED, HIGH); 
        digitalWrite(BUZZER_PIN, HIGH);
        delay(1000);
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(GREEN_LED, LOW);

        openGate();
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Denied");
        Serial.println("access:denied");

        digitalWrite(RED_LED, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(1000);
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(RED_LED, LOW);

        delay(1000);
      }

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      lastMessage = "";
    }
  } else {
    // Slot not available, but user tries to scan
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      Serial.println("no-slot");

      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(2000);
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(RED_LED, LOW);

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      lastMessage = "";
    }
  }

  // Send parking info to Serial
  Serial.print("count:");
  Serial.print(carCount);
  Serial.print(",available:");
  Serial.println(2 - carCount);
}

void updateSlotStatus() {
  float d1 = getDistance(SLOT1_TRIG, SLOT1_ECHO);
  float d2 = getDistance(SLOT2_TRIG, SLOT2_ECHO);

  if (d1 < 5 && !slot1Occupied) {
    slot1Occupied = true;
    carCount++;
  } else if (d1 >= 5 && slot1Occupied) {
    slot1Occupied = false;
    carCount--;
  }

  if (d2 < 5 && !slot2Occupied) {
    slot2Occupied = true;
    carCount++;
  } else if (d2 >= 5 && slot2Occupied) {
    slot2Occupied = false;
    carCount--;
  }

  carCount = constrain(carCount, 0, 2);
}

float getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH);
  return duration * 0.0343 / 2;
}

void showParkingStatus() {
  lcd.setCursor(0, 0);
  lcd.print("Available: ");
  lcd.print(2 - carCount);
  lcd.print("   ");
}

bool isAuthorized(byte *uid) {
  for (int i = 0; i < 4; i++) {
    if (uid[i] != authorizedUID[i])
      return false;
  }
  return true;
}

void openGate() {
  gateServo.write(90);
  delay(3000);
  gateServo.write(0);
  delay(500);
}

void clearLine(int line) {
  lcd.setCursor(0, line);
  lcd.print("                ");
}