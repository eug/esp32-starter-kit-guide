#pragma once
#include <Arduino.h>

namespace sensors {
void begin();
void loop();  // DHT every 2s, LDR/pot smoothed every 100ms -> app state
}  // namespace sensors
