#include "progress_effect.h"

static void progressInstant(LEDStrip& strip);
static void progressFill(LEDStrip& strip);
static void progressCenterOut(LEDStrip& strip);

void progressEffect(LEDStrip& strip){

  switch (strip.progressAnimation) {
        case PROGRESS_INSTANT:
            progressInstant(strip);
            break;

        case PROGRESS_FILL:
            progressFill(strip);
            break;

        case PROGRESS_CENTER_OUT:
            progressCenterOut(strip);
            break;

        default:
            progressFill(strip);
            break;
    }
}

static int getTargetLit(LEDStrip& strip) {
    int percent = constrain(strip.progress, 0, 100);
    return round((percent / 100.0) * strip.num_leds);
}



static void progressInstant(LEDStrip& strip) {
    int target = getTargetLit(strip);

    if (target == strip.lastLit) return;

    for (int i = 0; i < strip.num_leds; i++) {
        strip.leds[i] = (i < target) ? CRGB::Green : CRGB::Black;
    }

    strip.lastLit = target;
}

static void progressFill(LEDStrip& strip){


  int target = getTargetLit(strip);

  if (target != strip.lastProgress) {
      strip.lastProgress = target;
  }

  if (millis() - strip.lastUpdate > strip.loop_speed) {
    strip.lastUpdate = millis();

    if (strip.currentLit < target){
      strip.currentLit++;
    } else if (strip.currentLit > target){
       strip.currentLit--;
    }
  }

  if (strip.currentLit != strip.lastLit) {

    if (strip.currentLit > strip.lastLit) {
        for (int i = strip.lastLit; i < strip.currentLit; i++) {
            strip.leds[i] = CRGB::Green;
        }
    } else {
        for (int i = strip.currentLit; i < strip.lastLit; i++) {
            strip.leds[i] = CRGB::Black;
        }
    }

    strip.lastLit = strip.currentLit;
  }
}

static void progressCenterOut(LEDStrip& strip) {
    int target = getTargetLit(strip) / 2;

    if (millis() - strip.lastUpdate > strip.loop_speed) {
    strip.lastUpdate = millis();

    if (strip.currentLit < target){
        strip.currentLit++;
    } else if (strip.currentLit > target){
        strip.currentLit--;
    }
    }

    if (strip.currentLit == strip.lastLit) return;  // 👈 KEY FIX

    int center = strip.num_leds / 2;


    for (int i = 0; i < strip.num_leds; i++) {
        strip.leds[i] = CRGB::Black;
    }

    for (int i = 0; i < strip.currentLit; i++) {
        int left = center - i;
        int right = center + i;

        if (left >= 0) strip.leds[left] = CRGB::Green;
        if (right < strip.num_leds) strip.leds[right] = CRGB::Green;
    }

    strip.lastLit = strip.currentLit;
}


