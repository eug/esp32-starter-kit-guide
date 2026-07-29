#pragma once
#include <Arduino.h>

namespace net {
void begin();
void loop();
void notify(const char* msg);  // rate-limited ntfy push (no-op if unconfigured)
String statusJson();
}  // namespace net
