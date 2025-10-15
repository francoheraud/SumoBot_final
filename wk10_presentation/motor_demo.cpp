// BASIC DRIVER CODE (copy and paste this into main.cpp)

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Motor.h"
#include "Encoder.h"

#define IN1A 2
#define IN2A 3
#define PWMA 1
#define ENCA 16

#define IN1B 43
#define IN2B 44
#define PWMB 18
#define ENCB 21

void runMotion(Direction dir, SpeedLevel speed, unsigned long durationMs) {
    move(dir, speed);
    unsigned long startTime = millis();
    while (millis() - startTime < durationMs) {
        updateMotors();  // continuously run PID
    }
    stopMotors();
}

const int delayMillis = 100;
const int ENCODER_PIN = 1;
int count = 0;
int prev_count = 0;

TFT_eSPI tft = TFT_eSPI();

void increment();

void increment() {
  count++;
  tft.drawString(String(count) + " ticks", 0, 0);
  return;
}

/*
void setup() {
  Serial.begin(115200);
  pinMode(ENCODER_PIN, INPUT);
  attachInterrupt(ENCODER_PIN, increment, RISING); // increments after each gap

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setRotation(3);
  tft.drawString("0 ticks", 0, 0);
}

void loop() {
  static unsigned long lastUpdateTime = millis();
  static char speed_printout[50];
  static float speed;
  if (millis() - lastUpdateTime > delayMillis)
  {
    Serial.print("Count: ");
    Serial.println(count);
    Serial.print("Prev: ");
    Serial.println(prev_count);
    speed = (count - prev_count)*1.0 / (0.001*delayMillis);
    Serial.print("Speed: ");
    Serial.println(speed);
    sprintf(speed_printout, "%.1f ticks/s  ", speed);
    tft.drawString(speed_printout, 0, 40);
    prev_count = count;
    lastUpdateTime = millis();
  }
}
*/

void setup() {
    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setRotation(1);
    Serial.begin(115200);

    // Initialize motors and encoders
    initMotors(IN1A, IN2A, PWMA, ENCA, IN1B, IN2B, PWMB, ENCB);
}

void loop() {
    // Forward at LOW, MEDIUM, HIGH
    tft.drawString("FORWARD AT MIN SPEED             ", 0, 0);
    runMotion(FORWARD, MIN, 4000);
    delay(1000);
    tft.drawString("FORWARD AT MEDIUM SPEED             ", 0, 0);
    runMotion(FORWARD, MEDIUM, 4000);
    delay(1000);
    tft.drawString("FORWARD AT MAX SPEED             ", 0, 0); // need encoders?
    runMotion(FORWARD, MAX, 4000);
    delay(1000);

    // Reverse at MEDIUM
    tft.drawString("REVERSING AT MEDIUM SPEED             ", 0, 0); // 2ND doesn't move
    runMotion(REVERSE, MEDIUM, 4000);
    delay(1000);

    
    // Turn Right and Left (Medium)
    tft.drawString("TURN RIGHT             ", 0, 0); // Not working
    runMotion(RIGHT, MEDIUM, 4000);
    delay(1000);
    tft.drawString("TURN LEFT             ", 0, 0); // Working
    delay(1000);
    runMotion(LEFT, MEDIUM, 4000);

    // Rotate 360 CW and CCW at MEDIUM
    tft.drawString("ROTATE CLOCKWISE             ", 0, 0); // Not working
    runMotion(ROTATE_CW, MEDIUM, 4000);
    delay(1000);
    tft.drawString("ROTATE COUNTERCLOCKWISE     ", 0, 0); // Not working
    runMotion(ROTATE_CCW, MEDIUM, 4000);
    delay(1000);

    // Stop after testing
    tft.drawString("STOP MOTORS             ", 0, 0);
    while (true) stopMotors();
}
