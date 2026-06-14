#pragma once
// LVGL screen builders for the four modes + shared status bar. Kept in one
// module for brevity; maps to spec §6.4's ui_watch / ui_menu / ui_tasks /
// ui_provision / ui_status. These are the device's only LVGL-bound code and
// require the BSP display/touch setup to run.
#include <Arduino.h>

#include <functional>

#include "../model/NoteStore.h"

namespace wt::ui {

struct WatchFace {
  String time;        // "14:05"
  String date;        // "Sat, 14 Jun"
  String gps;         // "51.50, -0.12" or "--"
  int batteryPct = 100;
  bool charging = false;
  int conn = 0;       // 0 offline, 1 connecting, 2 online
};

struct MenuCallbacks {
  std::function<void()> onTasks;
  std::function<void()> onWifi;
  std::function<void()> onBack;
};

struct TaskCallbacks {
  std::function<void(const String& id)> onToggle;
  std::function<void(const String& id)> onDelete;
  std::function<void()> onBack;
};

void showWatch(const WatchFace& f);
void showMenu(const MenuCallbacks& cb);
void showTasks(const NoteStore& store, const TaskCallbacks& cb);
void refreshTasks(const NoteStore& store);   // update list in place
void showProvision(const String& apSsid, const String& url);
void updateProvision(bool connected, const String& ip);

}  // namespace wt::ui
