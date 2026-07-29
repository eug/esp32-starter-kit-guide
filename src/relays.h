#pragma once
#include <Arduino.h>
#include "state.h"

namespace relays {
void begin();
void loop();  // evaluate rules, honor overrides + min dwell
void cycleMode(int ch);  // AUTO -> FORCE_ON -> FORCE_OFF -> AUTO
void setMode(int ch, RelayMode m);
}  // namespace relays
