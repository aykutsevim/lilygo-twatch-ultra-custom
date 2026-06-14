// LVGL screen builders. The structure (objects, labels, list, event callbacks)
// is in place; styling/layout details are marked TODO and assume the BSP has
// initialised LVGL (display flush + touch read) per the official example.
#include "Screens.h"

#include <lvgl.h>

namespace wt::ui {

// Held so refreshTasks() can update the existing list without a rebuild.
static lv_obj_t* s_taskList = nullptr;
static const TaskCallbacks* s_taskCb = nullptr;
static lv_obj_t* s_provStatus = nullptr;

void showWatch(const WatchFace& f) {
  lv_obj_t* scr = lv_obj_create(nullptr);

  lv_obj_t* time = lv_label_create(scr);
  lv_label_set_text(time, f.time.c_str());
  lv_obj_align(time, LV_ALIGN_CENTER, 0, -40);
  // TODO: large clock font.

  lv_obj_t* date = lv_label_create(scr);
  lv_label_set_text(date, f.date.c_str());
  lv_obj_align(date, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t* gps = lv_label_create(scr);
  lv_label_set_text(gps, (String("GPS ") + f.gps).c_str());
  lv_obj_align(gps, LV_ALIGN_CENTER, 0, 30);

  lv_obj_t* batt = lv_label_create(scr);
  lv_label_set_text(batt, (String(f.batteryPct) + "%" + (f.charging ? " +" : "")).c_str());
  lv_obj_align(batt, LV_ALIGN_TOP_RIGHT, -8, 8);

  lv_obj_t* conn = lv_label_create(scr);
  const char* c = f.conn == 2 ? "online" : f.conn == 1 ? "..." : "offline";
  lv_label_set_text(conn, c);
  lv_obj_align(conn, LV_ALIGN_TOP_LEFT, 8, 8);

  lv_scr_load(scr);
}

static void menu_event_cb(lv_event_t* e) {
  auto* fn = static_cast<std::function<void()>*>(lv_event_get_user_data(e));
  if (fn && *fn) (*fn)();
}

void showMenu(const MenuCallbacks& cb) {
  // User data must outlive the screen; keep static copies.
  static std::function<void()> onTasks, onWifi, onBack;
  onTasks = cb.onTasks; onWifi = cb.onWifi; onBack = cb.onBack;

  lv_obj_t* scr = lv_obj_create(nullptr);
  lv_obj_t* list = lv_list_create(scr);
  lv_obj_set_size(list, lv_pct(100), lv_pct(100));

  auto add = [&](const char* text, std::function<void()>* fn) {
    lv_obj_t* btn = lv_list_add_btn(list, nullptr, text);
    lv_obj_add_event_cb(btn, menu_event_cb, LV_EVENT_CLICKED, fn);
  };
  add("Task list", &onTasks);
  add("Wi-Fi setup", &onWifi);
  add("Back to watch", &onBack);

  lv_scr_load(scr);
}

static void task_toggle_cb(lv_event_t* e) {
  const char* id = static_cast<const char*>(lv_event_get_user_data(e));
  if (s_taskCb && s_taskCb->onToggle) s_taskCb->onToggle(String(id));
}

void showTasks(const NoteStore& store, const TaskCallbacks& cb) {
  static TaskCallbacks held;
  held = cb;
  s_taskCb = &held;

  lv_obj_t* scr = lv_obj_create(nullptr);
  s_taskList = lv_list_create(scr);
  lv_obj_set_size(s_taskList, lv_pct(100), lv_pct(100));
  lv_scr_load(scr);
  refreshTasks(store);
}

void refreshTasks(const NoteStore& store) {
  if (!s_taskList) return;
  lv_obj_clean(s_taskList);  // simple rebuild; recycle rows for many items
  if (store.notes().empty()) {
    lv_list_add_text(s_taskList, "No todos");
    return;
  }
  for (const auto& n : store.notes()) {
    String label = (n.done ? "[x] " : "[ ] ") + String(n.text.c_str());
    lv_obj_t* btn = lv_list_add_btn(s_taskList, nullptr, label.c_str());
    // user_data must persist; store ids in a side table in a real impl.
    lv_obj_add_event_cb(btn, task_toggle_cb, LV_EVENT_CLICKED,
                        const_cast<char*>(n.id.c_str()));
    // TODO: long-press -> onDelete; strike-through styling for done.
  }
}

void showProvision(const String& apSsid, const String& url) {
  lv_obj_t* scr = lv_obj_create(nullptr);
  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Wi-Fi setup");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t* info = lv_label_create(scr);
  lv_label_set_text(info, (String("Join AP:\n") + apSsid + "\nOpen:\n" + url).c_str());
  lv_obj_align(info, LV_ALIGN_CENTER, 0, 0);
  // TODO: render a QR code for the URL.

  s_provStatus = lv_label_create(scr);
  lv_label_set_text(s_provStatus, "waiting…");
  lv_obj_align(s_provStatus, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_scr_load(scr);
}

void updateProvision(bool connected, const String& ip) {
  if (!s_provStatus) return;
  lv_label_set_text(s_provStatus,
                    connected ? (String("connected ") + ip).c_str() : "waiting…");
}

}  // namespace wt::ui
