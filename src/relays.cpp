#include "relays.h"

#include "config.h"
#include "pins.h"

namespace relays {
namespace {
constexpr int kPins[2] = {PIN_RELAY_1, PIN_RELAY_2};  // active LOW modules
uint32_t lastSwitch[2] = {0, 0};
bool darkLatch = false;

void drive(int ch, bool on) {
  if (on == app.relayOn[ch]) return;
  uint32_t now = millis();
  if (now - lastSwitch[ch] < kRelayMinDwellMs && lastSwitch[ch] != 0) return;
  lastSwitch[ch] = now;
  app.relayOn[ch] = on;
  digitalWrite(kPins[ch], on ? LOW : HIGH);
  Serial.printf("relay %d (%s): %s\n", ch + 1, ch == 0 ? "lamp" : "fan", on ? "ON" : "off");
}
}  // namespace

void begin() {
  for (int p : kPins) {
    pinMode(p, OUTPUT);
    digitalWrite(p, HIGH);  // off
  }
}

void cycleMode(int ch) {
  RelayMode m = app.relayMode[ch];
  m = m == RelayMode::AUTO ? RelayMode::FORCE_ON
      : m == RelayMode::FORCE_ON ? RelayMode::FORCE_OFF : RelayMode::AUTO;
  setMode(ch, m);
}

void setMode(int ch, RelayMode m) {
  app.relayMode[ch] = m;
  lastSwitch[ch] = 0;  // manual change applies immediately
  Serial.printf("relay %d mode: %s\n", ch + 1,
                m == RelayMode::AUTO ? "auto" : m == RelayMode::FORCE_ON ? "on" : "off");
}

void loop() {
  // dark detection with hysteresis
  if (app.lightRaw >= 0) {
    if (!darkLatch && app.lightRaw < kDarkThreshold) darkLatch = true;
    if (darkLatch && app.lightRaw > kDarkThreshold + kDarkHysteresis) darkLatch = false;
  }

  // rule: lamp on when dark + recent motion, and the alarm isn't armed
  bool lampWant = darkLatch &&
                  app.lastMotionMs != 0 &&
                  millis() - app.lastMotionMs < kMotionWindowMs &&
                  app.alarm == AlarmState::DISARMED;

  // rule: fan on when temp above pot-set threshold (1C hysteresis)
  bool fanWant = app.relayOn[1];
  if (!isnan(app.tempC)) {
    if (app.tempC > app.fanThreshC) fanWant = true;
    if (app.tempC < app.fanThreshC - kFanHysteresisC) fanWant = false;
  } else {
    fanWant = false;
  }

  bool want[2] = {lampWant, fanWant};
  for (int ch = 0; ch < 2; ch++) {
    switch (app.relayMode[ch]) {
      case RelayMode::AUTO:      drive(ch, want[ch]); break;
      case RelayMode::FORCE_ON:  drive(ch, true); break;
      case RelayMode::FORCE_OFF: drive(ch, false); break;
    }
  }
}
}  // namespace relays
