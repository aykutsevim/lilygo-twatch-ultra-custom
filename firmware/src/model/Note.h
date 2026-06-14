#pragma once
// Arduino-free data types so the core logic is unit-testable on the host.
#include <cstdint>
#include <string>
#include <vector>

namespace wt {

struct Note {
  std::string id;
  std::string text;
  bool done = false;
  int64_t created_at = 0;  // epoch ms
  int64_t updated_at = 0;  // epoch ms
};

// Server -> client event, already parsed from JSON by the transport layer.
enum class EventType { Hello, Created, Updated, Deleted, Cleared, Ping, Unknown };

struct ServerEvent {
  EventType type = EventType::Unknown;
  int64_t rev = 0;                  // revision stamped by the server
  Note note;                        // for Created / Updated
  std::string id;                   // for Deleted
  std::vector<std::string> ids;     // for Cleared
};

}  // namespace wt
