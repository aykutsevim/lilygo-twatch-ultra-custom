#include "Provisioning.h"

#include <WiFi.h>

#include "../../include/config.h"

namespace wt {

void Provisioning::start() {
  WiFi.mode(WIFI_AP);
  if (strlen(AP_PASS) > 0)
    WiFi.softAP(AP_SSID, AP_PASS);
  else
    WiFi.softAP(AP_SSID);

  IPAddress ip = WiFi.softAPIP();
  dns_.start(53, "*", ip);  // captive: resolve everything to us

  server_.on("/", [this] { handleRoot(); });
  server_.on("/save", HTTP_POST, [this] { handleSave(); });
  server_.on("/status", [this] { handleStatus(); });
  server_.onNotFound([this] { handleRoot(); });  // captive redirect
  server_.begin();

  running_ = true;
  saved_ = false;
}

void Provisioning::stop() {
  if (!running_) return;
  server_.stop();
  dns_.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  running_ = false;
}

void Provisioning::loop() {
  if (!running_) return;
  dns_.processNextRequest();
  server_.handleClient();
}

bool Provisioning::clientConnected() const { return WiFi.softAPgetStationNum() > 0; }
String Provisioning::apIp() const { return WiFi.softAPIP().toString(); }

void Provisioning::handleRoot() {
  // Minimal page; scan results pre-rendered into a datalist.
  String nets;
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    nets += "<option value=\"" + WiFi.SSID(i) + "\">";
  }
  String html =
      "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
      "<title>WatchTodo setup</title>"
      "<h2>WatchTodo Wi-Fi setup</h2>"
      "<form method=POST action=/save>"
      "<p>Network<br><input name=ssid list=nets autocomplete=off required>"
      "<datalist id=nets>" + nets + "</datalist>"
      "<p>Password<br><input name=pass type=password>"
      "<p>API base<br><input name=api_base value='" + settings_.apiBase + "' size=40>"
      "<p>WS base<br><input name=ws_base value='" + settings_.wsBase + "' size=40>"
      "<p>API key<br><input name=api_key value='" + settings_.apiKey + "' size=40>"
      "<p><button type=submit>Save &amp; connect</button>"
      "</form>";
  server_.send(200, "text/html", html);
}

void Provisioning::handleSave() {
  settings_.wifiSsid = server_.arg("ssid");
  settings_.wifiPass = server_.arg("pass");
  if (server_.hasArg("api_base")) settings_.apiBase = server_.arg("api_base");
  if (server_.hasArg("ws_base")) settings_.wsBase = server_.arg("ws_base");
  if (server_.hasArg("api_key")) settings_.apiKey = server_.arg("api_key");
  settings_.save();
  saved_ = true;
  server_.send(200, "text/html",
               "<meta http-equiv=refresh content='3;url=/status'>Saved. Connecting…");
}

void Provisioning::handleStatus() {
  String json = String("{\"connected\":") + (WiFi.status() == WL_CONNECTED ? "true" : "false") +
                ",\"ip\":\"" + WiFi.localIP().toString() + "\"," +
                "\"rssi\":" + WiFi.RSSI() + "}";
  server_.send(200, "application/json", json);
}

}  // namespace wt
