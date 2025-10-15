#include "Encoder.h"

static volatile long encoderCountA = 0;
static volatile long encoderCountB = 0;

void IRAM_ATTR handleEncoderA() { 
    encoderCountA++;
}


void IRAM_ATTR handleEncoderB() { 
    encoderCountB++;
}

void initEncoderA(int pin) {
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin), handleEncoderA, RISING);
}

void initEncoderB(int pin) {
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin), handleEncoderB, RISING);
}

long getEncoderCountA() { return encoderCountA; }
long getEncoderCountB() { return encoderCountB; }
void resetEncoders() { encoderCountA = 0; encoderCountB = 0; }
