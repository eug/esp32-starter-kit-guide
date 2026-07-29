#include "net.h"

#if __has_include("secrets.h")
#include "secrets.h"
#endif

#include <WiFi.h>

#include "config.h"
#include "state.h"

#ifdef WIFI_SSID
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>

#include "alarm.h"
#include "relays.h"
#endif

namespace net {

String statusJson() {
  char buf[420];
  snprintf(buf, sizeof(buf),
           "{\"temp\":%.1f,\"hum\":%.0f,\"light\":%d,\"pot\":%d,"
           "\"alarm\":\"%s\",\"lastTrigger\":\"%s\",\"motion\":%s,\"beam\":%s,"
           "\"relay1\":%s,\"relay2\":%s,\"mode1\":%d,\"mode2\":%d,"
           "\"fanThresh\":%.1f,\"uptime\":%lu,\"heap\":%u,\"wifi\":%s}",
           isnan(app.tempC) ? -99.0f : app.tempC,
           isnan(app.humidity) ? -1.0f : app.humidity, app.lightRaw, app.potRaw,
           alarmStateName(app.alarm), app.lastTrigger,
           app.pirMotion ? "true" : "false", app.beamBroken ? "true" : "false",
           app.relayOn[0] ? "true" : "false", app.relayOn[1] ? "true" : "false",
           (int)app.relayMode[0], (int)app.relayMode[1], app.fanThreshC,
           millis() / 1000, (unsigned)ESP.getFreeHeap(),
           app.wifiUp ? "true" : "false");
  return String(buf);
}

#ifndef WIFI_SSID
// ---- offline build: no secrets.h -----------------------------------------
void begin() { Serial.println("net: no secrets.h, running offline"); }
void loop() {}
void notify(const char*) {}
#else
// ---- online build ---------------------------------------------------------
namespace {
WebServer server(80);
uint32_t lastAttempt = 0, lastNotify = 0;
bool serverUp = false;
char pendingNotify[96] = "";

const char kIndexHtml[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1"><title>Room Guardian</title>
<style>body{font:16px system-ui;background:#10160f;color:#dfe8df;max-width:420px;margin:20px auto;padding:0 14px}
h1{font-size:22px}.card{background:#1a231a;border-radius:10px;padding:14px;margin:10px 0}
.big{font-size:30px;font-weight:700}button{font:15px system-ui;background:#2c3a2c;color:#dfe8df;border:1px solid #445544;border-radius:8px;padding:8px 14px;margin:4px 4px 0 0}
input{font:16px monospace;width:90px;background:#101810;color:#dfe8df;border:1px solid #445544;border-radius:6px;padding:6px}
.on{color:#7fd89a}.off{color:#8a958a}#alarm.SIREN{color:#ff6a5e}</style></head><body>
<h1>&#128737;&#65039; Room Guardian</h1>
<div class=card><div class=big><span id=temp>--</span>&deg;C &nbsp; <span id=hum>--</span>%</div>
light <span id=light>--</span> &middot; up <span id=up>--</span>s</div>
<div class=card>alarm: <b id=alarm class=big>--</b> <div>last: <span id=lt>--</span></div>
<div><input id=code placeholder=code maxlength=4>
<button onclick="alarmCmd('arm')">arm</button><button onclick="alarmCmd('disarm')">disarm</button></div></div>
<div class=card>lamp <b id=r1>--</b> <button onclick="relay(1)">cycle</button><br>
fan <b id=r2>--</b> (&gt;<span id=ft>--</span>&deg;C) <button onclick="relay(2)">cycle</button></div>
<script>
async function tick(){try{let r=await fetch('/api/status'),j=await r.json();
temp.textContent=j.temp==-99?'--':j.temp;hum.textContent=j.hum<0?'--':j.hum;
light.textContent=j.light<0?'--':j.light;up.textContent=j.uptime;
alarm.textContent=j.alarm;alarm.className='big '+j.alarm;lt.textContent=j.lastTrigger;
r1.textContent=j.relay1?'ON':'off';r1.className=j.relay1?'on':'off';
r2.textContent=j.relay2?'ON':'off';r2.className=j.relay2?'on':'off';ft.textContent=j.fanThresh;
}catch(e){}}
async function alarmCmd(a){await fetch('/api/alarm?action='+a+'&code='+code.value,{method:'POST'});tick()}
async function relay(c){await fetch('/api/relay?ch='+c,{method:'POST'});tick()}
tick();setInterval(tick,2000);</script></body></html>)HTML";

void handleAlarm() {
  String action = server.arg("action");
  String code = server.arg("code");
  if (action == "arm") {
    alarmfsm::arm();
    server.send(200, "text/plain", "arming");
    return;
  }
  if (action == "disarm") {
    int digits[4];
    if (code.length() == 4) {
      for (int i = 0; i < 4; i++) digits[i] = code[i] - '0';
      if (alarmfsm::disarm(digits, 4)) {
        server.send(200, "text/plain", "disarmed");
        return;
      }
    }
    server.send(403, "text/plain", "bad code");
    return;
  }
  server.send(400, "text/plain", "action=arm|disarm");
}

void startServer() {
  server.on("/", []() { server.send_P(200, "text/html", kIndexHtml); });
  server.on("/api/status", []() { server.send(200, "application/json", statusJson()); });
  server.on("/api/alarm", HTTP_POST, handleAlarm);
  server.on("/api/relay", HTTP_POST, []() {
    int ch = server.arg("ch").toInt();
    if (ch == 1 || ch == 2) {
      relays::cycleMode(ch - 1);
      server.send(200, "text/plain", "ok");
    } else {
      server.send(400, "text/plain", "ch=1|2");
    }
  });
  server.begin();
  serverUp = true;
}

void sendPending() {
#ifdef NTFY_TOPIC
  if (!pendingNotify[0] || !app.wifiUp) return;
  if (millis() - lastNotify < kNotifyRateMs && lastNotify != 0) return;
  lastNotify = millis();
  WiFiClientSecure client;
  client.setInsecure();  // ntfy.sh public cert, fine for a hobby alarm
  HTTPClient http;
  if (http.begin(client, String("https://ntfy.sh/") + NTFY_TOPIC)) {
    http.addHeader("Title", "Room Guardian");
    int rc = http.POST(String(pendingNotify));
    Serial.printf("ntfy: %d\n", rc);
    http.end();
  }
  pendingNotify[0] = 0;
#endif
}
}  // namespace

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(kHostname);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  lastAttempt = millis();
  Serial.printf("net: connecting to %s...\n", WIFI_SSID);
}

void loop() {
  bool up = WiFi.status() == WL_CONNECTED;
  if (up && !app.wifiUp) {
    strlcpy(app.ip, WiFi.localIP().toString().c_str(), sizeof(app.ip));
    Serial.printf("net: connected, http://%s (http://%s.local)\n", app.ip, kHostname);
    MDNS.begin(kHostname);
    configTzTime(kTz, "pool.ntp.org");
    app.timeSynced = true;  // best-effort; display falls back to uptime
    if (!serverUp) startServer();
  }
  if (!up && app.wifiUp) Serial.println("net: connection lost");
  app.wifiUp = up;

  if (!up && millis() - lastAttempt > 15000) {  // retry without blocking
    lastAttempt = millis();
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
  if (serverUp) server.handleClient();
  sendPending();
}

void notify(const char* msg) {
  strlcpy(pendingNotify, msg, sizeof(pendingNotify));
  Serial.printf("notify queued: %s\n", msg);
}
#endif  // WIFI_SSID
}  // namespace net
