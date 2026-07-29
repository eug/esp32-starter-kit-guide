#pragma once

// Room Guardian — full pin map for all phases.
// Wire once; each phase just starts using more of these.
//
// GPIO 34/35/36/39 are input-only (no pullups) — used for sensors that
// drive their own output. Buttons use INPUT_PULLUP, wired to GND.

// --- Phase 1: LEDs + buttons ---
constexpr int PIN_LED_ONBOARD = 2;   // also blue LED on most devkits
constexpr int PIN_LED_RED     = 12;
constexpr int PIN_LED_YELLOW  = 13;
constexpr int PIN_LED_GREEN   = 14;

constexpr int PIN_RGB_R = 32;
constexpr int PIN_RGB_G = 33;
constexpr int PIN_RGB_B = 27;

constexpr int PIN_BTN_1 = 18;
constexpr int PIN_BTN_2 = 19;
constexpr int PIN_BTN_3 = 23;
constexpr int PIN_BTN_4 = 5;
constexpr int PIN_BTN_5 = 15;
constexpr int PIN_BTN_6 = 0;   // shares the devkit BOOT button — fine after boot

// --- Phase 2: display + environment sensors ---
constexpr int PIN_I2C_SDA = 21;  // OLED SSD1306, addr 0x3C
constexpr int PIN_I2C_SCL = 22;
constexpr int PIN_DHT     = 4;   // DHT11 data
constexpr int PIN_LDR     = 34;  // photoresistor module AO (input-only, ADC1)
constexpr int PIN_POT     = 35;  // potentiometer wiper (input-only, ADC1)

// --- Phase 3: motion + alarm ---
constexpr int PIN_PIR      = 39;  // HC-SR501 OUT (input-only)
constexpr int PIN_OBSTACLE = 36;  // IR obstacle module OUT, active LOW (input-only)
constexpr int PIN_BUZZER_PASSIVE = 25;  // PWM tones
constexpr int PIN_BUZZER_ACTIVE  = 26;  // plain on/off beep

// --- Phase 4: relays ---
constexpr int PIN_RELAY_1 = 16;  // lamp channel (most modules are active LOW)
constexpr int PIN_RELAY_2 = 17;  // fan channel
