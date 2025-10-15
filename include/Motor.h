#ifndef MOTOR_H
#define MOTOR_H
#include <Arduino.h>

// ===================== CONFIGURATION =====================
// Motor driver pin mappings



#define IN1A 1
#define IN2A 2
#define IN1B 3
#define IN2B 10
#define PWMA 11
#define PWMB 12

// Encoder pins
#define ENCA 13
#define ENCB 21

// LEDC PWM Configuration
#define PWM_CHANNEL_A 0
#define PWM_CHANNEL_B 1
#define PWM_FREQ 1000
#define PWM_RESOLUTION 8  // 8-bit (0-255)

// PI Controller tuning
constexpr float kp = 7.0f;
constexpr float ki = 2.0f;
constexpr float maxTickSpeed = 25.0f;  // Changed: ticks per 10ms (desired speed)
constexpr unsigned long PI_UPDATE_INTERVAL_MS = 500;  

enum Direction {
    FORWARD,
    REVERSE,
    LEFT,
    RIGHT,
    ROTATE_CW,
    ROTATE_CCW
};

typedef struct {
    float desiredSpeedA;
    float desiredSpeedB;

    int pwmA;
    int pwmB;

    int velA;
    int velB;

    Direction direction;
} Motor_t;

// ===================== GLOBAL VARIABLES =====================
extern volatile long encoderCountA;
extern volatile long encoderCountB;
extern int rMotNewA;
extern int rMotNewB;

// ===================== FUNCTION PROTOTYPES =====================
void initMotors(void);
void initEncoderA(void);
void initEncoderB(void);
void move(Motor_t *motor);
void stopMotors(void);
void updatePIController(Motor_t *motor, float velA, float velB);
long getEncoderCountA(void);
long getEncoderCountB(void);
void resetEncoders(void);
void IRAM_ATTR handleEncoderA(void);
void IRAM_ATTR handleEncoderB(void);

#endif // MOTOR_H