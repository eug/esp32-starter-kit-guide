#include "buttons.h"
#include "pins.h"

namespace buttons {

bool events[kCount];

namespace {
constexpr int kPins[kCount] = {PIN_BTN_1, PIN_BTN_2, PIN_BTN_3,
                               PIN_BTN_4, PIN_BTN_5, PIN_BTN_6};
constexpr uint32_t kDebounceMs = 30;
bool lastReading[kCount];
bool stable[kCount];
uint32_t lastChange[kCount];
}  // namespace

void begin() {
  for (int i = 0; i < kCount; i++) {
    pinMode(kPins[i], INPUT_PULLUP);
    lastReading[i] = stable[i] = false;
    lastChange[i] = 0;
  }
}

void poll() {
  uint32_t now = millis();
  for (int i = 0; i < kCount; i++) {
    events[i] = false;
    bool reading = digitalRead(kPins[i]) == LOW;
    if (reading != lastReading[i]) {
      lastChange[i] = now;
      lastReading[i] = reading;
    }
    if (now - lastChange[i] > kDebounceMs && reading != stable[i]) {
      stable[i] = reading;
      if (stable[i]) events[i] = true;
    }
  }
}

}  // namespace buttons
