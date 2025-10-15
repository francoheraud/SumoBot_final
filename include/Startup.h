#ifndef STARTUP_H
#define STARTUP_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Sensors.h"
#include "Motor.h"

#define MENU_X_DATUM    20
#define MENU_Y_DATUM    20
#define BUFFER_CHARS    50

#define LEFT_BUTTON     0
#define RIGHT_BUTTON    14

#define PRIMARY_TEXT_COLOUR     TFT_WHITE
#define HIGH_EMPHASIS_COLOUR    TFT_GOLD
#define LOW_EMPHASIS_COLOUR     0x2965
#define BACKGROUND_COLOUR       TFT_BLACK

enum menuOption {
  SENSORS,
  MOTORS,
  COMPETITION,
  PRINT,
  CALIBRATE,
  RESET,
};

void userSelectFunction(TFT_eSPI *tft, Sensors_t *s, Motor_t *m);

// Delays main loop by (seconds+0.5) seconds
void competitionCountdownTimer(TFT_eSPI *tft, int seconds = 3);

#endif