#pragma once
// On-watch Wi-Fi setup: SoftAP + DNS captive portal + web form (spec §6.10).
// Saves Wi-Fi/API config to NVS, then the app switches to STA.
#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

#include "../storage/Settings.h"

namespace wt {

class Provisioning {
 public:
  explicit Provisioning(Settings& settings) : settings_(settings) {}

  void start();                       // bring up AP + DNS + HTTP
  void stop();                        // tear down, return radio to STA
  void loop();                        // service DNS + HTTP
  bool clientConnected() const;       // a phone/laptop is on the AP
  bool saved() const { return saved_; }  // user submitted creds
  String apIp() const;

 private:
  void handleRoot();
  void handleSave();
  void handleStatus();

  Settings& settings_;
  DNSServer dns_;
  WebServer server_{80};
  bool running_ = false;
  bool saved_ = false;
};

}  // namespace wt
