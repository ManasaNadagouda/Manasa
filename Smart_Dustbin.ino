#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// =====================================================
// SMART DUSTBIN - FINAL CODE
// ESP32 + 2 HC-SR04 + Servo + LCD + Buzzer + LED
// USB POWERED
// =====================================================


// ================= LCD =================

LiquidCrystal_I2C lcd(0x27, 16, 2);


// ================= SERVO =================

Servo lidServo;


// ================= PIN CONNECTIONS =================

// Hand detection HC-SR04
#define HAND_TRIG 5
#define HAND_ECHO 18

// Fill level HC-SR04
#define FILL_TRIG 19
#define FILL_ECHO 23

// Servo
#define SERVO_PIN 13

// Buzzer
#define BUZZER_PIN 25

// LED
#define LED_PIN 26


// ================= SETTINGS =================

// Hand must be within 10 cm
#define HAND_DISTANCE 10.0

// Fill-level calibration
#define EMPTY_DISTANCE 12.0
#define FULL_DISTANCE 3.0

// Alert at 80%
#define ALERT_LEVEL 80

// Servo positions
#define LID_CLOSED 0
#define LID_OPEN 90


// =====================================================
// USB POWER ESTIMATION
// =====================================================
// These are estimated values in mA.
// They are NOT battery measurements.

const float CURRENT_IDLE = 40.0;
const float CURRENT_SENSING = 150.0;
const float CURRENT_ALERT = 80.0;

const float USB_VOLTAGE = 5.0;


// =====================================================
// TIME TRACKING
// =====================================================

unsigned long idleTimeMs = 0;
unsigned long sensingTimeMs = 0;
unsigned long alertTimeMs = 0;

unsigned long previousMillis = 0;
unsigned long lastReportMillis = 0;


// =====================================================
// GET ULTRASONIC DISTANCE
// =====================================================

float getDistance(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0)
  {
    return -1;
  }

  float distance = duration * 0.0343 / 2.0;

  return distance;
}


// =====================================================
// CALCULATE FILL PERCENTAGE
// =====================================================

int getFillPercentage(float distance)
{
  if (distance < 0)
  {
    return -1;
  }

  float percentage =
    ((EMPTY_DISTANCE - distance) /
    (EMPTY_DISTANCE - FULL_DISTANCE)) * 100.0;

  percentage = constrain(percentage, 0, 100);

  return (int)percentage;
}


// =====================================================
// DISPLAY FILL LEVEL
// =====================================================

void displayFill(int percentage)
{
  lcd.setCursor(0, 1);

  if (percentage >= 0)
  {
    lcd.print("Fill: ");
    lcd.print(percentage);
    lcd.print("%     ");
  }
  else
  {
    lcd.print("Fill: ERROR     ");
  }
}


// =====================================================
// POWER REPORT
// =====================================================

