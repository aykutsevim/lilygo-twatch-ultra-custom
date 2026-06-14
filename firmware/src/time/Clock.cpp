#include "Clock.h"

#include <time.h>

#include "../hw/Board.h"

namespace wt {

void Clock::begin() {
  // Seed the C library time from the battery-backed RTC so we have a time even
  // before NTP / network.
  uint64_t ms = board::rtcEpochMs();
  if (ms > 0) {
    struct timeval tv {
      .tv_sec = static_cast<time_t>(ms / 1000),
      .tv_usec = static_cast<suseconds_t>((ms % 1000) * 1000)
    };
    settimeofday(&tv, nullptr);
  }
}

void Clock::syncNtpAsync() {
  // SNTP; once it lands, also push the corrected time back into the RTC.
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  // A small task/poll elsewhere can set synced_ = true and call
  // board::rtcSetEpochMs() once time() looks valid (year > 2023).
  time_t now = time(nullptr);
  if (now > 1700000000) {
    synced_ = true;
    board::rtcSetEpochMs(static_cast<uint64_t>(now) * 1000ULL);
  }
}

String Clock::timeHHMM() {
  time_t now = time(nullptr);
  struct tm tm;
  localtime_r(&now, &tm);
  char buf[8];
  strftime(buf, sizeof(buf), "%H:%M", &tm);
  return String(buf);
}

String Clock::dateLong() {
  time_t now = time(nullptr);
  struct tm tm;
  localtime_r(&now, &tm);
  char buf[20];
  strftime(buf, sizeof(buf), "%a, %d %b", &tm);
  return String(buf);
}

}  // namespace wt
