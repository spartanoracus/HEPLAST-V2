/**
 * ==============================================================
 *  Arduino Filament Extruder Controller
 *  --------------------------------------------------------------
 *  Description:
 *  This program controls a basic filament extruder setup. It features:
 * 
 *    - Security system: Potentiometer must be zeroed, then button pressed
 *    - LCD interface: Target temperature (fixed), mock room temp, and RPM display
 *    - Smooth PWM breathing LED while locked
 *    - Relay-controlled cartridge heater (mock condition based on temp)
 *    - L298N controlled DC motor with RPM set by potentiometer
 *    - Heater status red LED
 *    - Debug info printed via Serial Monitor
 * 
 *  Components:
 *    - Arduino Uno or Nano
 *    - 10k Potentiometer
 *    - Push-button (to GND)
 *    - Blue LED (status - PWM breathing)
 *    - Red LED (heater on)
 *    - Relay module (heater control)
 *    - L298N motor driver (ENB, IN3, IN4)
 *    - 12V DC motor
 *    - 16x2 I2C LCD (0x27 or 0x3F)
 * 
 *  Pin Configuration:
 *    Potentiometer        → A2
 *    Button               → D3
 *    Blue Status LED      → D10 (PWM)
 *    Red Heater LED       → D9
 *    Relay IN             → D8
 *    L298N ENB            → D5 (PWM)
 *    L298N IN3            → D6
 *    L298N IN4            → D7
 *    LCD SDA/SCL          → A4 / A5
 * 
 *  Author:     [YourNameHere]
 *  Created:    2025
 *  License:    MIT (optional)
 * ==============================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// === Pin Assignments ===
#define POT_SPEED A2
#define BUTTON_PIN 3
#define LED_READY 10
#define HEATER_LED 9
#define RELAY_PIN 8
#define MOTOR_ENB 5
#define MOTOR_IN3 6
#define MOTOR_IN4 7

// === Configuration Constants ===
const float targetTemp = 20.0;
const float roomTempMock = 27.0;
const int potZeroThreshold = 20;
const int maxBreathBrightness = 50;
const int fadeDelay = 80;  // ms

// === System State ===
bool systemReady = false;
int fadeValue = 0;
int fadeAmount = 2;
unsigned long prevFadeMillis = 0;

// === LCD Setup ===
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Change to 0x3F if needed

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_READY, OUTPUT);
  pinMode(HEATER_LED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(MOTOR_ENB, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);

  // Initial system OFF
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(HEATER_LED, LOW);
  analogWrite(MOTOR_ENB, 0);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Locked");
  delay(1000);
  lcd.clear();
}

void loop() {
  int potVal = analogRead(POT_SPEED);
  float rpmTarget = map(potVal, 0, 1023, 0, 300) / 100.0;
  int motorPWM = map(potVal, 0, 1023, 0, 255);
  int buttonState = digitalRead(BUTTON_PIN);

  // === Serial Debug Info ===
  Serial.print("Target: ");
  Serial.print(targetTemp);
  Serial.print("C | Room: ");
  Serial.print(roomTempMock);
  Serial.print("C | RPM: ");
  Serial.print(rpmTarget, 2);
  Serial.print(" | PWM: ");
  Serial.print(motorPWM);
  Serial.print(" | BTN: ");
  Serial.println(buttonState);

  // === LCD Display ===
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print((int)targetTemp);
  lcd.print("C RPM:");
  char rpmBuf[6];
  dtostrf(rpmTarget, 4, 2, rpmBuf);
  lcd.print(rpmBuf);

  if (!systemReady) {
    // === LOCKED STATE ===
    if (millis() - prevFadeMillis >= fadeDelay) {
      fadeValue += fadeAmount;
      if (fadeValue <= 0 || fadeValue >= maxBreathBrightness)
        fadeAmount = -fadeAmount;
      analogWrite(LED_READY, fadeValue);
      prevFadeMillis = millis();
    }

    if (potVal < potZeroThreshold) {
      lcd.setCursor(0, 1);
      lcd.print("Press Button...  ");
      if (buttonState == LOW) {
        systemReady = true;
        analogWrite(LED_READY, maxBreathBrightness);
        lcd.setCursor(0, 1);
        lcd.print("System Active    ");
        Serial.println(">> SYSTEM UNLOCKED <<");
      }
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Set pot to zero  ");
    }

    // Ensure everything is OFF
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(HEATER_LED, LOW);
    analogWrite(MOTOR_ENB, 0);
    digitalWrite(MOTOR_IN3, LOW);
    digitalWrite(MOTOR_IN4, LOW);

  } else {
    // === UNLOCKED STATE ===
    analogWrite(LED_READY, maxBreathBrightness);
    lcd.setCursor(0, 1);
    lcd.print("System Active    ");

    // Motor Control
    digitalWrite(MOTOR_IN3, HIGH);
    digitalWrite(MOTOR_IN4, LOW);
    analogWrite(MOTOR_ENB, motorPWM);

    // Heater Control
    if (roomTempMock < targetTemp) {
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(HEATER_LED, HIGH);
    } else {
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(HEATER_LED, LOW);
    }
  }

  delay(20);  // Loop stability
}
