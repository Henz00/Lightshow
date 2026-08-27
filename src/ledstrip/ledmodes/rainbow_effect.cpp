#include "rainbow_effect.h"

void rainbowEffect(LEDStrip& strip){
  if (millis() - strip.lastUpdate < strip.loop_speed) {
    return;
  }

  strip.lastUpdate = millis();

  fadeToBlackBy(strip.leds, strip.num_leds, 128);

  strip.leds[strip.ledIndex] = CHSV(strip.colour += strip.colourDelta, 255, 255);

  strip.ledIndex++;

  if (strip.ledIndex >= strip.num_leds) {
    strip.ledIndex = 0;
  }

}