#include "led_strip.h"
#include <FastLED.h>
#include "ledmodes/effects.h"


LEDStrip::LEDStrip(CRGB* leds, int num_leds): 
  leds(leds),
  num_leds(num_leds)
{
}

void LEDStrip::loop() {
  if(Current_LED_mode != LED_mode || currentProgressAnimation != progressAnimation){
    resetLEDStrip();
    Current_LED_mode = LED_mode;
    currentProgressAnimation = progressAnimation;
  }

  EffectFunc effect = getEffectFunction(LED_mode);
  effect(*this);    
}

void LEDStrip::handleProgress(int progress_target){
  progress = progress_target;
}

void LEDStrip::handleSpeed(int speed){
  loop_speed = speed;
}

void LEDStrip::handleMode(String newMode){
    LED_mode = parseMode(newMode);
}

Mode LEDStrip::parseMode(String modeStr) {
  if (modeStr == "rainbow") return MODE_RAINBOW;
  if (modeStr == "solid") return MODE_SOLID;
  if (modeStr == "ambient") return MODE_AMBIENT;
  if (modeStr == "progress_bar") return MODE_PROGRESS_BAR;

  return MODE_UNKNOWN;
}

void LEDStrip::handleAnimation(String newAnimation){
  progressAnimation = parseAnimation(newAnimation);
}

ProgressAnimation LEDStrip::parseAnimation(String animationStr){
  if (animationStr == "instant") return PROGRESS_INSTANT;
  if (animationStr == "fill") return PROGRESS_FILL;
  if (animationStr == "center_out") return PROGRESS_CENTER_OUT;
}

void LEDStrip::printValues(){
    /* Implement if needed */
}

void LEDStrip::resetLEDStrip(){
  FastLED.clear(true);
  FastLED.show();

  lastUpdate = 0;
  ledIndex = 0;
  lastProgress = -1;
  currentLit = 0;
  lastLit = 0;
}
