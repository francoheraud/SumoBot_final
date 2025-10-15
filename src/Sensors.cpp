// Combined line detection and ultrasonic sensor code
// Author: Allan Wu (23810308)

#include "Sensors.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "Startup.h"

Preferences botSettings; 

// Convert between analog voltage reading and binary codes 0000 to 1111
// (frontLeft, frontRight, rearLeft, rearRight): 0 = WHITE, 1 = BLACK
int ADCLookup[16];

int ADCLookupDefaults[16] = {
    1384, // 0000
    1523, // 0001
    1672, // 0010
    1821, // 0011

    1960, // 0100
    2068, // 0101
    2148, // 0110
    2336, // 0111

    2559, // 1000
    2708, // 1001
    2863, // 1010
    3023, // 1011

    3191, // 1100
    3382, // 1101
    3597, // 1110
    4096, // 1111
};

const char *ADCStrings[16] = {
    "0000 NO_LINE ", // 0000
    "0001 R_RIGHT ", // 0001
    "0010 R_LEFT  ", // 0010
    "0011 _REAR_  ", // 0011

    "0100 F_RIGHT ", // 0100
    "0101 _RIGHT_ ", // 0101
    "0110 INVALID1", // 0110
    "0111 R_RIGHT2", // 0111

    "1000 F_LEFT  ", // 1000
    "1001 INVALID2", // 1001
    "1010 _LEFT_  ", // 1010
    "1011 R_LEFT_2", // 1011

    "1100 _FRONT_ ", // 1100
    "1101 F_RIGHT2", // 1101
    "1110 F_LEFT_2", // 1110
    "1111 OUTSIDE ", // 1111
};

void initSensors() // Please note that the Line Detector pin must support ADC
{
    pinMode(LEFT_TRIGGER, OUTPUT);
    pinMode(LEFT_ECHO, INPUT);
    pinMode(RIGHT_TRIGGER, OUTPUT);
    pinMode(RIGHT_ECHO, INPUT);
    pinMode(LINEDETECTOR_DAC, INPUT);

	botSettings.begin("botSettings", false);
    for (int i = 0; i < 16; i++) {
	    if (!botSettings.isKey(ADCStrings[i])) {
            ADCLookup[i] = ADCLookupDefaults[i];
		    botSettings.putInt(ADCStrings[i], ADCLookup[i]);
        } else {
		    ADCLookup[i] = botSettings.getInt(ADCStrings[i]);
        }
    }
}

void waitForButtonPress()
{
    int currLeft, currRight, prevLeft, prevRight;
    while (true) {
        currLeft = !digitalRead(LEFT_BUTTON);
        currRight = !digitalRead(RIGHT_BUTTON);
        if (prevLeft && !currLeft || prevRight && !currRight) break;
        prevLeft = currLeft;
        prevRight = currRight;
    delay(100);
    }
}

// Display ADCLookup values
void printADCLookup(TFT_eSPI *tft, uint32_t colour)
{
    tft->setTextSize(1);
    tft->setTextColor(colour, TFT_BLACK);
    for (int i = 0; i < 16; i++) {
        tft->setCursor(0, i*10);
        tft->printf("%s: ADCLookup[%2d] = %d ", ADCStrings[i], i, ADCLookup[i]);
        delay(100);
    };
    waitForButtonPress();
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->fillScreen(TFT_BLACK);
}

// Resets ADC lookup table and flash memory to hardcoded values
void resetADCLookup(TFT_eSPI *tft)
{
    for (int i = 0; i < 16; i++) {
        ADCLookup[i] = ADCLookupDefaults[i];
        botSettings.putInt(ADCStrings[i], ADCLookup[i]);
    }

    printADCLookup(tft, TFT_RED);
}

