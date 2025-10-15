// Sensors Demo
// 3 October 2025

#include <Arduino.h>
#include <TFT_eSPI.h>

int IR_ANALOG_PIN = 13;
int LEFT_TRIGGER_PIN = 18;
int LEFT_ECHO_PIN = 17;
int RIGHT_TRIGGER_PIN = 43;
int RIGHT_ECHO_PIN = 44;

const int UPDATE_DELAY_MILLIS = 250;
double distanceCm;
static char distancePrintout[30];

// Timeout (microseconds) determines what is "OUT OF RANGE" for the sensor
// From my testing, 15000 us timeout limits the distane range to ~200 cm
int pollUltrasonicSensor(int trigger, int echo, long timeout = 15000);

// Text colour settings (for aesthetic purposes only)
const static int maximumRangeCm = 100;
const static int longRangeCm = 60;
const static int mediumRangeCm = 40;
const static int shortRangeCm = 20;

const static uint32_t outOfRangeColour = TFT_DARKGREY;
const static uint32_t maximumRangeColour = TFT_WHITE;
const static uint32_t longRangeColour = TFT_GREEN;
const static uint32_t mediumRangeColour = TFT_GOLD;
const static uint32_t shortRangeColour = TFT_RED;

int lookup[] = {
    300, // 0000
    550, // 0001
    800, // 0010
    950, // 0011

    1200, // 0100
    1400, // 0101
    1750, // 0110
    2049, // 0111

    2050, // 1000
    2250, // 1001
    2500, // 1010
    2700, // 1011

    3000, // 1100
    3200, // 1101
    3600, // 1110
    4096 // 1111
};
int binarycode[4];

TFT_eSPI tft = TFT_eSPI();

void setup() {
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
    tft.init();

    // Setup pins
    pinMode(IR_ANALOG_PIN, INPUT);
    pinMode(LEFT_TRIGGER_PIN, OUTPUT);
    pinMode(LEFT_ECHO_PIN, INPUT);
    pinMode(RIGHT_TRIGGER_PIN, OUTPUT);
    pinMode(RIGHT_ECHO_PIN, INPUT);

    // Initialize TFT
    tft.setRotation(3); // Can be 1 or 3
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    //sprintf(buffer, "A0:%d L: %d,%d R: %d,%d", IR_ANALOG_PIN, LEFT_TRIGGER_PIN, LEFT_ECHO_PIN, RIGHT_TRIGGER_PIN, RIGHT_ECHO_PIN);
}

void loop() {
    static unsigned long lastUpdateTime = millis();
    if (millis() - lastUpdateTime > UPDATE_DELAY_MILLIS) {
        // Determine line values from analog pin (R-2R DAC)
        int digitalValue = digitalRead(IR_DIGITAL_PIN);
        int analogValue = analogRead(IR_ANALOG_PIN);
        static char buffer[50];
        static int encoding;
        for (int i = 0; i < 16; i++) {
            if (analogValue < lookup[i]) {
                encoding = i;
                break;
            }
        }
        tft.setTextColor(TFT_SILVER, TFT_BLACK);
        sprintf(buffer, "  ADC: %d -> %d ", analogValue, encoding);
        tft.setTextDatum(CC_DATUM);
        tft.drawString(buffer, 160, 105);
        tft.setTextDatum(TL_DATUM);

        for (int j = 0; j < 4; j++) {
            int remainder = encoding % 2;
            encoding /= 2;
            binarycode[j] = remainder;
        }
        for (int k = 3; k >= 0; k--) {
            if (binarycode[k] < 1) tft.setTextColor(TFT_WHITE, TFT_BLACK);
            else tft.setTextColor(0x2965, TFT_BLACK);
            tft.drawNumber(binarycode[k], 290-270*(k%2), 130-50*(k/2));
        }

        // Poll ultrasonic sensors
        pollUltrasonicSensor(LEFT_TRIGGER_PIN, LEFT_ECHO_PIN);
        int leftDistanceCm = distanceCm;
        tft.drawString(distancePrintout, 125, 15);

        pollUltrasonicSensor(RIGHT_TRIGGER_PIN, RIGHT_ECHO_PIN);
        int deviation = abs(distanceCm - leftDistanceCm);
        tft.drawString(distancePrintout, 125, 45);
        if (distanceCm > 0 && deviation < 3) {
            tft.setTextColor(TFT_GOLD, TFT_BLACK);
            tft.drawString("OBSTACLE", 20, 15);
            tft.drawString("DETECTED", 20, 45);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString("Left  : ", 20, 15);
            tft.drawString("Right : ", 20, 45);
        }
        
        lastUpdateTime = millis();
    }
}

int pollUltrasonicSensor(int trigger, int echo, long timeout) {
    static unsigned long durationMicroseconds;
    static unsigned long startTimeMicroseconds, elapsedTimeMicroseconds = 0;

    digitalWrite(trigger, LOW); // Write low to ensure a clean trigger pulse
    delayMicroseconds(2);
    digitalWrite(trigger, HIGH); // Trigger pulse must be high for minimum 10 us
    delayMicroseconds(10);
    digitalWrite(trigger, LOW);

    startTimeMicroseconds = micros();

    // Call pulseIn() to read the echo pulse duration, if timeout occurs duration is instead set to 0
    durationMicroseconds = pulseIn(echo, HIGH, timeout);

    if (durationMicroseconds > 0) {
        // Calculation: distance = v*t = (343 m/s) * (100 c/m) * (time in us) * (0.000001 s / us) / 2
        distanceCm = (durationMicroseconds * 0.0343) / 2;
        if (distanceCm < shortRangeCm) tft.setTextColor(shortRangeColour, TFT_BLACK);
        else if (distanceCm < mediumRangeCm) tft.setTextColor(mediumRangeColour, TFT_BLACK);
        else if (distanceCm < longRangeCm) tft.setTextColor(longRangeColour, TFT_BLACK);
        else if (distanceCm < maximumRangeCm) tft.setTextColor(maximumRangeColour, TFT_BLACK);
        else tft.setTextColor(outOfRangeColour, TFT_BLACK);
        snprintf(distancePrintout, 20, "%.2f cm     ", distanceCm);
    } else {
        tft.setTextColor(outOfRangeColour, TFT_BLACK);
        distanceCm = -1; // Note that non-positive values indicate no enemy found
        snprintf(distancePrintout, 20, "??? cm     ");
    }
    
    return elapsedTimeMicroseconds; // Return elapsed time
}
