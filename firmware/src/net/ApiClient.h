#pragma once
// REST client for the WatchTodo API (specs/03-api-contract.md §3.3). Used for
// the initial/resync snapshot and (optionally) light actions.
#include <Arduino.h>

#include "../model/NoteStore.h"

namespace wt {

class ApiClient {
 public:
  void configure(const String& apiBase, const String& apiKey);

  // GET /api/notes -> applySnapshot into the store. Returns true on success.
  bool fetchInto(NoteStore& store);

  // Optional light actions (the watch normally uses the WebSocket for these).
  bool patchDone(const String& id, bool done);
  bool remove(const String& id);

 private:
  String base_, key_;
};

}  // namespace wt
