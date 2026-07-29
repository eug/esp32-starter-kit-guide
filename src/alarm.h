#pragma once
#include <Arduino.h>

namespace alarmfsm {
void begin();
void loop();                 // sensors -> state machine -> buzzers
void keyDigit(int digit);    // 1..4 from keypad buttons
void arm();
bool disarm(const int* code, int len);  // true if code correct
void forceDisarm();          // CLI/web with verified code
// injected by net module; called on siren with a reason string
extern void (*onSiren)(const char* reason);
}  // namespace alarmfsm
