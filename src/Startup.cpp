// Startup code (general user UI)
// Developer: Allan Wu (23810308)
// Rework: Franco Heraud (23602168)
#include "Startup.h"
#include "Sensors.h"
#include "Motor.h"

char robotModeDescriptions[6][BUFFER_CHARS] = {
  "0. SENSORS DEMO",
  "1. MOTORS DEMO",
  "2. START COMPETITION",
  "3. PRINT ADC LOOKUP TABLE",
  "4. CALIBRATE LINE DETECTOR ADC",
  "5. RESET ALL SETTINGS",
};

menuOption currentMenu;

void userSelectFunction(TFT_eSPI *tft, Sensors_t *s, Motor_t *mot)
{
  int prevLeft = 0, prevRight = 0;
  int currLeft = 0, currRight = 0;
  int prevChoice = -1, currChoice = 0;
  bool startOperation = false;
  unsigned long lastUpdateTime = millis();
  float delaySeconds = 9.9;
  tft->setTextFont(2);

  while (!startOperation) {
    tft->setCursor(MENU_X_DATUM, MENU_Y_DATUM-5);
    tft->printf("[^] START ROUTINE   [v] SCROLL OPTIONS");
    currLeft = !digitalRead(LEFT_BUTTON);
    currRight = !digitalRead(RIGHT_BUTTON);

    if (prevLeft && !currLeft) {
      tft->setTextFont(0);
      startOperation = true;
    } else if (prevRight && !currRight) {
      currChoice = (currChoice + 1) % 6; // Must update this whenever we add/remove robotModes
      lastUpdateTime = millis();
    }

    if (prevChoice != currChoice) {
      tft->setTextColor(LOW_EMPHASIS_COLOUR, BACKGROUND_COLOUR);

      for (int i = 0; i < 6; i++) {
        tft->drawString(robotModeDescriptions[i], MENU_X_DATUM, MENU_Y_DATUM+20*(i+1));
      }

      tft->setTextColor(HIGH_EMPHASIS_COLOUR, BACKGROUND_COLOUR);
      tft->drawString(robotModeDescriptions[currChoice], MENU_X_DATUM, MENU_Y_DATUM+20*(currChoice+1));
    }

    tft->setTextColor(PRIMARY_TEXT_COLOUR, BACKGROUND_COLOUR);
    currentMenu = (menuOption)currChoice;
    prevChoice = currChoice;

    prevLeft = currLeft;
    prevRight = currRight;
  }

  tft->setTextFont(0);
  tft->fillScreen(TFT_BLACK);

  switch (currentMenu) {
    case (CALIBRATE):
      recalibrateADC_GUI(tft);
      userSelectFunction(tft, s, mot);
      break;
    case (RESET):
      resetADCLookup(tft);
      userSelectFunction(tft, s, mot);
      break;
    case (MOTORS):
      break;
    case (SENSORS):
      sensorsDemo(tft, s);
      break;
    case (PRINT):
      printADCLookup(tft, TFT_SILVER);
      userSelectFunction(tft, s, mot);
      break;
    case (COMPETITION):
      competitionCountdownTimer(tft, 3);
      break;
  }
}

void competitionCountdownTimer(TFT_eSPI *tft, int seconds)
{
  tft->setTextSize(7);
  tft->setTextFont(2);
  tft->setTextDatum(CC_DATUM);
  while (seconds > 0) {
    tft->drawNumber(seconds, 170, 85);
    seconds -= 1;
    delay(1000);
  }
  tft->drawString("GO", 170, 85);
  delay(500);
  tft->setTextSize(3);
  tft->setTextDatum(TL_DATUM);
  tft->fillScreen(TFT_BLACK);
}