#include "Motor.h"
#include "Sensors.h"
#include "Startup.h"

TFT_eSPI tft = TFT_eSPI();

Sensors_t sensor;
Motor_t motor;

#define BUF_SIZE                    10
#define DETECTION_THRESHOLD         30
#define TRACK_OPPONENT_THRESHOLD    30
#define EDGE_BACKUP_TIME            300

bool startupSequence = true, opponentDetected = false, startupTurning = true;

int distanceBuf[BUF_SIZE] = {0};
int bufIdx = 0;
bool bufferFilled = false;

enum RobotState {
    SEARCHING,
    CHASING,
    AVOIDING_EDGE
};

RobotState currentState = SEARCHING;
unsigned long edgeAvoidStartTime = 0;
Direction edgeAvoidDirection = ROTATE_CW;
unsigned long lastPIUpdate = 0;

void setup() {
    pinMode(LEFT_BUTTON, INPUT);
    pinMode(RIGHT_BUTTON, INPUT);
    initMotors();
    initSensors();

    tft.init();
    tft.setTextSize(1);
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    userSelectFunction(&tft, &sensor, &motor);

    motor.direction = ROTATE_CCW;
    for (int i = 0; i < BUF_SIZE; ++i) distanceBuf[i] = 1000;
    bufIdx = 0;
    bufferFilled = false;
    lastPIUpdate = millis();
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
    int aveDistance = getAverageDistance();
    int currIdx = (bufIdx - 1 + BUF_SIZE) % BUF_SIZE;
    int currDistance = distanceBuf[currIdx];
    if (currDistance < TRACK_OPPONENT_THRESHOLD) return true;
    if ((aveDistance - currDistance) > DETECTION_THRESHOLD) return true;
    return false;
}

static bool edgeDetected() {
    return sensor.frontLeft || sensor.frontRight || sensor.rearLeft || sensor.rearRight;
}

static Direction getEdgeAvoidDirection() {
    if (sensor.frontLeft || sensor.frontRight) return REVERSE;
    if (sensor.rearLeft || sensor.rearRight) return FORWARD;
    if (sensor.frontLeft || sensor.rearLeft) return ROTATE_CW;
    if (sensor.frontRight || sensor.rearRight) return ROTATE_CCW;
    return ROTATE_CCW;
}

static void updateMotorControl() {
    static int encoderCountOldA = 0;
    static int encoderCountOldB = 0;
    unsigned long now = millis();
    unsigned long elapsedMs = now - lastPIUpdate;
    if (elapsedMs < PI_UPDATE_INTERVAL_MS) return;
    if (elapsedMs == 0) elapsedMs = 1;
    float velA = 1000.0f * (encoderCountA - encoderCountOldA) / (float)elapsedMs;
    float velB = 1000.0f * (encoderCountB - encoderCountOldB) / (float)elapsedMs;
    updatePIController(&motor, velA, velB);
    encoderCountOldA = encoderCountA;
    encoderCountOldB = encoderCountB;
    lastPIUpdate = now;
}

static void chaseMode() {
    bool opponentLeft = (sensor.leftCm > 0 && sensor.rightCm > 0) &&
                        (sensor.leftCm < sensor.rightCm - TRACK_OPPONENT_THRESHOLD);
    bool opponentRight = (sensor.leftCm > 0 && sensor.rightCm > 0) &&
                         (sensor.rightCm < sensor.leftCm - TRACK_OPPONENT_THRESHOLD);
    if (opponentLeft) motor.direction = LEFT;
    else if (opponentRight) motor.direction = RIGHT;
    else motor.direction = FORWARD;
    move(&motor);
}

void loop() {
    pollDistance(&sensor);
    pollDistance(&sensor);
    detectLine(&sensor);

    int leftForBuf = normaliseDistanceForBuffer(sensor.leftCm);
    int rightForBuf = normaliseDistanceForBuffer(sensor.rightCm);
    int avgDistanceForBuf = (leftForBuf + rightForBuf) / 2;
    updateDistanceBuf(avgDistanceForBuf);

    switch (currentState) {
        case SEARCHING:
            motor.direction = ROTATE_CCW;
            if (detectOpponent()) currentState = CHASING;
            if (edgeDetected()) {
                currentState = AVOIDING_EDGE;
                edgeAvoidStartTime = millis();
                edgeAvoidDirection = getEdgeAvoidDirection();
                stopMotors();
                delay(50);
            }
            break;

        case CHASING:
            chaseMode();
            if (edgeDetected()) {
                currentState = AVOIDING_EDGE;
                edgeAvoidStartTime = millis();
                edgeAvoidDirection = getEdgeAvoidDirection();
                stopMotors();
                delay(50);
            }
            if (sensor.leftCm == OUT_OF_RANGE && sensor.rightCm == OUT_OF_RANGE)
                currentState = SEARCHING;
            break;

        case AVOIDING_EDGE:
            motor.direction = edgeAvoidDirection;
            move(&motor);
            if (millis() - edgeAvoidStartTime > EDGE_BACKUP_TIME)
                currentState = SEARCHING;
            break;
    }

    move(&motor);
    updateMotorControl();
}
