#include "leds.h"
#include "config.h"
#include "pins.h"
#include "state.h"

namespace leds {
namespace {
constexpr int kLedPins[3] = {PIN_LED_RED, PIN_LED_YELLOW, PIN_LED_GREEN};
const char* const kLedNames[3] = {"red", "yellow", "green"};

struct Color { uint8_t r, g, b; const char* name; };
constexpr Color kColors[] = {
    {255, 0, 0, "red"},      {0, 255, 0, "green"},    {0, 0, 255, "blue"},
    {255, 120, 0, "orange"}, {0, 200, 255, "cyan"},   {255, 0, 180, "magenta"},
    {255, 200, 40, "warm white"}, {0, 0, 0, "off"},
};
constexpr uint8_t kBrightness[] = {255, 128, 40};
int colorIdx = 0, brightIdx = 0;

uint32_t lastBlink = 0, lastChase = 0, lastFlash = 0;
int chaseStep = 0;
bool flashOn = false;

void applyManualRgb() {
  const Color& c = kColors[colorIdx];
  uint8_t s = kBrightness[brightIdx];
  rgbSet(c.r * s / 255, c.g * s / 255, c.b * s / 255);
}
}  // namespace

void begin() {
  pinMode(PIN_LED_ONBOARD, OUTPUT);
  for (int p : kLedPins) pinMode(p, OUTPUT);
  rgbSet(0, 0, 0);
}

void rgbSet(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(PIN_RGB_R, r);
  analogWrite(PIN_RGB_G, g);
  analogWrite(PIN_RGB_B, b);
}

void toggle(int idx) {
  digitalWrite(kLedPins[idx], !digitalRead(kLedPins[idx]));
  Serial.printf("%s LED toggled\n", kLedNames[idx]);
}

void cycleColor() {
  colorIdx = (colorIdx + 1) % (int)(sizeof(kColors) / sizeof(kColors[0]));
  applyManualRgb();
  Serial.printf("RGB color: %s\n", kColors[colorIdx].name);
}

void cycleBrightness() {
  brightIdx = (brightIdx + 1) % (int)(sizeof(kBrightness) / sizeof(kBrightness[0]));
  applyManualRgb();
  Serial.printf("RGB brightness: %d/255\n", kBrightness[brightIdx]);
}

void loop() {
  uint32_t now = millis();

  if (now - lastBlink >= 500) {
    lastBlink = now;
    digitalWrite(PIN_LED_ONBOARD, !digitalRead(PIN_LED_ONBOARD));
  }

  static bool prevParty = false;
  if (app.party && now - lastChase >= 120) {
    lastChase = now;
    for (int i = 0; i < 3; i++)
      digitalWrite(kLedPins[i], i == chaseStep ? HIGH : LOW);
    chaseStep = (chaseStep + 1) % 3;
  }
  if (prevParty && !app.party)
    for (int p : kLedPins) digitalWrite(p, LOW);
  prevParty = app.party;

  // In full mode the RGB is the alarm status light; in phase-1 mode it stays
  // under manual control (BTN4/BTN5).
  if (app.displayFound) {
    switch (app.alarm) {
      case AlarmState::DISARMED:  rgbSet(0, 60, 0); break;
      case AlarmState::ARMING:
      case AlarmState::TRIGGERED: rgbSet(120, 90, 0); break;
      case AlarmState::ARMED:     rgbSet(80, 0, 0); break;
      case AlarmState::SIREN:
        if (now - lastFlash >= 250) { lastFlash = now; flashOn = !flashOn; }
        rgbSet(flashOn ? 255 : 0, 0, 0);
        break;
    }
  }
}
}  // namespace leds