void recalibrateADC_GUI(TFT_eSPI *tft)
{
    int calibrationStage = 0;
    int currentReading = 0;
    int prevLeft = 0, prevRight = 0, currLeft = 0, currRight = 0;
    int analogReadings[16] = {0};

    tft->setTextSize(2);

    while (calibrationStage < 16) {
        currLeft = !digitalRead(LEFT_BUTTON);
        currRight = !digitalRead(RIGHT_BUTTON);
        currentReading = analogRead(LINEDETECTOR_DAC);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);

        tft->setCursor(5,10);
        tft->printf("Calibrating ADC (%d/15)", calibrationStage);

        tft->setCursor(5,40);
        tft->printf("State: %s      ", ADCStrings[calibrationStage]);

        if (calibrationStage == 6 || calibrationStage == 9) {
            tft->setCursor(5,85);
            tft->printf("Analog reading: N/A    ");
            tft->setCursor(5,145);
            tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft->printf("Press any button...       ");
        } else {
            tft->setCursor(5,85);
            tft->printf("Analog reading: %d    ", currentReading);
            tft->setCursor(5,145);
            tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft->printf("Press any button to record");
        }

        if (!prevLeft && currLeft || !prevRight && currRight) {
            if (calibrationStage == 6 || calibrationStage == 9) {
                analogReadings[calibrationStage] = analogReadings[calibrationStage-1];
                delay(100);
            } else {
                analogReadings[calibrationStage] = currentReading;
                tft->setTextColor(TFT_GREEN, TFT_BLACK);
                tft->setCursor(5,85);
                tft->printf("Recorded value: %d      ", currentReading);
                delay(400);
                tft->setTextColor(TFT_WHITE, TFT_BLACK);
            }
            calibrationStage++;
        }

        prevLeft = currLeft;
        prevRight = currRight;
        delay(100);
    }

    analogReadings[6] = (analogReadings[5] + analogReadings[7])/2;
    analogReadings[9] = (analogReadings[8] + analogReadings[10])/2;

    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
    tft->setTextSize(1);
    int curr, next;
    for (int i = 0; i < 16; i++) {
        curr = analogReadings[i];
        next = (i+1 < 16) ? analogReadings[i+1] : 4096;
        ADCLookup[i] = (curr + next)/2;
		botSettings.putInt(ADCStrings[i], ADCLookup[i]);
        tft->setCursor(0, i*10);
    };
    printADCLookup(tft, TFT_GREEN);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
	tft->fillScreen(TFT_BLACK);
}

void detectLine(Sensors_t *sensors)
{
    int analogValue = analogRead(LINEDETECTOR_DAC);
	sensors->analogReading = analogValue;
	int *ptrs[4] = {&sensors->rearRight, &sensors->rearLeft, &sensors->frontRight, &sensors->frontLeft};
    int encoding = 16; // Defaults to '1111'
    for (int i = 0; i < 16; i++) {
        if (analogValue < ADCLookup[i]) {
            encoding = i;
            break;
        }
    }
    for (int j = 0; j < 4; j++) {
        int remainder = encoding % 2;
        encoding /= 2;
        *ptrs[j] = remainder;
    }
}

void pollDistance(Sensors_t *sensors)
{
    static int currSensor = LEFT_ULTRASONIC; // LEFT = 0, RIGHT = 1
	double ultrasonicDistanceCm;
	unsigned long durationMicroseconds;
	int echoPin, triggerPin;
	int *sensorPtr;

	switch (currSensor) {
        case LEFT_ULTRASONIC:
            echoPin = LEFT_ECHO;
            triggerPin = LEFT_TRIGGER;
            sensorPtr = &sensors->leftCm;
            break;
        case RIGHT_ULTRASONIC:
            echoPin = RIGHT_ECHO;
            triggerPin = RIGHT_TRIGGER;
            sensorPtr = &sensors->rightCm;
	}

	// Send trigger pulse
    digitalWrite(triggerPin, LOW); 
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH); 
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    // Call pulseIn() to read the echo pulse duration, if timeout occurs duration is instead set to 0
    durationMicroseconds = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT_US);

    if (durationMicroseconds > 0) {
        // Calculation: distance = v*t = (343 m/s) * (100 c/m) * (time in us) * (0.000001 s / us) / 2
        ultrasonicDistanceCm = (durationMicroseconds * 0.0343) / 2;
		// Round to nearest centimetre
    	*sensorPtr = (int)(ultrasonicDistanceCm + 0.5); 
    } else {
        *sensorPtr = OUT_OF_RANGE;
    }
    // Alternate between the left and right ultrasonic sensor
    currSensor = !currSensor;
    return;
}

