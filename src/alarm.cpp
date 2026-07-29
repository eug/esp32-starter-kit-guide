#include "alarm.h"

#include "config.h"
#include "pins.h"
#include "state.h"

namespace alarmfsm {

void (*onSiren)(const char* reason) = nullptr;

namespace {
constexpr int kToneChannel = 6;  // analogWrite RGB uses low channels
int codeBuf[4];
int codeLen = 0;
uint32_t lastKeyMs = 0;
uint32_t lastSirenStep = 0;
bool sirenHigh = false;
uint32_t beepUntil = 0;

void tone(uint32_t freq) { ledcWriteTone(kToneChannel, freq); }

void beep(uint32_t ms) {  // short UI beep on the active buzzer
  digitalWrite(PIN_BUZZER_ACTIVE, HIGH);
  beepUntil = millis() + ms;
}

void setState(AlarmState s, const char* why) {
  app.alarm = s;
  app.alarmSince = millis();
  Serial.printf("alarm: %s (%s)\n", alarmStateName(s), why);
  if (s == AlarmState::SIREN && onSiren) onSiren(app.lastTrigger);
  if (s != AlarmState::SIREN) tone(0);
}

void checkCode() {
  if (codeLen < 4) return;
  bool ok = true;
  for (int i = 0; i < 4; i++)
    if (codeBuf[i] != kAlarmCode[i]) ok = false;
  codeLen = 0;
  if (ok) {
    app.wrongCodes = 0;
    setState(AlarmState::DISARMED, "code accepted");
    beep(80);
  } else {
    app.wrongCodes++;
    Serial.printf("wrong code (%d/%d)\n", app.wrongCodes, kMaxWrongCodes);
    beep(400);
    if (app.wrongCodes >= kMaxWrongCodes &&
        (app.alarm == AlarmState::ARMED || app.alarm == AlarmState::TRIGGERED)) {
      strlcpy(app.lastTrigger, "bad codes", sizeof(app.lastTrigger));
      setState(AlarmState::SIREN, "too many wrong codes");
    }
  }
}
}  // namespace

void begin() {
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_OBSTACLE, INPUT);
  pinMode(PIN_BUZZER_ACTIVE, OUTPUT);
  digitalWrite(PIN_BUZZER_ACTIVE, LOW);
  ledcSetup(kToneChannel, 2000, 10);
  ledcAttachPin(PIN_BUZZER_PASSIVE, kToneChannel);
  tone(0);
}

void keyDigit(int digit) {
  uint32_t now = millis();
  if (now - lastKeyMs > kCodeTimeoutMs) codeLen = 0;
  lastKeyMs = now;
  if (codeLen < 4) codeBuf[codeLen++] = digit;
  beep(30);
  checkCode();
}

void arm() {
  if (app.alarm != AlarmState::DISARMED) return;
  setState(AlarmState::ARMING, "arm requested");
}

bool disarm(const int* code, int len) {
  if (len != 4) return false;
  for (int i = 0; i < 4; i++)
    if (code[i] != kAlarmCode[i]) return false;
  forceDisarm();
  return true;
}

void forceDisarm() {
  app.wrongCodes = 0;
  codeLen = 0;
  setState(AlarmState::DISARMED, "disarmed");
}

void loop() {
  uint32_t now = millis();

  if (beepUntil && now >= beepUntil) {
    digitalWrite(PIN_BUZZER_ACTIVE, LOW);
    beepUntil = 0;
  }

  // sensor reads (PIR needs warm-up; obstacle module is active LOW)
  if (!app.simSensors) {
    app.pirMotion = now > kPirWarmupMs && digitalRead(PIN_PIR) == HIGH;
    app.beamBroken = digitalRead(PIN_OBSTACLE) == LOW;
  }
  if (app.pirMotion) app.lastMotionMs = now;

  switch (app.alarm) {
    case AlarmState::ARMING:
      if ((now - app.alarmSince) % 1000 < 30 && !beepUntil) beep(25);
      if (now - app.alarmSince >= kExitDelayMs) setState(AlarmState::ARMED, "exit delay over");
      break;
    case AlarmState::ARMED:
      if (app.pirMotion) {
        strlcpy(app.lastTrigger, "motion", sizeof(app.lastTrigger));
        setState(AlarmState::TRIGGERED, "PIR");
      } else if (app.beamBroken) {
        strlcpy(app.lastTrigger, "beam", sizeof(app.lastTrigger));
        setState(AlarmState::TRIGGERED, "IR beam");
      }
      break;
    case AlarmState::TRIGGERED:
      if ((now - app.alarmSince) % 500 < 30 && !beepUntil) beep(25);
      if (now - app.alarmSince >= kEntryDelayMs) setState(AlarmState::SIREN, "entry delay over");
      break;
    case AlarmState::SIREN:
      if (now - lastSirenStep >= 350) {
        lastSirenStep = now;
        sirenHigh = !sirenHigh;
        tone(sirenHigh ? 2400 : 1600);
      }
      if (now - app.alarmSince >= kSirenMaxMs) setState(AlarmState::ARMED, "siren timeout, rearmed");
      break;
    case AlarmState::DISARMED:
      break;
  }
}
}  // namespace alarmfsm
