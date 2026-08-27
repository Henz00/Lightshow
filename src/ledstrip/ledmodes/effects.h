#pragma once
#include "../led_strip.h"

typedef void (*EffectFunc)(LEDStrip&);

EffectFunc getEffectFunction(int mode);