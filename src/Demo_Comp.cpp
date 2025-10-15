#include "Motor.h"
#include "Sensors.h"
#include "Startup.h"

TFT_eSPI tft = TFT_eSPI();

Sensors_t sensor;
Motor_t motor;

#define BUF_SIZE                    8
#define DETECTION_THRESHOLD         30   // INCREASED - needs bigger drop
#define TRACK_OPPONENT_THRESHOLD    15    // Slightly higher
#define EDGE_BACKUP_TIME            300
#define WARMUP_TIME_MS              3000  // Longer warmup
#define BASELINE_SAMPLES            20    // Samples for baseline measurement

int distanceBuf[BUF_SIZE] = {0};
int bufIdx = 0;
bool bufferFilled = false;
int baselineDistance = 1000;  // Will be measured

enum RobotState {
    SEARCHING,
    CHASING
};

RobotState currentState = SEARCHING;
unsigned long lastPIUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long warmupStart = 0;
bool warmedUp = false;
bool baselineMeasured = false;

static int detectConfirmCount = 0;
static const int DETECT_REQUIRED = 6;  // MORE confirmations needed
static int lostConfirmCount = 0;
static const int LOST_REQUIRED = 8;    // Even more to lose

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

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("SUMO BOT TEST MODE");
    tft.println("------------------");
    tft.println("Measuring baseline...");
    
    // Measure baseline distance in open space
    delay(500);  // Let sensors settle
    int sum = 0;
    int validSamples = 0;
    for (int i = 0; i < BASELINE_SAMPLES; i++) {
        pollDistance(&sensor);
        delay(50);
        // Use RAW values, don't normalize yet
        int left = (sensor.leftCm > 0) ? sensor.leftCm : 0;
        int right = (sensor.rightCm > 0) ? sensor.rightCm : 0;
        
        // Only use valid readings for baseline
        if (left > 0 && right > 0) {
            int avg = (left + right) / 2;
            sum += avg;
            validSamples++;
            tft.printf("Sample %d: %d cm\n", validSamples, avg);
        }
        delay(100);
    }
    
    // Calculate baseline from valid samples only
    if (validSamples > 0) {
        baselineDistance = sum / validSamples;
    } else {
        baselineDistance = 200;  // Fallback if no valid readings
        tft.println("WARNING: Using default baseline");
    }
    
    // DON'T pre-fill buffer - let it fill naturally during warmup
    // This ensures we're comparing against REAL environment data
    for (int i = 0; i < BUF_SIZE; ++i) distanceBuf[i] = 0;
    bufferFilled = false;
    bufIdx = 0;
    
    warmupStart = millis();
    lastPIUpdate = millis();
    
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.printf("Baseline: %d cm\n", baselineDistance);
    tft.println("Warmup 3s...");
    tft.setTextSize(2);
}

static inline int normaliseDistanceForBuffer(int d) {
    return (d <= 0) ? baselineDistance : d;  // Use measured baseline for invalid readings
}

static int getAverageDistance() {
    long sum = 0;
    for (int i = 0; i < BUF_SIZE; i++) sum += distanceBuf[i];
    return (int)(sum / BUF_SIZE);
}

static void updateDistanceBuf(int distance) {
    // Always update the buffer - continuous tracking is essential
    int d = normaliseDistanceForBuffer(distance);
    distanceBuf[bufIdx] = d;
    bufIdx = (bufIdx + 1) % BUF_SIZE;
    
    // Mark buffer as filled after one complete cycle
    if (bufIdx == 0 && !bufferFilled) {
        bufferFilled = true;
    }
}

