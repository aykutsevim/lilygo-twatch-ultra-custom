#pragma once
// Inactivity power ladder over the mode machine (specs/06-firmware.md §6.9).
// Owns its own idle timer; sets backlight and reports the desired tier. Main
// decides when to actually enter light/deep sleep (deep sleep does not return).
#include <Arduino.h>

#include "../storage/Settings.h"

namespace wt {

enum class PowerTier { Active, Dimmed, ScreenOff, DeepSleep };

class PowerManager {
 public:
  void begin(const Settings& s);
  void onActivity(uint32_t nowMs) { lastActivityMs_ = nowMs; }

  // Apply backlight for the current idle level and return the tier so the main
  // loop can enter light/deep sleep when appropriate.
  PowerTier tick(uint32_t nowMs, PowerProfile profile);

  // Call when network transfer is in flight to keep modem awake; otherwise the
  // manager requests Wi-Fi modem sleep to save power.
  void setNetworkBusy(bool busy) { netBusy_ = busy; }

 private:
  uint8_t activeBrightness_ = 200;
  uint32_t lastActivityMs_ = 0;
  bool netBusy_ = false;
};

}  // namespace wt
