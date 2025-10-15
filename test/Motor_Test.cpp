 /**
 * @file main.cpp
 * @author 
 * @brief Testing the PI Controller (Official Test!)
 * @version 1.0
 * @date 2025-10-15
 */

#include "Motor.h"

Motor_t motor;

const unsigned long CONTROL_PERIOD_MS = 100;  // Control loop period
unsigned long lastUpdateTime = 0;

// Store previous encoder counts to calculate velocity
int encoderCountOldA = 0;
int encoderCountOldB = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== PI Controller Demo ===");

  // Initialize motors and encoders
  initMotors();
  resetEncoders();

  // Start moving forward for test
  motor.direction = FORWARD;
  move(&motor);

  Serial.println("Motors initialized. Starting PI control loop...");
}

void loop() {
  unsigned long now = millis();

  if (now - lastUpdateTime >= 500) {
    lastUpdateTime = now;

    // Calculate velocities (counts per interval)
    float velA = 100.0f * (encoderCountA - encoderCountOldA) / (float)PI_UPDATE_INTERVAL_MS;
    float velB = 100.0f * (encoderCountB - encoderCountOldB) / (float)PI_UPDATE_INTERVAL_MS;

    // Update PI controller for both motors
    updatePIController(&motor, velA, velB);

    // Apply motor direction and write new PWM via move()
    move(&motor);                                                               

    // Print debug info
    Serial.printf("EncA: %ld | velA: %.2f | rMotA: %d || EncB: %ld | velB: %.2f | rMotB: %d\n",
                  encoderCountA, velA, rMotNewA,
                  encoderCountB, velB, rMotNewB);

    // Store previous encoder counts
    encoderCountOldA = encoderCountA;
    encoderCountOldB = encoderCountB;
  }
}