static bool detectOpponentDebounced() {
    // Must have warmed up AND buffer must be filled with real data
    if (!warmedUp || !baselineMeasured || !bufferFilled) return false;
    
    // Extra buffer cycles to ensure stable readings
    static int loopsSinceWarmup = 0;
    if (loopsSinceWarmup < BUF_SIZE * 2) {
        loopsSinceWarmup++;
        return false;
    }
    
    int aveDistance = getAverageDistance();
    int currIdx = (bufIdx - 1 + BUF_SIZE) % BUF_SIZE;
    int currDistance = distanceBuf[currIdx];
    
    // Detect if current reading shows something MUCH closer than baseline average
    bool bigDrop = (aveDistance - currDistance) > DETECTION_THRESHOLD;
    bool veryClose = currDistance < TRACK_OPPONENT_THRESHOLD;
    
    if (bigDrop || veryClose) {
        detectConfirmCount++;
        lostConfirmCount = 0;
    } else {
        // Decay instead of hard reset for more stable detection
        detectConfirmCount = max(0, detectConfirmCount - 1);
    }

    return (detectConfirmCount >= DETECT_REQUIRED);
}

static void noteLostOpponent() {
    if (!warmedUp) return;
    
    int aveDistance = getAverageDistance();
    int currIdx = (bufIdx - 1 + BUF_SIZE) % BUF_SIZE;
    int currDistance = distanceBuf[currIdx];
    
    bool raw = (currDistance < TRACK_OPPONENT_THRESHOLD) ||
               ((aveDistance - currDistance) > DETECTION_THRESHOLD);

    if (!raw) {
        lostConfirmCount++;
    } else {
        lostConfirmCount = 0;
    }
}

static void updateMotorControl() {
    static int encoderCountOldA = 0;
    static int encoderCountOldB = 0;
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

    if (opponentLeft) motor.direction = LEFT;
    else if (opponentRight) motor.direction = RIGHT;
    else motor.direction = FORWARD;

    move(&motor);
}

static void updateDisplay(int left, int right, int avg, bool isWarmup) {
    unsigned long now = millis();
    if (now - lastDisplayUpdate < 100) return;
    lastDisplayUpdate = now;

    tft.fillRect(0, 40, 240, 200, TFT_BLACK);
    tft.setCursor(0, 40);
    tft.printf("Left : %4d cm\n", left);
    tft.printf("Right: %4d cm\n", right);
    tft.printf("Avg  : %4d cm\n", avg);
    tft.printf("Base : %4d cm\n", baselineDistance);
    tft.printf("BufAvg: %4d cm\n", getAverageDistance());
    tft.printf("BufFull: %s\n", bufferFilled ? "YES" : "NO");
    
    if (!isWarmup && warmedUp) {
        if (currentState == SEARCHING) {
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.println("State: SEARCHING");
        } else {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.println("State: CHASING");
        }
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.printf("Detect:%d/%d\n", detectConfirmCount, DETECT_REQUIRED);
    } else {
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        float elapsed = (millis() - warmupStart) / 1000.0f;
        tft.printf("WARMING UP: %.1fs\n", elapsed);
    }

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void loop() {
    // Check warmup completion
    if (!warmedUp && (millis() - warmupStart > WARMUP_TIME_MS)) {
        warmedUp = true;
        baselineMeasured = true;
        detectConfirmCount = 0;
        lostConfirmCount = 0;
    }

    // Always poll sensors
    pollDistance(&sensor);
    delay(5);
    pollDistance(&sensor);

    int leftForBuf = normaliseDistanceForBuffer(sensor.leftCm);
    int rightForBuf = normaliseDistanceForBuffer(sensor.rightCm);
    int avgDistanceForBuf = (leftForBuf + rightForBuf) / 2;
    
    // Buffer continuously updates - this is critical for tracking
    updateDistanceBuf(avgDistanceForBuf);

    switch (currentState) {
        case SEARCHING:
            motor.direction = ROTATE_CCW;
            move(&motor);
            
            // Only detect after full warmup
            if (warmedUp && baselineMeasured && detectOpponentDebounced()) {
                currentState = CHASING;
                detectConfirmCount = 0;
                lostConfirmCount = 0;
            }
            break;

        case CHASING:
            chaseMode();
            if (warmedUp) noteLostOpponent();
            if (lostConfirmCount >= LOST_REQUIRED) {
                currentState = SEARCHING;
                lostConfirmCount = 0;
            }
            break;
    }

    updateMotorControl();
    updateDisplay(sensor.leftCm, sensor.rightCm, avgDistanceForBuf, !warmedUp);
}