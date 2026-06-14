#include "Settings.h"

#include <Preferences.h>

#include "../../include/config.h"

namespace wt {

static const char* NS = "watchtodo";

void Settings::load() {
  Preferences p;
  p.begin(NS, /*readOnly=*/true);
  wifiSsid = p.getString("wifi_ssid", DEFAULT_WIFI_SSID);
  wifiPass = p.getString("wifi_pass", DEFAULT_WIFI_PASS);
  apiBase = p.getString("api_base", DEFAULT_API_BASE);
  wsBase = p.getString("ws_base", DEFAULT_WS_BASE);
  apiKey = p.getString("api_key", DEFAULT_API_KEY);
  brightness = p.getUChar("bright", 200);
  profile = static_cast<PowerProfile>(p.getUChar("profile", 0));
  p.end();
}

void Settings::save() {
  Preferences p;
  p.begin(NS, /*readOnly=*/false);
  p.putString("wifi_ssid", wifiSsid);
  p.putString("wifi_pass", wifiPass);
  p.putString("api_base", apiBase);
  p.putString("ws_base", wsBase);
  p.putString("api_key", apiKey);
  p.putUChar("bright", brightness);
  p.putUChar("profile", static_cast<uint8_t>(profile));
  p.end();
}

}  // namespace wt
