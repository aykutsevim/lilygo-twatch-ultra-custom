#pragma once
// WebSocket client for /ws/notes/ (specs/03-api-contract.md §3.4). Parses frames
// into wt::ServerEvent and hands them to a callback; auto-reconnects.
#include <Arduino.h>
#include <WebSocketsClient.h>

#include <functional>

#include "../model/Note.h"

namespace wt {

class NotesSocket {
 public:
  using Handler = std::function<void(const ServerEvent&)>;

  void begin(const String& wsBase, const String& apiKey, Handler onEvent);
  void loop();
  void stop();
  bool connected() const { return connected_; }

  // Light client->server actions (watch only; web uses REST).
  void sendToggle(const String& id);
  void sendDelete(const String& id);

 private:
  void onWsEvent(WStype_t type, uint8_t* payload, size_t len);

  WebSocketsClient ws_;
  Handler handler_;
  bool connected_ = false;
};

}  // namespace wt
