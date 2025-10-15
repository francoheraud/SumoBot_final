#include "Motor.h"
#include "Sensors.h"
#include "Startup.h"

TFT_eSPI tft = TFT_eSPI();
Sensors_t sensor;
Motor_t motor;

#define BUF_SIZE                 8
#define DETECTION_THRESHOLD      20
#define TRACK_OPPONENT_THRESHOLD 25
#define LOST_REQUIRED            6
#define EDGE_AVOID_TIME_MS       350
#define STARTUP_ROTATE_DELAY_MS  200   // delay between rotation checks during startup

int distanceBuf[BUF_SIZE] = {0};
int bufIdx = 0;
bool bufferFilled = false;

enum RobotState { STARTUP_ROTATE, SEARCHING, CHASING, AVOID_EDGE };
RobotState currentState = STARTUP_ROTATE;

Direction lastSeenDirection = ROTATE_CCW;
unsigned long lastPIUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long edgeAvoidStart = 0;
static int detectConfirmCount = 0;

void setup() {
  pinMode(LEFT_BUTTON, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);

  initMotors();
  initSensors();

  tft.init();
  tft.setTextSize(2);
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("SUMO BOT READY!");
  tft.println("------------------");

  for (int i = 0; i < BUF_SIZE; ++i) distanceBuf[i] = 1000;
  bufIdx = 0;
  bufferFilled = false;
  lastPIUpdate = millis();

  currentState = STARTUP_ROTATE;
  motor.direction = ROTATE_CW;
  move(&motor);
}

static inline int normaliseDistanceForBuffer(int d) {
  return (d <= 0) ? 1000 : d;
}

static int getAverageDistance() {
  long sum = 0;
  for (int i = 0; i < BUF_SIZE; i++) sum += distanceBuf[i];
  return (int)(sum / BUF_SIZE);
}

static void updateDistanceBuf(int distance) {
  int d = normaliseDistanceForBuffer(distance);
  distanceBuf[bufIdx] = d;
  bufIdx = (bufIdx + 1) % BUF_SIZE;
  if (bufIdx == 0) bufferFilled = true;
}

static bool detectOpponent() {
  if (!bufferFilled) return false;
  int avg = getAverageDistance();
  int currIdx = (bufIdx - 1 + BUF_SIZE) % BUF_SIZE;
  int curr = distanceBuf[currIdx];
  bool dropDetected = (avg - curr) > DETECTION_THRESHOLD || curr < TRACK_OPPONENT_THRESHOLD;
  if (dropDetected) detectConfirmCount++;
  else detectConfirmCount = max(0, detectConfirmCount - 1);
  return (detectConfirmCount >= 2);
}

static void updateMotorControl() {
  static int encoderCountOldA = 0, encoderCountOldB = 0;
  unsigned long now = millis();
  unsigned long elapsedMs = now - lastPIUpdate;
  if (elapsedMs < PI_UPDATE_INTERVAL_MS) return;
  if (elapsedMs == 0) elapsedMs = 1;

  float velA = 100.0f * (encoderCountA - encoderCountOldA) / (float)elapsedMs;
  float velB = 100.0f * (encoderCountB - encoderCountOldB) / (float)elapsedMs;
  updatePIController(&motor, velA, velB);
  encoderCountOldA = encoderCountA;
  encoderCountOldB = encoderCountB;
  lastPIUpdate = now;
}

static void chaseMode() {
  bool haveBoth = (sensor.leftCm > 0 && sensor.rightCm > 0);
  bool opponentLeft = haveBoth && (sensor.leftCm < sensor.rightCm - TRACK_OPPONENT_THRESHOLD);
  bool opponentRight = haveBoth && (sensor.rightCm < sensor.leftCm - TRACK_OPPONENT_THRESHOLD);

  if (opponentLeft) {
    motor.direction = LEFT;
    lastSeenDirection = LEFT;
  } else if (opponentRight) {
    motor.direction = RIGHT;
    lastSeenDirection = RIGHT;
  } else {
    motor.direction = FORWARD;
  }

  move(&motor);
}

static bool lineDetected() {
  return (sensor.frontLeft || sensor.frontRight || sensor.rearLeft || sensor.rearRight);
}

static Direction edgeAvoidDirection() {
  if (sensor.frontLeft)  return RIGHT;
  if (sensor.frontRight) return LEFT;
  if (sensor.rearLeft)   return RIGHT;
  if (sensor.rearRight)  return LEFT;
  return ROTATE_CW;
}

static void updateDisplay(int left, int right, int avg) {
  unsigned long now = millis();
  if (now - lastDisplayUpdate < 100) return;
  lastDisplayUpdate = now;

  tft.fillRect(0, 40, 240, 120, TFT_BLACK);
  tft.setCursor(20, 40);
  tft.printf("Left : %4d cm\n", left);
  tft.printf("Right: %4d cm\n", right);
  tft.printf("Avg  : %4d cm\n", avg);
  tft.printf("State: %s\n",
    (currentState == STARTUP_ROTATE) ? "STARTUP" :
    (currentState == SEARCHING) ? "SEARCH" :
    (currentState == CHASING)  ? "CHASE" : "EDGE");
  tft.printf("FL:%d FR:%d \n RL:%d RR:%d\n",
    sensor.frontLeft, sensor.frontRight, sensor.rearLeft, sensor.rearRight);
}

void loop() {
  pollDistance(&sensor);
  delay(5);
  pollDistance(&sensor);
  detectLine(&sensor);

  int left = normaliseDistanceForBuffer(sensor.leftCm);
  int right = normaliseDistanceForBuffer(sensor.rightCm);
  int avg = (left + right) / 2;
  updateDistanceBuf(avg);

  switch (currentState) {
    case STARTUP_ROTATE:
      motor.direction = ROTATE_CW;
      move(&motor);
      if (detectOpponent()) {
        currentState = CHASING;
        detectConfirmCount = 0;
      }
      delay(STARTUP_ROTATE_DELAY_MS);
      break;

    case SEARCHING:
      motor.direction = lastSeenDirection;
      move(&motor);
      if (lineDetected()) {
        currentState = AVOID_EDGE;
        edgeAvoidStart = millis();
        motor.direction = edgeAvoidDirection();
        move(&motor);
      } else if (detectOpponent()) {
        currentState = CHASING;
        detectConfirmCount = 0;
      }
      break;

    case CHASING:
      chaseMode();
      if (lineDetected()) {
        currentState = AVOID_EDGE;
        edgeAvoidStart = millis();
        motor.direction = edgeAvoidDirection();
        move(&motor);
      } else if (sensor.leftCm == OUT_OF_RANGE && sensor.rightCm == OUT_OF_RANGE) {
        currentState = SEARCHING;
      }
      break;

    case AVOID_EDGE:
      move(&motor);
      if (millis() - edgeAvoidStart > EDGE_AVOID_TIME_MS) {
        currentState = SEARCHING;
      }
      break;
  }

  updateMotorControl();
  updateDisplay(sensor.leftCm, sensor.rightCm, avg);
}
