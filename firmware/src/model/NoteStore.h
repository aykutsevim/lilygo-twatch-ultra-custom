#pragma once
#include "Note.h"

namespace wt {

// Result of applying a server event, telling the caller what to do next.
enum class ApplyOutcome {
  Applied,      // state changed; UI should refresh
  Ignored,      // duplicate/echo/ping — no change
  NeedResync,   // gap or hello-behind — caller must GET /api/notes
};

// In-RAM mirror of the notes list with `rev` reconciliation. Pure logic, no
// Arduino/LVGL deps (see specs/03-api-contract.md §3.5, specs/07-realtime.md).
class NoteStore {
 public:
  // Replace the whole list from a REST snapshot.
  void applySnapshot(int64_t rev, std::vector<Note> notes);

  // Apply one server event according to the rev rules:
  //   rev <= current        -> Ignored (echo/duplicate)
  //   rev == current + 1     -> Applied
  //   rev  > current + 1     -> NeedResync (gap)
  //   Hello with rev>current -> NeedResync; Ping -> Ignored
  ApplyOutcome applyEvent(const ServerEvent& e);

  int64_t rev() const { return rev_; }
  const std::vector<Note>& notes() const { return notes_; }
  size_t size() const { return notes_.size(); }

 private:
  void upsert(const Note& n);
  void removeId(const std::string& id);
  void removeIds(const std::vector<std::string>& ids);
  void sortByCreated();

  std::vector<Note> notes_;
  int64_t rev_ = 0;
};

}  // namespace wt
