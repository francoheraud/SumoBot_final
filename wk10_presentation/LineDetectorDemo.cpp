// Line detector demo with R-2R resistor ladder DAC
// Allan Wu (23810308)
// 30 September 2025

#include <Arduino.h>
#include <TFT_eSPI.h>

// These are default values, they may be overriden using the setup function
int IR_DIGITAL_PIN = 2;
int IR_ANALOG_PIN = 1;

const int IR_DELAY_MILLIS = 500;

int lookup[16] = {
    299,
    499,
    699,
    799,
    999,
    1249,
    1499,
    1619,
    1799,
    1999,
    2299,
    2499,
    2749,
    2999,
    3399,
    4095
};
int binarycode[4];

TFT_eSPI tft = TFT_eSPI();

void setup() {
    tft.init();

    // Setup pins
    pinMode(IR_DIGITAL_PIN, INPUT);
    pinMode(IR_ANALOG_PIN, INPUT);


    // Initialize TFT
    tft.setRotation(3); // Can be 1 or 3
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    char buffer[100];
    sprintf(buffer, "LINE DETECTORS A0:%d", IR_DIGITAL_PIN, IR_ANALOG_PIN);
    tft.drawString(buffer, 10, 15);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void loop() {
    // Read IR sensor
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
    sprintf(buffer, "Analog: %d -> %d  ", analogValue, encoding);
    tft.drawString(buffer, 10, 40);

    for (int j = 0; j < 4; j++) {
        int remainder = encoding % 2;
        encoding /= 2;
        binarycode[j] = remainder;
    }
    for (int k = 3; k >= 0; k--) {
        if (binarycode[k] < 1) tft.setTextColor(TFT_WHITE, TFT_BLACK);
        else tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawNumber(binarycode[k], 100-60*(k%2), 120-40*(k/2));
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    delay(IR_DELAY_MILLIS);
}
