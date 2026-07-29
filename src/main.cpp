// Room Guardian — full firmware, phases 1-6.
//
// Hardware-adaptive: every module works with its hardware missing. With no
// OLED attached the buttons keep their Phase 1 behaviors (BTN4 = RGB color,
// BTN5 = brightness) so the Phase 1 checkpoint still passes. With the OLED
// present: BTN1-4 = keypad digits / LED toggles, BTN4 also = next page on
// double duty via pages, BTN5 = arm, BOOT = party.
//
// Serial console: type `help` at 115200.

#include <Arduino.h>

#include "alarm.h"
#include "buttons.h"
#include "cli.h"
#include "display.h"
#include "leds.h"
#include "mqttlink.h"
#include "net.h"
#include "pins.h"
#include "relays.h"
#include "sensors.h"
#include "state.h"

AppState app;

const char* alarmStateName(AlarmState s) {
  switch (s) {
    case AlarmState::DISARMED:  return "DISARMED";
    case AlarmState::ARMING:    return "ARMING";
    case AlarmState::ARMED:     return "ARMED";
    case AlarmState::TRIGGERED: return "TRIGGERED";
    default:                    return "SIREN";
  }
}

namespace {
void sirenNotify(const char* reason) {
  char msg[80];
  snprintf(msg, sizeof(msg), "ALARM! trigger: %s", reason);
  net::notify(msg);
}

void dispatchButtons() {
  bool armedish = app.alarm != AlarmState::DISARMED;

  // BTN1-4 are keypad digits whenever a code could disarm something.
  // Deliberately NO shortcut disarm here — only the code (or serial `disarm`,
  // for bench testing) silences an armed system.
  if (armedish) {
    for (int i = 0; i < 4; i++)
      if (buttons::events[i]) alarmfsm::keyDigit(i + 1);
    return;
  }

  if (!app.displayFound) {  // phase-1 mode: original controls
    if (buttons::events[0]) leds::toggle(0);
    if (buttons::events[1]) leds::toggle(1);
    if (buttons::events[2]) leds::toggle(2);
    if (buttons::events[3]) leds::cycleColor();
    if (buttons::events[4]) leds::cycleBrightness();
    if (buttons::events[5]) {
      app.party = !app.party;
      Serial.printf("party mode %s\n", app.party ? "ON" : "OFF");
    }
    return;
  }

  // full mode, disarmed
  bool onAutoPage = app.page == display::kPageAuto;
  if (buttons::events[0]) onAutoPage ? relays::cycleMode(0) : leds::toggle(0);
  if (buttons::events[1]) onAutoPage ? relays::cycleMode(1) : leds::toggle(1);
  if (buttons::events[2]) leds::toggle(2);
  if (buttons::events[3]) {
    app.page = (app.page + 1) % display::kPages;
    Serial.printf("page %d\n", app.page);
  }
  if (buttons::events[4]) alarmfsm::arm();
  if (buttons::events[5]) {
    app.party = !app.party;
    Serial.printf("party mode %s\n", app.party ? "ON" : "OFF");
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  leds::begin();
  buttons::begin();
  sensors::begin();
  display::begin();
  alarmfsm::begin();
  relays::begin();
  net::begin();
  mqttlink::begin();
  alarmfsm::onSiren = sirenNotify;
  Serial.println("\nRoom Guardian ready. Type 'help' for the console.");
  Serial.printf("mode: %s\n", app.displayFound ? "full" : "phase-1 (no OLED)");
}

void loop() {
  buttons::poll();
  dispatchButtons();
  sensors::loop();
  alarmfsm::loop();
  relays::loop();
  leds::loop();
  display::loop();
  net::loop();
  mqttlink::loop();
  cli::loop();
}
