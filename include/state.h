#pragma once
#include <Arduino.h>

enum class AlarmState { DISARMED, ARMING, ARMED, TRIGGERED, SIREN };
enum class RelayMode { AUTO, FORCE_ON, FORCE_OFF };

struct AppState {
  // sensors (NAN / 0 until first good read)
  float tempC = NAN;
  float humidity = NAN;
  int lightRaw = -1;   // -1 = no reading yet
  int potRaw = 0;
  bool simSensors = false;  // serial CLI can inject fake values

  // motion
  bool pirMotion = false;
  bool beamBroken = false;
  uint32_t lastMotionMs = 0;

  // alarm
  AlarmState alarm = AlarmState::DISARMED;
  uint32_t alarmSince = 0;
  char lastTrigger[16] = "none";
  int wrongCodes = 0;

  // relays
  bool relayOn[2] = {false, false};
  RelayMode relayMode[2] = {RelayMode::AUTO, RelayMode::AUTO};
  float fanThreshC = 26.0f;

  // ui
  int page = 0;
  bool party = false;
  bool displayFound = false;  // false -> phase-1 button behaviors

  // net
  bool wifiUp = false;
  char ip[20] = "";
  bool timeSynced = false;
  bool mqttUp = false;
};

extern AppState app;

const char* alarmStateName(AlarmState s);
