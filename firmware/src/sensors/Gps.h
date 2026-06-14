#pragma once
// GPS provider: duty-cycled NMEA reader with a graceful "no source" mode.
// See specs/06-firmware.md §6.7 (GPS is power-hungry -> sample, cache, power off).
#include <Arduino.h>
#include <cstdint>

namespace wt {

struct GpsFix {
  bool valid = false;
  double lat = 0, lon = 0;
  uint32_t ageMs = 0;  // since acquired
};

class Gps {
 public:
  void begin();              // detect source via Board::hasGps()
  void loop(uint32_t nowMs); // duty cycle: power on, acquire, cache, power off
  GpsFix lastFix() const { return fix_; }
  bool available() const { return available_; }
  void setRefreshMs(uint32_t ms) { refreshMs_ = ms; }

 private:
  bool available_ = false;
  bool sampling_ = false;
  uint32_t refreshMs_ = 300000;
  uint32_t lastStartMs_ = 0;
  uint32_t fixAtMs_ = 0;
  GpsFix fix_;
};

}  // namespace wt
