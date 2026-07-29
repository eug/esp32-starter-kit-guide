#pragma once
#include <Arduino.h>

namespace leds {
void begin();
void loop();                       // heartbeat, party chase, alarm RGB
void toggle(int idx);              // 0=red 1=yellow 2=green
void rgbSet(uint8_t r, uint8_t g, uint8_t b);
// phase-1 mode helpers (no OLED attached)
void cycleColor();
void cycleBrightness();
}  // namespace leds
