#pragma once
// Hardware facade. ALL T-Watch Ultra / LilyGo-BSP specifics live behind this
// interface so the rest of the app stays portable and the BSP wiring is in one
// place (Board_TWatchUltra.cpp). See specs/06-firmware.md §6.1.
#include <Arduino.h>
#include <cstdint>

namespace wt::board {

void begin();                 // PMU, display, touch, RTC, IMU, backlight, LVGL hook
void loop();                  // service anything the BSP needs each iteration

// Display / backlight
void setBacklight(uint8_t level);   // 0..255 (0 = off)

// Power / battery
int batteryPercent();         // 0..100
bool isCharging();

// Real-time clock (survives deep sleep)
uint64_t rtcEpochMs();
void rtcSetEpochMs(uint64_t ms);

// GPS source (may be absent on this unit — see hasGps()).
bool hasGps();
Stream* gpsStream();          // NMEA stream, or nullptr if no source
void gpsPower(bool on);       // gate the module for duty-cycling

// Sleep / wake
void configureWakeSources();  // touch IRQ, side button, IMU tilt, RTC alarm
void prepareLightSleep();
void prepareDeepSleep(uint64_t wakeAfterMs);  // 0 = no timed wake
bool buttonPressed();

}  // namespace wt::board
