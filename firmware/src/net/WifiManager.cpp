#include "WifiManager.h"

#include <WiFi.h>

namespace wt {

void WifiManager::connect(const String& ssid, const String& pass) {
  ssid_ = ssid;
  pass_ = pass;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);  // allow modem sleep; PowerManager tunes the ps mode
  WiFi.begin(ssid_.c_str(), pass_.c_str());
  lastAttemptMs_ = millis();
  backoffMs_ = 1000;
}

void WifiManager::disconnect() {
  WiFi.disconnect(/*wifioff=*/true);
}

void WifiManager::loop() {
  if (ssid_.isEmpty() || connected()) {
    backoffMs_ = 1000;
    return;
  }
  uint32_t now = millis();
  if (now - lastAttemptMs_ >= backoffMs_) {
    WiFi.begin(ssid_.c_str(), pass_.c_str());
    lastAttemptMs_ = now;
    backoffMs_ = min<uint32_t>(backoffMs_ * 2, 30000);  // exp backoff -> 30s
  }
}

bool WifiManager::connected() const { return WiFi.status() == WL_CONNECTED; }
String WifiManager::ip() const { return WiFi.localIP().toString(); }
int WifiManager::rssi() const { return WiFi.RSSI(); }

}  // namespace wt