void printPowerReport()
{
  float idleHours =
    idleTimeMs / 3600000.0;

  float sensingHours =
    sensingTimeMs / 3600000.0;

  float alertHours =
    alertTimeMs / 3600000.0;


  // Estimated energy in mAh

  float idlemAh =
    CURRENT_IDLE * idleHours;

  float sensingmAh =
    CURRENT_SENSING * sensingHours;

  float alertmAh =
    CURRENT_ALERT * alertHours;

  float totalmAh =
    idlemAh + sensingmAh + alertmAh;


  // Estimated power in Watts

  float idlePower =
    USB_VOLTAGE * (CURRENT_IDLE / 1000.0);

  float sensingPower =
    USB_VOLTAGE * (CURRENT_SENSING / 1000.0);

  float alertPower =
    USB_VOLTAGE * (CURRENT_ALERT / 1000.0);


  Serial.println();
  Serial.println("====================================");
  Serial.println("       SMART DUSTBIN REPORT");
  Serial.println("====================================");

  Serial.print("USB Voltage: ");
  Serial.print(USB_VOLTAGE);
  Serial.println(" V");

  Serial.println();

  // IDLE

  Serial.print("IDLE      : ");
  Serial.print(idleTimeMs / 1000);
  Serial.print(" s | ");

  Serial.print(CURRENT_IDLE);
  Serial.print(" mA | ");

  Serial.print(idlePower, 3);
  Serial.println(" W");


  // SENSING

  Serial.print("SENSING   : ");
  Serial.print(sensingTimeMs / 1000);
  Serial.print(" s | ");

  Serial.print(CURRENT_SENSING);
  Serial.print(" mA | ");

  Serial.print(sensingPower, 3);
  Serial.println(" W");


  // ALERT

  Serial.print("ALERT     : ");
  Serial.print(alertTimeMs / 1000);
  Serial.print(" s | ");

  Serial.print(CURRENT_ALERT);
  Serial.print(" mA | ");

  Serial.print(alertPower, 3);
  Serial.println(" W");


  Serial.println("------------------------------------");

  Serial.print("Estimated Energy: ");
  Serial.print(totalmAh, 4);
  Serial.println(" mAh");

  Serial.println("====================================");
  Serial.println();
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);


  // ---------------- Ultrasonic sensors ----------------

  pinMode(HAND_TRIG, OUTPUT);
  pinMode(HAND_ECHO, INPUT);

  pinMode(FILL_TRIG, OUTPUT);
  pinMode(FILL_ECHO, INPUT);


  // ---------------- Buzzer and LED ----------------

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);


  // ---------------- LCD ----------------

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();


  // ---------------- Servo ----------------

  lidServo.attach(SERVO_PIN);

  lidServo.write(LID_CLOSED);


  // ---------------- Startup screen ----------------

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("SMART DUSTBIN");

  lcd.setCursor(0, 1);
  lcd.print("SYSTEM READY");

  delay(2000);

  lcd.clear();


  // ---------------- Start timing ----------------

  previousMillis = millis();
  lastReportMillis = millis();
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{
  unsigned long currentMillis = millis();

  unsigned long elapsedTime =
    currentMillis - previousMillis;

  previousMillis = currentMillis;


  // ===================================================
  // FILL SENSOR
  // ===================================================

  float fillDistance =
    getDistance(FILL_TRIG, FILL_ECHO);

  int fillPercentage =
    getFillPercentage(fillDistance);


  // ===================================================
  // ALERT STATE
  // ===================================================

  if (fillPercentage >= ALERT_LEVEL)
  {
    alertTimeMs += elapsedTime;

    digitalWrite(LED_PIN, HIGH);


    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("!! ALERT !!");

    lcd.setCursor(0, 1);
    lcd.print("BIN ");
    lcd.print(fillPercentage);
    lcd.print("% FULL");


    Serial.print("STATE: ALERT | Fill: ");
    Serial.print(fillPercentage);
    Serial.println("%");


    // Buzzer ON

    digitalWrite(BUZZER_PIN, HIGH);
    delay(250);

    // Buzzer OFF

    digitalWrite(BUZZER_PIN, LOW);
    delay(750);
  }


  // ===================================================
  // NORMAL OPERATION
  // ===================================================

  else
  {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);


    // =================================================
    // HAND SENSOR
    // =================================================

    float handDistance =
      getDistance(HAND_TRIG, HAND_ECHO);


    // =================================================
    // SENSING STATE
    // =================================================

    if (handDistance > 0 &&
        handDistance <= HAND_DISTANCE)
    {
      sensingTimeMs += elapsedTime;


      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("SENSING...");

      displayFill(fillPercentage);


      Serial.print("STATE: SENSING | Fill: ");
      Serial.print(fillPercentage);
      Serial.println("%");


      // Open lid

      lidServo.write(LID_OPEN);

      delay(3000);


      // Close lid

      lidServo.write(LID_CLOSED);

      delay(1000);
    }


    // =================================================
    // IDLE STATE
    // =================================================

    else
    {
      idleTimeMs += elapsedTime;


      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("IDLE");

      displayFill(fillPercentage);


      Serial.print("STATE: IDLE | Fill: ");

      if (fillPercentage >= 0)
      {
        Serial.print(fillPercentage);
        Serial.println("%");
      }
      else
      {
        Serial.println("ERROR");
      }

      delay(500);
    }
  }


  // ===================================================
  // POWER REPORT EVERY 10 SECONDS
  // ===================================================

  if (millis() - lastReportMillis >= 10000)
  {
    printPowerReport();

    lastReportMillis = millis();
  }
}
