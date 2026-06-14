#include "ModeManager.h"

namespace wt {

Mode ModeManager::handle(Input in, uint32_t nowMs) {
  switch (in) {
    case Input::Tap:
      // A tap on the watch face opens the menu; elsewhere it is just activity.
      if (mode_ == Mode::Watch) enter(Mode::Menu, nowMs);
      else onActivity(nowMs);
      break;
    case Input::Back:
      // Back returns home from any mode.
      enter(Mode::Watch, nowMs);
      break;
    case Input::SelectTasks:
      if (mode_ == Mode::Menu) enter(Mode::Tasks, nowMs);
      else onActivity(nowMs);
      break;
    case Input::SelectWifi:
      if (mode_ == Mode::Menu) enter(Mode::Provisioning, nowMs);
      else onActivity(nowMs);
      break;
  }
  return mode_;
}

Mode ModeManager::tick(uint32_t nowMs) {
  if (mode_ == Mode::Watch) return mode_;
  const bool provBusy = (mode_ == Mode::Provisioning && provClient_);
  if (!provBusy && (nowMs - lastActivityMs_) >= idleReturnMs_) {
    enter(Mode::Watch, nowMs);
  }
  return mode_;
}

}  // namespace wt
