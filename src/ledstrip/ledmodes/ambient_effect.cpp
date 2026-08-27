#include "ambient_effect.h"

void ambientEffect(LEDStrip& strip){
  if (millis() - strip.lastUpdate < strip.loop_speed) {
    return;
  }

  strip.lastUpdate = millis();

  strip.colour += strip.colourDelta;

  for(int i = 0; i < strip.num_leds; i++){
      strip.leds[i] = CHSV(strip.colour, 255, 255);
  }
}
