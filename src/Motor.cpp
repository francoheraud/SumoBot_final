#include "Motor.h"

volatile long encoderCountA = 0;
volatile long encoderCountB = 0;

int rMotNewA = 0;
int rMotNewB = 0;

void IRAM_ATTR handleEncoderA() {
    encoderCountA++;
}

void IRAM_ATTR handleEncoderB() {
    encoderCountB++;
}

void initEncoderA() {
    pinMode(ENCA, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCA), handleEncoderA, RISING);
}

void initEncoderB() {
    pinMode(ENCB, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCB), handleEncoderB, RISING);
}

long getEncoderCountA() { return encoderCountA; }
long getEncoderCountB() { return encoderCountB; }

void resetEncoders() {
    encoderCountA = 0;
    encoderCountB = 0;
}

void initMotors(void) {
    pinMode(IN1A, OUTPUT);
    pinMode(IN2A, OUTPUT);
    pinMode(IN1B, OUTPUT);
    pinMode(IN2B, OUTPUT);
    
    pinMode(PWMA, OUTPUT);

    ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWMA, PWM_CHANNEL_A);
    ledcAttachPin(PWMB, PWM_CHANNEL_B);

    stopMotors();
    initEncoderA();
    initEncoderB();
}


// according to my testing: I screwed up the polarity of the motor connections,
// 
void move(Motor_t *motor) {
    switch (motor->direction) {
        case FORWARD:
            digitalWrite(IN1A, HIGH); digitalWrite(IN2A, LOW);
            digitalWrite(IN1B, HIGH); digitalWrite(IN2B, LOW);
            // temporary
            motor->desiredSpeedA = maxTickSpeed * 0.5f;
            motor->desiredSpeedB = maxTickSpeed * 0.5f;
            break;

        case REVERSE:
            digitalWrite(IN1A, LOW); digitalWrite(IN2A, HIGH);
            digitalWrite(IN1B, LOW); digitalWrite(IN2B, HIGH);
            motor->desiredSpeedA = maxTickSpeed * 0.5f;
            motor->desiredSpeedB = maxTickSpeed * 0.5f;
            break;

        case RIGHT:
            digitalWrite(IN1A, HIGH); digitalWrite(IN2A, LOW);
            digitalWrite(IN1B, LOW);  digitalWrite(IN2B, LOW);
            motor->desiredSpeedA = maxTickSpeed;
            motor->desiredSpeedB = 0.0f;
            break;

        case LEFT:
            digitalWrite(IN1A, LOW);  digitalWrite(IN2A, LOW);
            digitalWrite(IN1B, HIGH); digitalWrite(IN2B, LOW);
            motor->desiredSpeedA = 0.0f;
            motor->desiredSpeedB = maxTickSpeed;
            break;

        case ROTATE_CW:
            digitalWrite(IN1A, HIGH); digitalWrite(IN2A, LOW);
            digitalWrite(IN1B, LOW);  digitalWrite(IN2B, HIGH);
            motor->desiredSpeedA = maxTickSpeed * 0.5f;
            motor->desiredSpeedB = maxTickSpeed * 0.5f;
            break;

        case ROTATE_CCW:
            digitalWrite(IN1A, LOW);  digitalWrite(IN2A, HIGH);
            digitalWrite(IN1B, HIGH); digitalWrite(IN2B, LOW);
            motor->desiredSpeedA = maxTickSpeed * 0.5f;
            motor->desiredSpeedB = maxTickSpeed * 0.5f;
            break;
    }
  
}

void stopMotors(void) {
    ledcWrite(PWM_CHANNEL_A, 0);
    ledcWrite(PWM_CHANNEL_B, 0);
    digitalWrite(IN1A, LOW);
    digitalWrite(IN2A, LOW);
    digitalWrite(IN1B, LOW);
    digitalWrite(IN2B, LOW);
}


void updatePIController(Motor_t *motor, float velA, float velB) {
    static long encOldA = 0, encOldB = 0;
    static int rOldA = 0, rOldB = 0;
    static int errOldA = 0, errOldB = 0;

    int errA = motor->desiredSpeedA - velA;
    int errB = motor->desiredSpeedB - velB;

    rMotNewA = rOldA + (int)(kp * (errA - errOldA)) + (int)(ki * ((errA + errOldA) / 2));
    rMotNewB = rOldB + (int)(kp * (errB - errOldB)) + (int)(ki * ((errB + errOldB) / 2));

    rMotNewA = constrain(rMotNewA, 0, 255);
    rMotNewB = constrain(rMotNewB, 0, 255);
    rOldA = constrain(rOldA, 0, 255);
    rOldB = constrain(rOldB, 0, 255);

    rOldA = rMotNewA;
    rOldB = rMotNewB;

    errOldA = errA;
    errOldB = errB;

    ledcWrite(PWM_CHANNEL_A, rMotNewA);
    ledcWrite(PWM_CHANNEL_B, rMotNewB);
} 


