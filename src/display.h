#pragma once
#include <Arduino.h>

namespace display {
constexpr int kPages = 5;  // ENV, LIGHT, ALARM, AUTO, SYSTEM
constexpr int kPageAuto = 3;
void begin();  // sets app.displayFound
void loop();
}  // namespace display
