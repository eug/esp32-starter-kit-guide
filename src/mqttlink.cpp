#include "mqttlink.h"

#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef MQTT_HOST
namespace mqttlink {
void begin() {}
void loop() {}
}  // namespace mqttlink
#else

#include <PubSubClient.h>
#include <WiFi.h>

#include "alarm.h"
#include "net.h"
#include "relays.h"
#include "state.h"

namespace mqttlink {
namespace {
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
uint32_t lastAttempt = 0, lastPublish = 0;
bool discoverySent = false;

void publishDiscovery() {
  // Home Assistant MQTT discovery: entities appear automatically.
  struct Ent { const char* comp; const char* id; const char* cfg; };
  const Ent ents[] = {
      {"sensor", "rg_temp",
       "{\"name\":\"Guardian temp\",\"stat_t\":\"rg/state\",\"unit_of_meas\":\"°C\","
       "\"dev_cla\":\"temperature\",\"val_tpl\":\"{{value_json.temp}}\",\"uniq_id\":\"rg_temp\"}"},
      {"sensor", "rg_hum",
       "{\"name\":\"Guardian humidity\",\"stat_t\":\"rg/state\",\"unit_of_meas\":\"%\","
       "\"dev_cla\":\"humidity\",\"val_tpl\":\"{{value_json.hum}}\",\"uniq_id\":\"rg_hum\"}"},
      {"sensor", "rg_light",
       "{\"name\":\"Guardian light\",\"stat_t\":\"rg/state\","
       "\"val_tpl\":\"{{value_json.light}}\",\"uniq_id\":\"rg_light\"}"},
      {"binary_sensor", "rg_motion",
       "{\"name\":\"Guardian motion\",\"stat_t\":\"rg/state\",\"dev_cla\":\"motion\","
       "\"val_tpl\":\"{{'ON' if value_json.motion else 'OFF'}}\",\"uniq_id\":\"rg_motion\"}"},
      {"sensor", "rg_alarm",
       "{\"name\":\"Guardian alarm\",\"stat_t\":\"rg/state\","
       "\"val_tpl\":\"{{value_json.alarm}}\",\"uniq_id\":\"rg_alarm\"}"},
      {"switch", "rg_lamp",
       "{\"name\":\"Guardian lamp\",\"stat_t\":\"rg/state\",\"cmd_t\":\"rg/relay1/set\","
       "\"val_tpl\":\"{{'ON' if value_json.relay1 else 'OFF'}}\",\"uniq_id\":\"rg_lamp\"}"},
      {"switch", "rg_fan",
       "{\"name\":\"Guardian fan\",\"stat_t\":\"rg/state\",\"cmd_t\":\"rg/relay2/set\","
       "\"val_tpl\":\"{{'ON' if value_json.relay2 else 'OFF'}}\",\"uniq_id\":\"rg_fan\"}"},
  };
  char topic[80];
  for (auto& e : ents) {
    snprintf(topic, sizeof(topic), "homeassistant/%s/%s/config", e.comp, e.id);
    mqtt.publish(topic, e.cfg, true);
  }
  discoverySent = true;
}

void onMessage(char* topic, byte* payload, unsigned int len) {
  String t(topic), p;
  for (unsigned int i = 0; i < len; i++) p += (char)payload[i];
  int ch = t == "rg/relay1/set" ? 0 : t == "rg/relay2/set" ? 1 : -1;
  if (ch >= 0) relays::setMode(ch, p == "ON" ? RelayMode::FORCE_ON : RelayMode::FORCE_OFF);
  if (t == "rg/alarm/set") {
    if (p == "ARM") alarmfsm::arm();
    // disarm via MQTT intentionally requires the code appended: DISARM:3124
    if (p.startsWith("DISARM:") && p.length() == 11) {
      int d[4];
      for (int i = 0; i < 4; i++) d[i] = p[7 + i] - '0';
      alarmfsm::disarm(d, 4);
    }
  }
}
}  // namespace

void begin() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);
}

void loop() {
  if (!app.wifiUp) { app.mqttUp = false; return; }

  if (!mqtt.connected()) {
    app.mqttUp = false;
    if (millis() - lastAttempt < 10000) return;
    lastAttempt = millis();
    // LWT: HA marks the device unavailable if we drop off
    if (mqtt.connect("room-guardian", MQTT_USER, MQTT_PASSWORD,
                     "rg/availability", 0, true, "offline")) {
      mqtt.publish("rg/availability", "online", true);
      mqtt.subscribe("rg/relay1/set");
      mqtt.subscribe("rg/relay2/set");
      mqtt.subscribe("rg/alarm/set");
      if (!discoverySent) publishDiscovery();
      Serial.println("mqtt: connected");
    }
    return;
  }

  app.mqttUp = true;
  mqtt.loop();
  if (millis() - lastPublish >= 10000) {
    lastPublish = millis();
    mqtt.publish("rg/state", net::statusJson().c_str());
  }
}
}  // namespace mqttlink
#endif  // MQTT_HOST
