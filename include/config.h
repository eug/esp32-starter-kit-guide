#pragma once

// Alarm
constexpr int kAlarmCode[4] = {3, 1, 2, 4};  // BTN sequence to disarm
constexpr uint32_t kExitDelayMs = 10000;     // arm -> armed countdown
constexpr uint32_t kEntryDelayMs = 8000;     // trigger -> siren grace
constexpr uint32_t kSirenMaxMs = 60000;      // siren auto-stop
constexpr uint32_t kPirWarmupMs = 60000;     // HC-SR501 settle time after boot
constexpr uint32_t kCodeTimeoutMs = 5000;    // keypad entry resets after this
constexpr int kMaxWrongCodes = 3;

// Automation
constexpr int kDarkThreshold = 1200;         // LDR raw (0-4095), below = dark
constexpr int kDarkHysteresis = 200;
constexpr uint32_t kMotionWindowMs = 5UL * 60UL * 1000UL;  // lamp: motion within 5 min
constexpr float kFanHysteresisC = 1.0f;
constexpr float kFanThreshMinC = 18.0f;      // pot maps across this range
constexpr float kFanThreshMaxC = 32.0f;
constexpr uint32_t kRelayMinDwellMs = 30000; // no relay chatter

// Net
constexpr uint32_t kNotifyRateMs = 60000;    // max one push per minute
constexpr char kHostname[] = "room-guardian";
constexpr char kTz[] = "CET-1CEST,M3.5.0,M10.5.0/3";  // Europe (adjust if needed)
