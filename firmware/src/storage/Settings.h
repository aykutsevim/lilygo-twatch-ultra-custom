#pragma once
// Persistent config in NVS (Preferences). Provisioning writes these; they take
// precedence over build-time defaults. See specs/06-firmware.md §6.12.
#include <Arduino.h>

namespace wt {

enum class PowerProfile { Balanced, PowerSaver };

class Settings {
 public:
  void load();                 // read NVS, falling back to config.h defaults
  void save();                 // persist current values

  bool hasWifi() const { return wifiSsid.length() > 0; }

  String wifiSsid, wifiPass;
  String apiBase, wsBase, apiKey;
  uint8_t brightness = 200;
  PowerProfile profile = PowerProfile::Balanced;
};

}  // namespace wt
