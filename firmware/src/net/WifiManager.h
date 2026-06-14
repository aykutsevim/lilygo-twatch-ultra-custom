#pragma once
// Station-mode Wi-Fi: connect, monitor, reconnect. See spec §6.8/§6.11.
#include <Arduino.h>

namespace wt {

class WifiManager {
 public:
  void connect(const String& ssid, const String& pass);
  void disconnect();
  void loop();                 // monitor + reconnect with backoff
  bool connected() const;
  String ip() const;
  int rssi() const;

 private:
  String ssid_, pass_;
  uint32_t lastAttemptMs_ = 0;
  uint32_t backoffMs_ = 1000;
};

}  // namespace wt
