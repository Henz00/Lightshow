#include "effects.h"
#include "rainbow_effect.h"
#include "solid_effect.h"
#include "progress_effect.h"
#include "ambient_effect.h"

EffectFunc getEffectFunction(int mode) {
    switch (mode) {
        case MODE_RAINBOW: return rainbowEffect;
        case MODE_SOLID: return solidEffect;
        case MODE_AMBIENT: return ambientEffect;
        case MODE_PROGRESS_BAR: return progressEffect;
        default: return rainbowEffect;
    }
}