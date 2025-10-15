#ifndef SENSORS_H
#define SENSORS_H
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>

// Pins may need to be changed
#define LEFT_TRIGGER			18
#define LEFT_ECHO				17
#define RIGHT_TRIGGER			43
#define RIGHT_ECHO				44
#define LINEDETECTOR_DAC		13

#define LEFT_ULTRASONIC 	0
#define RIGHT_ULTRASONIC 	1
#define OUT_OF_RANGE 		-1

// Set the maximum time we will wait for the echo pulse: this determines what is "OUT OF RANGE" for the sensor!
// From my testing, 15000 us timeout limits the distance range to ~200 cm
#define ULTRASONIC_TIMEOUT_US	15000

extern Preferences botSettings; 

// Lookup table
extern int ADCLookup[16];
extern int ADCLookupDefaults[16];

// String keys
extern const char *ADCStrings[16];

typedef struct {
	// Ultrasonic sensor distances stored to the nearest cm, -1 is OUT_OF_RANGE
	int leftCm;
	int rightCm;

	// Line detector booleans: 0 = WHITE, 1 = BLACK 
	// 1 (BLACK) indicates a corner has gone over the line
    int analogReading;
    int frontLeft;
    int frontRight;
    int rearLeft;
    int rearRight;
} Sensors_t;

// Setup pins and preferences (saved to flash memory).
void initSensors();

// Useful function for GUI
void waitForButtonPress();

// Print ADC lookup table values to TFT
void printADCLookup(TFT_eSPI *tft, uint32_t colour);

// Reset ADC lookup table to hardcoded values
void resetADCLookup(TFT_eSPI *tft);

// Initiates a 16-step recalibration of the ADC lookup table
void recalibrateADC_GUI(TFT_eSPI *tft);

// Run in an infinite loop
void sensorsDemo(TFT_eSPI *tft, Sensors_t *sensors);

/**
 * \brief	    Updates all line detector booleans using DAC.
 * \param       sensors Pointer to Sensors_t struct.
 */
void detectLine(Sensors_t *sensors);

/**
 * \brief	    Poll distance in cm, alternating between left and right sensors.
 * \param       sensors Pointer to Sensors_t struct.
 */
// pollDistance(sensors) to get the distance in cm
// Alternates between updating leftCm and rightCm each call
void pollDistance(Sensors_t *sensors);

#endif
