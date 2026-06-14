#include "NotesSocket.h"

#include <ArduinoJson.h>

#include "../../include/config.h"

namespace wt {

static EventType parseType(const char* t) {
  if (!t) return EventType::Unknown;
  if (!strcmp(t, "hello")) return EventType::Hello;
  if (!strcmp(t, "note.created")) return EventType::Created;
  if (!strcmp(t, "note.updated")) return EventType::Updated;
  if (!strcmp(t, "note.deleted")) return EventType::Deleted;
  if (!strcmp(t, "notes.cleared")) return EventType::Cleared;
  if (!strcmp(t, "ping")) return EventType::Ping;
  return EventType::Unknown;
}

void NotesSocket::begin(const String& wsBase, const String& apiKey, Handler onEvent) {
  handler_ = std::move(onEvent);

  // Parse "wss://host[:port]" / "ws://host[:port]".
  bool ssl = wsBase.startsWith("wss://");
  String rest = wsBase.substring(ssl ? 6 : 5);
  String host = rest;
  uint16_t port = ssl ? 443 : 80;
  int colon = rest.indexOf(':');
  if (colon >= 0) {
    host = rest.substring(0, colon);
    port = rest.substring(colon + 1).toInt();
  }
  String path = String(WS_PATH) + "?key=" + apiKey;

  ws_.onEvent([this](WStype_t type, uint8_t* payload, size_t len) {
    onWsEvent(type, payload, len);
  });
  if (ssl) {
#if TLS_INSECURE
    ws_.beginSSL(host.c_str(), port, path.c_str());  // lib validates per build
#else
    ws_.beginSSL(host.c_str(), port, path.c_str());  // TODO: pin CA via beginSslWithCA
#endif
  } else {
    ws_.begin(host.c_str(), port, path.c_str());
  }
  ws_.setReconnectInterval(2000);
  ws_.enableHeartbeat(15000, 3000, 2);  // app-level keepalive
}

void NotesSocket::loop() { ws_.loop(); }

void NotesSocket::stop() {
  ws_.disconnect();
  connected_ = false;
}

void NotesSocket::onWsEvent(WStype_t type, uint8_t* payload, size_t len) {
  switch (type) {
    case WStype_CONNECTED:
      connected_ = true;
      break;
    case WStype_DISCONNECTED:
      connected_ = false;
      break;
    case WStype_TEXT: {
      JsonDocument doc;
      if (deserializeJson(doc, payload, len)) return;
      ServerEvent e;
      e.type = parseType(doc["type"].as<const char*>());
      if (e.type == EventType::Ping) {
        ws_.sendTXT("{\"type\":\"pong\"}");
        return;
      }
      e.rev = doc["rev"] | 0;
      if (e.type == EventType::Created || e.type == EventType::Updated) {
        JsonObject o = doc["note"];
        e.note.id = o["id"].as<const char*>();
        e.note.text = o["text"].as<const char*>();
        e.note.done = o["done"] | false;
        e.note.created_at = o["created_at"] | 0;
        e.note.updated_at = o["updated_at"] | 0;
      } else if (e.type == EventType::Deleted) {
        e.id = doc["id"].as<const char*>();
      } else if (e.type == EventType::Cleared) {
        for (JsonVariant v : doc["ids"].as<JsonArray>())
          e.ids.push_back(v.as<const char*>());
      }
      if (handler_) handler_(e);
      break;
    }
    default:
      break;
  }
}

void NotesSocket::sendToggle(const String& id) {
  ws_.sendTXT(String("{\"type\":\"toggle\",\"id\":\"") + id + "\"}");
}

void NotesSocket::sendDelete(const String& id) {
  ws_.sendTXT(String("{\"type\":\"delete\",\"id\":\"") + id + "\"}");
}

}  // namespace wt
