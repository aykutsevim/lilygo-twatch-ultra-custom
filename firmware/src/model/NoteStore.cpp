#include "NoteStore.h"

#include <algorithm>

namespace wt {

void NoteStore::sortByCreated() {
  std::sort(notes_.begin(), notes_.end(), [](const Note& a, const Note& b) {
    return a.created_at < b.created_at;
  });
}

void NoteStore::upsert(const Note& n) {
  for (auto& existing : notes_) {
    if (existing.id == n.id) {
      existing = n;
      sortByCreated();
      return;
    }
  }
  notes_.push_back(n);
  sortByCreated();
}

void NoteStore::removeId(const std::string& id) {
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(),
                              [&](const Note& n) { return n.id == id; }),
               notes_.end());
}

void NoteStore::removeIds(const std::vector<std::string>& ids) {
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(),
                              [&](const Note& n) {
                                return std::find(ids.begin(), ids.end(), n.id) !=
                                       ids.end();
                              }),
               notes_.end());
}

void NoteStore::applySnapshot(int64_t rev, std::vector<Note> notes) {
  notes_ = std::move(notes);
  rev_ = rev;
  sortByCreated();
}

ApplyOutcome NoteStore::applyEvent(const ServerEvent& e) {
  if (e.type == EventType::Ping || e.type == EventType::Unknown) {
    return ApplyOutcome::Ignored;
  }
  if (e.type == EventType::Hello) {
    return e.rev > rev_ ? ApplyOutcome::NeedResync : ApplyOutcome::Ignored;
  }
  if (e.rev <= rev_) return ApplyOutcome::Ignored;        // echo / duplicate
  if (e.rev > rev_ + 1) return ApplyOutcome::NeedResync;  // gap

  switch (e.type) {
    case EventType::Created:
    case EventType::Updated:
      upsert(e.note);
      break;
    case EventType::Deleted:
      removeId(e.id);
      break;
    case EventType::Cleared:
      removeIds(e.ids);
      break;
    default:
      return ApplyOutcome::Ignored;
  }
  rev_ = e.rev;
  return ApplyOutcome::Applied;
}

}  // namespace wt
