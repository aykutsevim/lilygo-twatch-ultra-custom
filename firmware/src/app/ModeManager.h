#pragma once
// Arduino-free mode state machine + inactivity timer (specs/06-firmware.md §6.2).
// Time is passed in (epoch/uptime ms) so it is deterministic and testable.
#include <cstdint>

namespace wt {

enum class Mode { Watch, Menu, Tasks, Provisioning };

// User/router inputs that can change the mode.
enum class Input { Tap, Back, SelectTasks, SelectWifi };

class ModeManager {
 public:
  explicit ModeManager(uint32_t idleReturnMs) : idleReturnMs_(idleReturnMs) {}

  Mode mode() const { return mode_; }

  // Register any user input (resets the idle timer) without a transition.
  void onActivity(uint32_t nowMs) { lastActivityMs_ = nowMs; }

  // Apply an input event; also counts as activity. Returns the resulting mode.
  Mode handle(Input in, uint32_t nowMs);

  // Call frequently. Auto-returns to Watch after idleReturnMs of no activity
  // (suspended in Provisioning while a client is connected). Returns the mode.
  Mode tick(uint32_t nowMs);

  // Provisioning suspends auto-return while a phone/laptop is on the AP.
  void setProvisioningClientActive(bool active) { provClient_ = active; }

 private:
  void enter(Mode m, uint32_t nowMs) {
    mode_ = m;
    lastActivityMs_ = nowMs;
  }

  Mode mode_ = Mode::Watch;
  uint32_t idleReturnMs_;
  uint32_t lastActivityMs_ = 0;
  bool provClient_ = false;
};

}  // namespace wt
