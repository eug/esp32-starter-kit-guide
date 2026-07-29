// Copy this file to secrets.h (git-ignored) and fill in.
// Without a secrets.h the firmware runs fully offline — everything but
// WiFi/notifications/MQTT works.
#pragma once

#define WIFI_SSID "your-wifi-name"
#define WIFI_PASS "your-wifi-password"

// Optional: push notifications via https://ntfy.sh — pick a random topic name,
// subscribe to it in the ntfy phone app, uncomment:
// #define NTFY_TOPIC "room-guardian-CHANGE-ME-8231"

// Optional: MQTT / Home Assistant (Phase 6), uncomment:
// #define MQTT_HOST "192.168.1.10"
// #define MQTT_PORT 1883
// #define MQTT_USER ""
// #define MQTT_PASSWORD ""
