// Standalone host verification of the Arduino-free core logic, with no test
// framework, so it compiles with a bare g++ (e.g. in a gcc Docker image):
//
//   g++ -std=c++17 -I src tools/host_check.cpp \
//       src/model/NoteStore.cpp src/app/ModeManager.cpp -o /tmp/hc && /tmp/hc
//
// The canonical test suite is test/test_core (PlatformIO + Unity, `pio test
// -e native`); this mirrors it for environments without a system compiler.
#include <cstdio>
#include <string>

#include "app/ModeManager.h"
#include "model/NoteStore.h"

using namespace wt;

static int g_fail = 0;
#define CHECK(cond)                                                      \
  do {                                                                   \
    if (!(cond)) {                                                       \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
      ++g_fail;                                                          \
    }                                                                    \
  } while (0)

static Note mk(const std::string& id, int64_t created, bool done = false) {
  Note n;
  n.id = id; n.text = id; n.done = done;
  n.created_at = created; n.updated_at = created;
  return n;
}
static ServerEvent ev(EventType t, int64_t rev) {
  ServerEvent e; e.type = t; e.rev = rev; return e;
}

int main() {
  // ── NoteStore: snapshot + rev reconciliation ──
  {
    NoteStore s;
    s.applySnapshot(3, {mk("b", 200), mk("a", 100)});
    CHECK(s.rev() == 3);
    CHECK(s.size() == 2);
    CHECK(s.notes()[0].id == "a");   // sorted by created_at
    CHECK(s.notes()[1].id == "b");
  }
  {
    NoteStore s; s.applySnapshot(1, {});
    ServerEvent e = ev(EventType::Created, 2); e.note = mk("n1", 10);
    CHECK(s.applyEvent(e) == ApplyOutcome::Applied);
    CHECK(s.rev() == 2 && s.size() == 1);
  }
  {
    NoteStore s; s.applySnapshot(5, {mk("n1", 10)});
    CHECK(s.applyEvent(ev(EventType::Updated, 5)) == ApplyOutcome::Ignored);  // echo
    CHECK(s.applyEvent(ev(EventType::Updated, 3)) == ApplyOutcome::Ignored);  // stale
    CHECK(s.applyEvent(ev(EventType::Created, 8)) == ApplyOutcome::NeedResync);// gap
    CHECK(s.rev() == 5);
  }
  {
    NoteStore s; s.applySnapshot(2, {});
    CHECK(s.applyEvent(ev(EventType::Hello, 9)) == ApplyOutcome::NeedResync);
    CHECK(s.applyEvent(ev(EventType::Hello, 2)) == ApplyOutcome::Ignored);
    CHECK(s.applyEvent(ev(EventType::Ping, 0)) == ApplyOutcome::Ignored);
  }
  {
    NoteStore s; s.applySnapshot(1, {mk("n1", 10, false)});
    ServerEvent e = ev(EventType::Updated, 2); e.note = mk("n1", 10, true);
    CHECK(s.applyEvent(e) == ApplyOutcome::Applied);
    CHECK(s.notes()[0].done == true && s.size() == 1);
  }
  {
    NoteStore s; s.applySnapshot(1, {mk("a", 10), mk("b", 20), mk("c", 30)});
    ServerEvent del = ev(EventType::Deleted, 2); del.id = "b";
    CHECK(s.applyEvent(del) == ApplyOutcome::Applied && s.size() == 2);
    ServerEvent cl = ev(EventType::Cleared, 3); cl.ids = {"a", "c"};
    CHECK(s.applyEvent(cl) == ApplyOutcome::Applied && s.size() == 0);
  }

  // ── ModeManager: transitions + idle auto-return ──
  {
    ModeManager m(30000);
    CHECK(m.mode() == Mode::Watch);
    CHECK(m.handle(Input::Tap, 1000) == Mode::Menu);
    CHECK(m.handle(Input::SelectTasks, 1100) == Mode::Tasks);
    CHECK(m.handle(Input::Back, 1200) == Mode::Watch);
  }
  {
    ModeManager m(30000);
    m.handle(Input::Tap, 0);
    CHECK(m.handle(Input::SelectWifi, 50) == Mode::Provisioning);
  }
  {
    ModeManager m(30000);
    m.handle(Input::Tap, 1000);                       // -> Menu at t=1000
    CHECK(m.tick(20000) == Mode::Menu);               // 19s: stay
    CHECK(m.tick(31001) == Mode::Watch);              // 30s: auto-return
  }
  {
    ModeManager m(30000);
    m.handle(Input::Tap, 1000);
    m.onActivity(25000);                              // defer
    CHECK(m.tick(50000) == Mode::Menu);
    CHECK(m.tick(56000) == Mode::Watch);
  }
  {
    ModeManager m(30000);
    m.handle(Input::Tap, 0);
    m.handle(Input::SelectWifi, 10);
    m.setProvisioningClientActive(true);
    CHECK(m.tick(100000) == Mode::Provisioning);      // suspended while client on AP
    m.setProvisioningClientActive(false);
    CHECK(m.tick(200000) == Mode::Watch);
  }

  if (g_fail == 0) std::printf("PASS: all core-logic checks green\n");
  else std::printf("%d CHECK(s) FAILED\n", g_fail);
  return g_fail == 0 ? 0 : 1;
}
