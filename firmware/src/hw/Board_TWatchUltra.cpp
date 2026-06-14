// LilyGo T-Watch Ultra implementation of the Board facade.
//
// INTEGRATION POINT: wire each function to the official LilyGo T-Watch Ultra
// BSP / library (PMU, AMOLED + touch, RTC, IMU, LVGL hooks). The bodies below
// are structured stubs with the intended calls marked `// TODO(BSP)`. Pin maps
// and driver init come from the BSP example — do not hand-roll them.
#include "Board.h"

#include "../../include/config.h"

// #include "LilyGo_TWatch_Ultra.h"   // TODO(BSP): official board library
// #include <lvgl.h>                   // LVGL is set up via the BSP example

namespace wt::board {

void begin() {
  // TODO(BSP): power on rails via PMU; init AMOLED panel + touch; init RTC, IMU;
  // init LVGL display buffers in PSRAM and register flush + touchpad read
  // callbacks (copy from the official example). Set initial backlight.
  setBacklight(200);
}

void loop() {
  // TODO(BSP): service PMU/IMU IRQs if the library needs periodic polling.
}

void setBacklight(uint8_t level) {
  (void)level;  // TODO(BSP): PMU/backlight brightness (0 = off).
}

int batteryPercent() {
  return 100;  // TODO(BSP): read fuel gauge / PMU battery percent.
}

bool isCharging() {
  return false;  // TODO(BSP): PMU charge status.
}

uint64_t rtcEpochMs() {
  return 0;  // TODO(BSP): read RTC -> epoch ms.
}

void rtcSetEpochMs(uint64_t ms) {
  (void)ms;  // TODO(BSP): set RTC from epoch ms (after NTP).
}

bool hasGps() {
  return false;  // TODO(BSP): true only if a GNSS source is wired (see spec §6.1).
}

Stream* gpsStream() {
  return nullptr;  // TODO(BSP): return the NMEA UART (e.g. &Serial1) when present.
}

void gpsPower(bool on) {
  (void)on;  // TODO(BSP): gate GPS module power for duty-cycling.
}

void configureWakeSources() {
  // TODO(BSP): esp_sleep_enable_ext1/gpio wakeup for touch IRQ + side button,
  // IMU tilt interrupt, and esp_sleep_enable_timer_wakeup / RTC alarm.
}

void prepareLightSleep() {
  // TODO(BSP): ensure peripherals are safe for automatic light sleep.
}

void prepareDeepSleep(uint64_t wakeAfterMs) {
  (void)wakeAfterMs;  // TODO(BSP): set timer wake, then esp_deep_sleep_start().
}

bool buttonPressed() {
  return false;  // TODO(BSP): debounced side-button state.
}

}  // namespace wt::board
