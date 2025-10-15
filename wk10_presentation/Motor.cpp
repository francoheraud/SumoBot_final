#include "Motor.h"
#include "Encoder.h"

// Motor pins
static int IN1A, IN2A, PWMA;
static int IN1B, IN2B, PWMB;

// PID control variables
static long lastCountA = 0, lastCountB = 0;
static int targetSpeed = 0; // ticks/sec
static Direction currentDir = FORWARD;

// Convert speed level to target ticks/sec
static int speedToTargetTicks(SpeedLevel s) { // speed to analogwrite duty cycle
    switch(s) {
        case MIN: return 100;
        case MEDIUM: return 180;
        case MAX: return 255;
    }
    return 0;
}

void initMotors(int in1A, int in2A, int pwmA, int encA,
                int in1B, int in2B, int pwmB, int encB) {
    IN1A = in1A; IN2A = in2A; PWMA = pwmA;
    IN1B = in1B; IN2B = in2B; PWMB = pwmB;

    pinMode(IN1A, OUTPUT); pinMode(IN2A, OUTPUT); pinMode(PWMA, OUTPUT);
    pinMode(IN1B, OUTPUT); pinMode(IN2B, OUTPUT); pinMode(PWMB, OUTPUT);

    initEncoderA(encA);
    initEncoderB(encB);
}

void move(Direction dir, SpeedLevel speed) {
    currentDir = dir;
    targetSpeed = speedToTargetTicks(speed);

    switch(dir) {
        case FORWARD:
            digitalWrite(IN1A,HIGH); digitalWrite(IN2A,LOW);
            digitalWrite(IN1B,HIGH); digitalWrite(IN2B,LOW);
            break;
        case REVERSE:
            digitalWrite(IN1A,LOW); digitalWrite(IN2A,HIGH);
            digitalWrite(IN1B,LOW); digitalWrite(IN2B,HIGH);
            break;
        case RIGHT: // pivot right
            digitalWrite(IN1A,LOW); digitalWrite(IN2A,LOW);
            digitalWrite(IN1B,HIGH); digitalWrite(IN2B,LOW);
            break;
        case LEFT:  // pivot left
            digitalWrite(IN1A,HIGH); digitalWrite(IN2A,LOW);
            digitalWrite(IN1B,LOW); digitalWrite(IN2B,LOW);
            break;
        case ROTATE_CW:
            digitalWrite(IN1A,LOW); digitalWrite(IN2A,HIGH);
            digitalWrite(IN1B,HIGH); digitalWrite(IN2B,LOW);
            break;
        case ROTATE_CCW:
            digitalWrite(IN1A,HIGH); digitalWrite(IN2A,LOW);
            digitalWrite(IN1B,LOW); digitalWrite(IN2B,HIGH);
            break;
    }
}

void stopMotors() {
    analogWrite(PWMA,0);
    analogWrite(PWMB,0);
}

void updateMotors() { // bang-bang controller
    static unsigned long lastTime = millis();
    unsigned long dt = (millis() - lastTime); //milliseconds 
    if (dt < 100) return; // update every 50ms

    long countA = getEncoderCountA();
    long countB = getEncoderCountB();

    long deltaA = countA - lastCountA;
    long deltaB = countB - lastCountB;

    lastCountA = countA;
    lastCountB = countB;

    float actualSpeedA = deltaA / dt;
    float actualSpeedB = deltaB / dt;

    /*
    int pwmA = (actualSpeedA < targetSpeed) ? 255 : 0; // on and off valyes
    int pwmB = (actualSpeedB < targetSpeed) ? 255 : 0;
    */
    int pwmA = targetSpeed;
    int pwmB = targetSpeed;

    analogWrite(PWMA, pwmA);
    analogWrite(PWMB, pwmB);
    lastTime = millis();
}
