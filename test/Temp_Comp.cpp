#include "Motor.h"
#include "Sensors.h"
#include "Startup.h"

/**
 * Sumo Bot Competition Code
 * By Franco Heraud and ya boy Chat
 */

TFT_eSPI tft = TFT_eSPI();

Sensors_t sensor;
Motor_t motor;

#define BUF_SIZE                    10
#define DETECTION_THRESHOLD         30
#define TRACK_OPPONENT_THRESHOLD    30
#define EDGE_BACKUP_TIME            300 // ms

bool startupSequence = true, opponentDetected = false, startupTurning = true;

const int detectionThreshold = 30; // obv change if needed
const int trackOpponentThreshold = 40;
int distanceBuf[BUF_SIZE] = {0};
int bufIdx = 0;


int avePrevFutValue, currIdx, prevIdx, futIdx;
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
    Serial.begin(115200);
    pinMode(0, INPUT);
    pinMode(14, INPUT);
    Serial.println("pin setup done");
    initMotors();
    Serial.println("init motors done");
    initSensors();
    Serial.println("init sensors done");
    
    tft.init();
    tft.setTextSize(1);
	tft.setRotation(3);
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
    userSelectFunction(&tft, &sensor);
    motor.direction = ROTATE_CCW;
}


static int getAverageDistance() {
    int sum;
    for (int i = 0; i < BUF_SIZE; i++) 
        sum += distanceBuf[i];
    return sum / BUF_SIZE;   
}


static void updateDistanceBuf(int distance) {
    distanceBuf[bufIdx] = distance;
    bufIdx = (bufIdx + 1) % BUF_SIZE;
    if (bufIdx == 0) bufferFilled = true;
}

static bool detectOpponent() {
    if (!bufferFilled) return false;
    int aveDistance = getAverageDistance();
    int currDistance = distanceBuf[(bufIdx - 1 + BUF_SIZE) % BUF_SIZE];
    return (aveDistance - currDistance) > DETECTION_THRESHOLD;
}

static bool edgeDetected() {
    return sensor.frontLeft || sensor.frontRight || 
    sensor.rearLeft || sensor.rearRight;
}


static Direction getEdgeAvoidDirection() {
    if (sensor.frontLeft || sensor.frontRight) return REVERSE;
    if (sensor.rearLeft || sensor.rearRight) return FORWARD;
    if (sensor.frontLeft || sensor.rearLeft) return ROTATE_CW; 
    else return ROTATE_CCW;  
}


// velocity updates arent in sync? test plz!
// TODO: update to PID?
static void updateMotorControl() {
    static float velA = 0.0f, velB = 0.0f;
    static unsigned long elapsedMs;
    if (elapsedMs = millis() - lastPIUpdate < PI_UPDATE_INTERVAL_MS) {
        return;
    }
    
    static int encoderCountOldA = 0;
    static int encoderCountOldB = 0;
    
    velA = 1000.0f * (encoderCountA - encoderCountOldA) / elapsedMs;
    velB = 1000.0f * (encoderCountB - encoderCountOldB) / elapsedMs;
    
    updatePIController(&motor, velA, velB);
    
    encoderCountOldA = encoderCountA;
    encoderCountOldB = encoderCountB;
    lastPIUpdate = millis();
}


static void chaseMode() {
    bool opponentLeft = (sensor.leftCm < sensor.rightCm - TRACK_OPPONENT_THRESHOLD);
    bool opponentRight = (sensor.rightCm < sensor.leftCm - TRACK_OPPONENT_THRESHOLD);

    if (opponentLeft) motor.direction = LEFT;
    if (opponentRight) motor.direction = RIGHT;
    else motor.direction = FORWARD;
    move(&motor);
}


void loop() {
    pollDistance(&sensor);
    detectLine(&sensor);
    
    int avgDistance = (sensor.leftCm + sensor.rightCm) / 2;
    updateDistanceBuf(avgDistance);
    
    switch (currentState) {
        case SEARCHING:
            motor.direction = ROTATE_CCW;
            
            if (detectOpponent()) {
                currentState = CHASING;
            }
            
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
            

            if (sensor.leftCm == OUT_OF_RANGE && sensor.rightCm == OUT_OF_RANGE) {
                currentState = SEARCHING;
            }
            break;
            
        case AVOIDING_EDGE:
            motor.direction = edgeAvoidDirection;
            
            if (millis() - edgeAvoidStartTime > EDGE_BACKUP_TIME) {
                currentState = SEARCHING;  
            }
            break;
    }
    
    move(&motor);
    updateMotorControl();
}
