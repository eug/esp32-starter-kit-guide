#include "display.h"

#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <time.h>

#include "config.h"
#include "pins.h"
#include "state.h"

namespace display {
namespace {
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
uint32_t lastDraw = 0;

const char* modeName(RelayMode m) {
  return m == RelayMode::AUTO ? "auto" : m == RelayMode::FORCE_ON ? "ON" : "OFF";
}

void header(const char* title) {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(title);
  oled.setCursor(98, 0);
  oled.printf("%d/%d", app.page + 1, kPages);
  oled.drawFastHLine(0, 10, 128, SSD1306_WHITE);
}

void pageEnv() {
  header("ENVIRONMENT");
  oled.setTextSize(2);
  oled.setCursor(0, 18);
  if (isnan(app.tempC)) oled.print("--.- C");
  else oled.printf("%.1f C", app.tempC);
  oled.setCursor(0, 42);
  if (isnan(app.humidity)) oled.print("--%");
  else oled.printf("%.0f%% rh", app.humidity);
}

void pageLight() {
  header("LIGHT");
  oled.setTextSize(2);
  oled.setCursor(0, 18);
  if (app.lightRaw < 0) oled.print("----");
  else oled.printf("%d", app.lightRaw);
  oled.setTextSize(1);
  oled.setCursor(0, 40);
  oled.print(app.lightRaw >= 0 && app.lightRaw < kDarkThreshold ? "dark" : "bright");
  int w = app.lightRaw < 0 ? 0 : map(app.lightRaw, 0, 4095, 0, 128);
  oled.drawRect(0, 54, 128, 8, SSD1306_WHITE);
  oled.fillRect(0, 54, w, 8, SSD1306_WHITE);
}

void pageAlarm() {
  header("ALARM");
  oled.setTextSize(2);
  oled.setCursor(0, 18);
  oled.print(alarmStateName(app.alarm));
  oled.setTextSize(1);
  oled.setCursor(0, 42);
  long remainMs = app.alarm == AlarmState::ARMING
                      ? (long)kExitDelayMs - (long)(millis() - app.alarmSince)
                      : (long)kEntryDelayMs - (long)(millis() - app.alarmSince);
  if (remainMs < 0) remainMs = 0;
  if (app.alarm == AlarmState::ARMING)
    oled.printf("exit in %lds", remainMs / 1000 + 1);
  else if (app.alarm == AlarmState::TRIGGERED)
    oled.printf("DISARM! %lds", remainMs / 1000 + 1);
  else
    oled.printf("last: %s", app.lastTrigger);
  oled.setCursor(0, 54);
  if (millis() < kPirWarmupMs) oled.print("PIR warming up...");
  else oled.printf("pir:%d beam:%d", app.pirMotion, app.beamBroken);
}

void pageAuto() {
  header("AUTOMATION");
  oled.setTextSize(1);
  oled.setCursor(0, 16);
  oled.printf("lamp: %s (%s)", app.relayOn[0] ? "ON " : "off", modeName(app.relayMode[0]));
  oled.setCursor(0, 28);
  oled.printf("fan : %s (%s)", app.relayOn[1] ? "ON " : "off", modeName(app.relayMode[1]));
  oled.setCursor(0, 42);
  oled.printf("fan thresh: %.1fC (pot)", app.fanThreshC);
  oled.setCursor(0, 54);
  oled.print("BTN1/BTN2 = override");
}

void pageSystem() {
  header("SYSTEM");
  oled.setTextSize(1);
  oled.setCursor(0, 16);
  oled.printf("wifi: %s", app.wifiUp ? app.ip : "offline");
  oled.setCursor(0, 28);
  if (app.timeSynced) {
    struct tm t;
    if (getLocalTime(&t, 10)) {
      char buf[24];
      strftime(buf, sizeof(buf), "%H:%M:%S  %d %b", &t);
      oled.print(buf);
    }
  } else {
    oled.printf("up: %lus", millis() / 1000);
  }
  oled.setCursor(0, 40);
  oled.printf("heap: %u", (unsigned)ESP.getFreeHeap());
  oled.setCursor(0, 52);
  oled.printf("mqtt: %s", app.mqttUp ? "up" : "-");
}
}  // namespace

void begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  app.displayFound = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (app.displayFound) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 24);
    oled.setTextSize(2);
    oled.print("Guardian");
    oled.display();
  }
  Serial.printf("OLED: %s\n", app.displayFound ? "found" : "not found (phase-1 mode)");
}

void loop() {
  if (!app.displayFound || millis() - lastDraw < 250) return;
  lastDraw = millis();
  oled.clearDisplay();
  switch (app.page) {
    case 0: pageEnv(); break;
    case 1: pageLight(); break;
    case 2: pageAlarm(); break;
    case 3: pageAuto(); break;
    default: pageSystem(); break;
  }
  oled.display();
}
}  // namespace display
