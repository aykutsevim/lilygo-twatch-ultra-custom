#pragma once
// Timekeeping: RTC-backed, corrected by NTP when online. See spec §6.5/§6.11.
#include <Arduino.h>

namespace wt {

class Clock {
 public:
  void begin();                       // seed from the RTC
  void syncNtpAsync();                // kick off NTP (call when Wi-Fi is up)
  bool synced() const { return synced_; }

  // Formatted local strings for the watch face.
  String timeHHMM();                  // "14:05"
  String dateLong();                  // "Sat, 14 Jun"

 private:
  bool synced_ = false;
};

}  // namespace wt
