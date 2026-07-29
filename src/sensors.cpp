#include "sensors.h"

#include <DHT.h>

#include "config.h"
#include "pins.h"
#include "state.h"

namespace sensors {
namespace {
DHT dht(PIN_DHT, DHT11);
uint32_t lastDht = 0, lastAdc = 0;
float ldrEma = -1, potEma = -1;
}  // namespace

void begin() {
  dht.begin();
  analogReadResolution(12);
}

void loop() {
  uint32_t now = millis();
  if (app.simSensors) return;  // CLI is injecting values

  if (now - lastDht >= 2000) {
    lastDht = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) app.tempC = t;      // keep last good value on bad reads
    if (!isnan(h)) app.humidity = h;
  }

  if (now - lastAdc >= 100) {
    lastAdc = now;
    int ldr = analogRead(PIN_LDR);
    int pot = analogRead(PIN_POT);
    ldrEma = ldrEma < 0 ? ldr : ldrEma * 0.8f + ldr * 0.2f;
    potEma = potEma < 0 ? pot : potEma * 0.8f + pot * 0.2f;
    app.lightRaw = (int)ldrEma;
    app.potRaw = (int)potEma;
    app.fanThreshC = kFanThreshMinC +
                     (kFanThreshMaxC - kFanThreshMinC) * (app.potRaw / 4095.0f);
  }
}
}  // namespace sensors
