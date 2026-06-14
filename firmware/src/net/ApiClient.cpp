#include "ApiClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "../../include/config.h"

namespace wt {

void ApiClient::configure(const String& apiBase, const String& apiKey) {
  base_ = apiBase;
  key_ = apiKey;
}

// Returns a TLS-capable client. Prefer pinning a CA; TLS_INSECURE is a
// documented hobby stopgap (see spec §6.12).
static WiFiClientSecure makeClient() {
  WiFiClientSecure c;
#if TLS_INSECURE
  c.setInsecure();
#else
  // c.setCACert(ROOT_CA_PEM);  // TODO: pin the host root CA
#endif
  return c;
}

bool ApiClient::fetchInto(NoteStore& store) {
  WiFiClientSecure client = makeClient();
  HTTPClient http;
  if (!http.begin(client, base_ + API_NOTES_PATH)) return false;
  http.addHeader("X-API-Key", key_);
  int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }

  // NOTE: for very large lists prefer a streaming/filtered parse; bounded to a
  // personal number of notes this elastic document is fine (text <= 280).
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) return false;

  int64_t rev = doc["rev"] | 0;
  std::vector<Note> notes;
  for (JsonObject o : doc["notes"].as<JsonArray>()) {
    Note n;
    n.id = o["id"].as<const char*>();
    n.text = o["text"].as<const char*>();
    n.done = o["done"] | false;
    n.created_at = o["created_at"] | 0;
    n.updated_at = o["updated_at"] | 0;
    notes.push_back(std::move(n));
  }
  store.applySnapshot(rev, std::move(notes));
  return true;
}

bool ApiClient::patchDone(const String& id, bool done) {
  WiFiClientSecure client = makeClient();
  HTTPClient http;
  if (!http.begin(client, base_ + API_NOTES_PATH + "/" + id)) return false;
  http.addHeader("X-API-Key", key_);
  http.addHeader("Content-Type", "application/json");
  String body = String("{\"done\":") + (done ? "true" : "false") + "}";
  int code = http.sendRequest("PATCH", body);
  http.end();
  return code == 200;
}

bool ApiClient::remove(const String& id) {
  WiFiClientSecure client = makeClient();
  HTTPClient http;
  if (!http.begin(client, base_ + API_NOTES_PATH + "/" + id)) return false;
  http.addHeader("X-API-Key", key_);
  int code = http.sendRequest("DELETE");
  http.end();
  return code == 204;
}

}  // namespace wt
