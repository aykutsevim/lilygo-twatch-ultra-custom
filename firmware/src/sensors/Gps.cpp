#include "Gps.h"

#include <TinyGPSPlus.h>

#include "../hw/Board.h"

namespace wt {

static TinyGPSPlus s_gps;
static const uint32_t kAcquireWindowMs = 60000;  // bounded fix attempt

void Gps::begin() {
  available_ = board::hasGps();
}

void Gps::loop(uint32_t nowMs) {
  if (!available_) return;

  // Start a sampling window every refreshMs_.
  if (!sampling_ && (nowMs - lastStartMs_) >= refreshMs_) {
    sampling_ = true;
    lastStartMs_ = nowMs;
    board::gpsPower(true);
  }
  if (!sampling_) return;

  Stream* s = board::gpsStream();
  while (s && s->available()) {
    if (s_gps.encode(s->read()) && s_gps.location.isValid()) {
      fix_ = {true, s_gps.location.lat(), s_gps.location.lng(), 0};
      fixAtMs_ = nowMs;
      sampling_ = false;        // got a fix; power down to save energy
      board::gpsPower(false);
      return;
    }
  }
  // Give up this window after the acquire timeout; keep the last cached fix.
  if ((nowMs - lastStartMs_) >= kAcquireWindowMs) {
    sampling_ = false;
    board::gpsPower(false);
  }
}

}  // namespace wt