void sensorsDemo(TFT_eSPI *tft, Sensors_t *sensors)
{
    tft->setTextSize(2);
    static char buffer[50];
    static char distancePrintout[30];

    const static int maximumRangeCm = 100;
    const static int longRangeCm = 60;
    const static int mediumRangeCm = 40;
    const static int shortRangeCm = 20;

    const static uint32_t outOfRangeColour = TFT_DARKGREY;
    const static uint32_t maximumRangeColour = TFT_WHITE;
    const static uint32_t longRangeColour = TFT_GREEN;
    const static uint32_t mediumRangeColour = TFT_GOLD;
    const static uint32_t shortRangeColour = TFT_RED;
    static unsigned long lastUpdateTime = millis();

    while (true) {
        // Determine line values from analog pin (R-2R DAC)
        pollDistance(sensors);
        detectLine(sensors);
        tft->setTextColor(TFT_SILVER, TFT_BLACK);
        int encoding = (sensors->frontLeft << 3)+(sensors->frontRight << 2)+(sensors->rearLeft << 1)+(sensors->rearRight);
        sprintf(buffer, "  ADC: %d -> %d ", sensors->analogReading, encoding);
        tft->setTextDatum(CC_DATUM);
        tft->drawString(buffer, 160, 105);
        tft->setTextDatum(TL_DATUM);

        int *binaryPtrs[4] = {&sensors->rearRight, &sensors->rearLeft, &sensors->frontRight, &sensors->frontLeft};

        for (int k = 3; k >= 0; k--) {
            if (*binaryPtrs[k]) tft->setTextColor(TFT_RED, TFT_BLACK);
            else tft->setTextColor(TFT_WHITE, TFT_BLACK);
            tft->drawNumber(*binaryPtrs[k], 290-270*(k%2), 130-50*(k/2));
        }

        if (sensors->leftCm > 0) {
            if (sensors->leftCm < shortRangeCm) tft->setTextColor(shortRangeColour, TFT_BLACK);
            else if (sensors->leftCm < mediumRangeCm) tft->setTextColor(mediumRangeColour, TFT_BLACK);
            else if (sensors->leftCm < longRangeCm) tft->setTextColor(longRangeColour, TFT_BLACK);
            else if (sensors->leftCm < maximumRangeCm) tft->setTextColor(maximumRangeColour, TFT_BLACK);
            else tft->setTextColor(outOfRangeColour, TFT_BLACK);
            snprintf(distancePrintout, 20, "%d cm     ", sensors->leftCm);
        } else {
            tft->setTextColor(outOfRangeColour, TFT_BLACK);
            snprintf(distancePrintout, 20, "??? cm     ");
        }
        tft->drawString(distancePrintout, 125, 15);

        pollDistance(sensors);

        if (sensors->rightCm > 0) {
            if (sensors->rightCm < shortRangeCm) tft->setTextColor(shortRangeColour, TFT_BLACK);
            else if (sensors->rightCm < mediumRangeCm) tft->setTextColor(mediumRangeColour, TFT_BLACK);
            else if (sensors->rightCm < longRangeCm) tft->setTextColor(longRangeColour, TFT_BLACK);
            else if (sensors->rightCm < maximumRangeCm) tft->setTextColor(maximumRangeColour, TFT_BLACK);
            else tft->setTextColor(outOfRangeColour, TFT_BLACK);
            snprintf(distancePrintout, 20, "%d cm     ", sensors->rightCm);
        } else {
            tft->setTextColor(outOfRangeColour, TFT_BLACK);
            snprintf(distancePrintout, 20, "??? cm     ");
        }
        tft->drawString(distancePrintout, 125, 45);

        int deviation = abs(sensors->leftCm - sensors->rightCm);

        if (sensors->leftCm > 0 && sensors->rightCm > 0 && deviation < 5) {
            tft->setTextColor(TFT_GOLD, TFT_BLACK);
            tft->drawString("OBSTACLE", 20, 15);
            tft->drawString("DETECTED", 20, 45);
        } else {
            tft->setTextColor(TFT_WHITE, TFT_BLACK);
            tft->drawString("Left  : ", 20, 15);
            tft->drawString("Right : ", 20, 45);
        }
        delay(250);
    }
}