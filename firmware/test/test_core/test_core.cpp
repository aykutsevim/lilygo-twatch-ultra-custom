// Native (host) tests for the Arduino-free core logic.  Run: pio test -e native
#include <unity.h>

#include "app/ModeManager.h"
#include "model/NoteStore.h"

using namespace wt;

void setUp() {}
void tearDown() {}

// ───────────────────────── NoteStore: rev reconciliation ──────────────────
static Note mk(const std::string& id, int64_t created, bool done = false) {
  Note n;
  n.id = id;
  n.text = id;
  n.done = done;
  n.created_at = created;
  n.updated_at = created;
  return n;
}

static ServerEvent ev(EventType t, int64_t rev) {
  ServerEvent e;
  e.type = t;
  e.rev = rev;
  return e;
}

void test_snapshot_sorts_by_created() {
  NoteStore s;
  s.applySnapshot(3, {mk("b", 200), mk("a", 100)});
  TEST_ASSERT_EQUAL_INT64(3, s.rev());
  TEST_ASSERT_EQUAL_size_t(2, s.size());
  TEST_ASSERT_EQUAL_STRING("a", s.notes()[0].id.c_str());
  TEST_ASSERT_EQUAL_STRING("b", s.notes()[1].id.c_str());
}

void test_created_applies_when_next_rev() {
  NoteStore s;
  s.applySnapshot(1, {});
  ServerEvent e = ev(EventType::Created, 2);
  e.note = mk("n1", 10);
  TEST_ASSERT_EQUAL(ApplyOutcome::Applied, s.applyEvent(e));
  TEST_ASSERT_EQUAL_INT64(2, s.rev());
  TEST_ASSERT_EQUAL_size_t(1, s.size());
}

void test_duplicate_or_stale_event_ignored() {
  NoteStore s;
  s.applySnapshot(5, {mk("n1", 10)});
  TEST_ASSERT_EQUAL(ApplyOutcome::Ignored, s.applyEvent(ev(EventType::Updated, 5)));
  TEST_ASSERT_EQUAL(ApplyOutcome::Ignored, s.applyEvent(ev(EventType::Updated, 3)));
  TEST_ASSERT_EQUAL_INT64(5, s.rev());
}

void test_gap_requests_resync() {
  NoteStore s;
  s.applySnapshot(5, {});
  // rev jumps from 5 to 8 -> gap.
  TEST_ASSERT_EQUAL(ApplyOutcome::NeedResync, s.applyEvent(ev(EventType::Created, 8)));
  TEST_ASSERT_EQUAL_INT64(5, s.rev());  // unchanged until resync
}

void test_hello_behind_requests_resync() {
  NoteStore s;
  s.applySnapshot(2, {});
  TEST_ASSERT_EQUAL(ApplyOutcome::NeedResync, s.applyEvent(ev(EventType::Hello, 9)));
  TEST_ASSERT_EQUAL(ApplyOutcome::Ignored, s.applyEvent(ev(EventType::Hello, 2)));
}

void test_updated_mutates_in_place() {
  NoteStore s;
  s.applySnapshot(1, {mk("n1", 10, false)});
  ServerEvent e = ev(EventType::Updated, 2);
  e.note = mk("n1", 10, true);
  TEST_ASSERT_EQUAL(ApplyOutcome::Applied, s.applyEvent(e));
  TEST_ASSERT_TRUE(s.notes()[0].done);
  TEST_ASSERT_EQUAL_size_t(1, s.size());
}

void test_deleted_and_cleared() {
  NoteStore s;
  s.applySnapshot(1, {mk("a", 10), mk("b", 20), mk("c", 30)});

  ServerEvent del = ev(EventType::Deleted, 2);
  del.id = "b";
  TEST_ASSERT_EQUAL(ApplyOutcome::Applied, s.applyEvent(del));
  TEST_ASSERT_EQUAL_size_t(2, s.size());

  ServerEvent cl = ev(EventType::Cleared, 3);
  cl.ids = {"a", "c"};
  TEST_ASSERT_EQUAL(ApplyOutcome::Applied, s.applyEvent(cl));
  TEST_ASSERT_EQUAL_size_t(0, s.size());
}

void test_ping_ignored() {
  NoteStore s;
  s.applySnapshot(4, {});
  TEST_ASSERT_EQUAL(ApplyOutcome::Ignored, s.applyEvent(ev(EventType::Ping, 0)));
  TEST_ASSERT_EQUAL_INT64(4, s.rev());
}

// ───────────────────────── ModeManager: transitions ───────────────────────
void test_watch_tap_opens_menu_then_selects() {
  ModeManager m(30000);
  TEST_ASSERT_EQUAL(Mode::Watch, m.mode());
  TEST_ASSERT_EQUAL(Mode::Menu, m.handle(Input::Tap, 1000));
  TEST_ASSERT_EQUAL(Mode::Tasks, m.handle(Input::SelectTasks, 1100));
}

void test_menu_selects_wifi_provisioning() {
  ModeManager m(30000);
  m.handle(Input::Tap, 0);
  TEST_ASSERT_EQUAL(Mode::Provisioning, m.handle(Input::SelectWifi, 50));
}

void test_back_returns_to_watch() {
  ModeManager m(30000);
  m.handle(Input::Tap, 0);
  m.handle(Input::SelectTasks, 10);
  TEST_ASSERT_EQUAL(Mode::Watch, m.handle(Input::Back, 20));
}

void test_idle_auto_returns_to_watch() {
  ModeManager m(30000);
  m.handle(Input::Tap, 1000);                 // -> Menu at t=1000
  TEST_ASSERT_EQUAL(Mode::Menu, m.tick(20000));   // not yet (19s)
  TEST_ASSERT_EQUAL(Mode::Watch, m.tick(31001));  // 30s elapsed -> Watch
}

void test_activity_defers_auto_return() {
  ModeManager m(30000);
  m.handle(Input::Tap, 1000);
  m.onActivity(25000);                         // fresh activity
  TEST_ASSERT_EQUAL(Mode::Menu, m.tick(50000));    // 25s since activity
  TEST_ASSERT_EQUAL(Mode::Watch, m.tick(56000));   // now 31s -> Watch
}

void test_provisioning_client_suspends_auto_return() {
  ModeManager m(30000);
  m.handle(Input::Tap, 0);
  m.handle(Input::SelectWifi, 10);
  m.setProvisioningClientActive(true);
  TEST_ASSERT_EQUAL(Mode::Provisioning, m.tick(100000));  // stays while client on AP
  m.setProvisioningClientActive(false);
  TEST_ASSERT_EQUAL(Mode::Watch, m.tick(200000));         // now times out
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_snapshot_sorts_by_created);
  RUN_TEST(test_created_applies_when_next_rev);
  RUN_TEST(test_duplicate_or_stale_event_ignored);
  RUN_TEST(test_gap_requests_resync);
  RUN_TEST(test_hello_behind_requests_resync);
  RUN_TEST(test_updated_mutates_in_place);
  RUN_TEST(test_deleted_and_cleared);
  RUN_TEST(test_ping_ignored);
  RUN_TEST(test_watch_tap_opens_menu_then_selects);
  RUN_TEST(test_menu_selects_wifi_provisioning);
  RUN_TEST(test_back_returns_to_watch);
  RUN_TEST(test_idle_auto_returns_to_watch);
  RUN_TEST(test_activity_defers_auto_return);
  RUN_TEST(test_provisioning_client_suspends_auto_return);
  return UNITY_END();
}
