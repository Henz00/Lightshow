#pragma once

#include <FastLED.h>

enum Mode {
    MODE_RAINBOW,
    MODE_SOLID,
    MODE_AMBIENT,
    MODE_PROGRESS_BAR,
    MODE_UNKNOWN
};

enum ProgressAnimation {
    PROGRESS_INSTANT,
    PROGRESS_FILL,
    PROGRESS_CENTER_OUT,
};

class LEDStrip {
public:
    CRGB* leds;
    int num_leds;

    // Variables from users
    int progress = 0;
    int loop_speed = 40;
    Mode LED_mode = MODE_RAINBOW;
    Mode Current_LED_mode = MODE_RAINBOW;
    ProgressAnimation progressAnimation = PROGRESS_FILL;
    ProgressAnimation currentProgressAnimation = PROGRESS_FILL;
    // Implements below to users later
    uint8_t colour = 0;
    uint8_t colourDelta = 5;

    
    // Variables for class only
    unsigned long lastUpdate = 0;
    int ledIndex = 0;
    int lastProgress = -1;
    int currentLit = 0;
    int lastLit = 0;

    LEDStrip(CRGB* leds, int num_leds);

    void loop();

    void handleProgress(int);
    void handleSpeed(int);
    void handleMode(String);
    Mode parseMode(String);
    void handleAnimation(String);
    ProgressAnimation parseAnimation(String);
    void printValues();

    void resetLEDStrip();

};