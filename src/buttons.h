#pragma once
#include <Arduino.h>

// Debounced buttons. poll() once per loop, then read events[i] (true = a new
// press this iteration). BTN indexes 0..5 map to BTN1..BTN6 (5 = BOOT).
namespace buttons {
constexpr int kCount = 6;
extern bool events[kCount];
void begin();
void poll();
}  // namespace buttons
