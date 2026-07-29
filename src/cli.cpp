#include "cli.h"

#include <Arduino.h>

#include "alarm.h"
#include "net.h"
#include "relays.h"
#include "state.h"

namespace cli {
namespace {
char line[96];
size_t len = 0;

void handle(char* cmd) {
  char* arg = strchr(cmd, ' ');
  if (arg) *arg++ = 0;

  if (!strcmp(cmd, "help")) {
    Serial.println(
        "commands:\n"
        "  status                  full state as JSON\n"
        "  arm | disarm            alarm control (disarm needs no code here)\n"
        "  relay <1|2> <on|off|auto>\n"
        "  page <0-4>              OLED page\n"
        "  sim on|off              enable/disable simulated sensors\n"
        "  sim temp <C> | sim hum <pct> | sim light <0-4095>\n"
        "  sim motion | sim beam   pulse a trigger\n"
        "  notify <text>           test push notification");
    return;
  }
  if (!strcmp(cmd, "status")) { Serial.println(net::statusJson()); return; }
  if (!strcmp(cmd, "arm")) { alarmfsm::arm(); return; }
  if (!strcmp(cmd, "disarm")) { alarmfsm::forceDisarm(); return; }
  if (!strcmp(cmd, "page") && arg) { app.page = constrain(atoi(arg), 0, 4); return; }
  if (!strcmp(cmd, "notify") && arg) { net::notify(arg); return; }
  if (!strcmp(cmd, "relay") && arg) {
    int ch = atoi(arg) - 1;
    char* mode = strchr(arg, ' ');
    if (ch >= 0 && ch <= 1 && mode) {
      mode++;
      relays::setMode(ch, !strcmp(mode, "on")    ? RelayMode::FORCE_ON
                          : !strcmp(mode, "off") ? RelayMode::FORCE_OFF
                                                 : RelayMode::AUTO);
    }
    return;
  }
  if (!strcmp(cmd, "sim") && arg) {
    char* val = strchr(arg, ' ');
    if (val) *val++ = 0;
    if (!strcmp(arg, "on")) { app.simSensors = true; Serial.println("sim: on"); }
    else if (!strcmp(arg, "off")) { app.simSensors = false; Serial.println("sim: off"); }
    else if (!strcmp(arg, "temp") && val) { app.simSensors = true; app.tempC = atof(val); }
    else if (!strcmp(arg, "hum") && val) { app.simSensors = true; app.humidity = atof(val); }
    else if (!strcmp(arg, "light") && val) { app.simSensors = true; app.lightRaw = atoi(val); }
    else if (!strcmp(arg, "motion")) {
      app.simSensors = true; app.pirMotion = true; app.lastMotionMs = millis();
      Serial.println("sim: motion pulse");
    } else if (!strcmp(arg, "beam")) {
      app.simSensors = true; app.beamBroken = true;
      Serial.println("sim: beam pulse");
    }
    return;
  }
  Serial.printf("unknown: %s (try 'help')\n", cmd);
}
}  // namespace

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (len) { line[len] = 0; handle(line); len = 0; }
    } else if (len < sizeof(line) - 1) {
      line[len++] = c;
    }
  }
  // simulated pulses decay so a single `sim motion` behaves like a real blip
  static uint32_t lastDecay = 0;
  if (app.simSensors && millis() - lastDecay > 1500) {
    lastDecay = millis();
    app.pirMotion = false;
    app.beamBroken = false;
  }
}
}  // namespace cli
