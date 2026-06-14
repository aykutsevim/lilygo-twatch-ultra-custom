#include "PowerManager.h"

#include "../../include/config.h"
#include "../hw/Board.h"

// ESP-IDF power-management headers (available under Arduino-ESP32).
#if defined(ESP_PLATFORM)
#include "esp_pm.h"
#include "esp_wifi.h"
#endif

namespace wt {

void PowerManager::begin(const Settings& s) {
  activeBrightness_ = s.brightness;
  board::configureWakeSources();

#if defined(ESP_PLATFORM)
  // Dynamic frequency scaling + automatic light sleep: the CPU idles between
  // the 1 Hz watch tick and sensor reads instead of spinning.
  esp_pm_config_t pm{};
  pm.max_freq_mhz = 240;
  pm.min_freq_mhz = 80;
  pm.light_sleep_enable = true;
  esp_pm_configure(&pm);
#endif
  board::setBacklight(activeBrightness_);
}

PowerTier PowerManager::tick(uint32_t nowMs, PowerProfile profile) {
  const uint32_t idle = nowMs - lastActivityMs_;

#if defined(ESP_PLATFORM)
  // Wi-Fi modem sleep whenever idle and not mid-transfer.
  esp_wifi_set_ps(netBusy_ ? WIFI_PS_NONE : WIFI_PS_MAX_MODEM);
#endif

  if (idle < DIM_MS) {
    board::setBacklight(activeBrightness_);
    return PowerTier::Active;
  }
  if (idle < SLEEP_MS) {
    board::setBacklight(activeBrightness_ / 5);  // dimmed
    return PowerTier::Dimmed;
  }
  // Screen off.
  board::setBacklight(0);
  if (profile == PowerProfile::PowerSaver && idle >= DEEP_SLEEP_MS) {
    return PowerTier::DeepSleep;
  }
  return PowerTier::ScreenOff;
}

}  // namespace wt
