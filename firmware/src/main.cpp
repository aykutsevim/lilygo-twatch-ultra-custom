// WatchTodo firmware entrypoint — wires the modules behind the ModeManager
// state machine. See specs/06-firmware.md §6.11.
#include <Arduino.h>
#include <lvgl.h>

#if defined(ESP_PLATFORM)
#include "esp_sleep.h"
#endif

#include "../include/config.h"
#include "app/ModeManager.h"
#include "hw/Board.h"
#include "model/NoteStore.h"
#include "net/ApiClient.h"
#include "net/NotesSocket.h"
#include "net/Provisioning.h"
#include "net/WifiManager.h"
#include "power/PowerManager.h"
#include "sensors/Gps.h"
#include "storage/Settings.h"
#include "time/Clock.h"
#include "ui/Screens.h"

using namespace wt;

static Settings settings;
static ModeManager modes(IDLE_RETURN_MS);
static PowerManager power;
static WifiManager wifi;
static ApiClient api;
static NotesSocket socket;
static NoteStore store;
static Gps gps;
static Clock clk;
static Provisioning* prov = nullptr;

static Mode lastMode = Mode::Watch;
static uint32_t lastFaceMs = 0;

// ── helpers ───────────────────────────────────────────────────────────────
static void resync() {
  power.setNetworkBusy(true);
  if (api.fetchInto(store)) ui::refreshTasks(store);
  power.setNetworkBusy(false);
}

static void onServerEvent(const ServerEvent& e) {
  switch (store.applyEvent(e)) {
    case ApplyOutcome::Applied:
      ui::refreshTasks(store);
      break;
    case ApplyOutcome::NeedResync:
      resync();
      break;
    case ApplyOutcome::Ignored:
      break;
  }
}

static void enterTasks() {
  if (settings.hasWifi() && wifi.connected()) {
    resync();  // seed list
    socket.begin(settings.wsBase, settings.apiKey, onServerEvent);
  }
  ui::showTasks(store, {
    [](const String& id) { socket.sendToggle(id); },
    [](const String& id) { socket.sendDelete(id); },
    [] { modes.handle(Input::Back, millis()); },
  });
}

static void enterProvisioning() {
  if (!prov) prov = new Provisioning(settings);
  prov->start();
  ui::showProvision(AP_SSID, String("http://") + prov->apIp());
}

static void enterWatch() {
  socket.stop();           // free the WS when leaving tasks (power)
  if (prov) prov->stop();
}

static void enterMenu() {
  ui::showMenu({
    [] { modes.handle(Input::SelectTasks, millis()); },
    [] { modes.handle(Input::SelectWifi, millis()); },
    [] { modes.handle(Input::Back, millis()); },
  });
}

static void onModeChanged(Mode m) {
  switch (m) {
    case Mode::Watch:        enterWatch(); break;
    case Mode::Menu:         enterMenu(); break;
    case Mode::Tasks:        enterTasks(); break;
    case Mode::Provisioning: enterProvisioning(); break;
  }
}

static void drawWatchFace() {
  ui::WatchFace f;
  f.time = clk.timeHHMM();
  f.date = clk.dateLong();
  GpsFix fix = gps.lastFix();
  f.gps = fix.valid ? (String(fix.lat, 4) + ", " + String(fix.lon, 4)) : "--";
  f.batteryPct = board::batteryPercent();
  f.charging = board::isCharging();
  f.conn = wifi.connected() ? 2 : (settings.hasWifi() ? 1 : 0);
  ui::showWatch(f);
}

// ── Arduino lifecycle ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  board::begin();            // PMU, display, touch, RTC, IMU, LVGL hooks (BSP)
  settings.load();
  power.begin(settings);
  clk.begin();
  gps.begin();
  gps.setRefreshMs(GPS_REFRESH_MS);

  if (settings.hasWifi()) {
    wifi.connect(settings.wifiSsid, settings.wifiPass);
    api.configure(settings.apiBase, settings.apiKey);
  }

  drawWatchFace();           // start on the watch face
  lastMode = Mode::Watch;
}

void loop() {
  const uint32_t now = millis();

  lv_timer_handler();        // LVGL
  board::loop();

  // Mode machine + auto-return.
  Mode m = modes.tick(now);
  if (m != lastMode) {
    onModeChanged(m);
    lastMode = m;
  }

  // Per-mode services.
  switch (m) {
    case Mode::Watch:
      wifi.loop();
      gps.loop(now);
      if (now - lastFaceMs >= 1000) {  // 1 Hz face refresh
        drawWatchFace();
        lastFaceMs = now;
      }
      break;
    case Mode::Menu:
      wifi.loop();
      break;
    case Mode::Tasks:
      wifi.loop();
      socket.loop();
      break;
    case Mode::Provisioning:
      if (prov) {
        prov->loop();
        modes.setProvisioningClientActive(prov->clientConnected());
        if (prov->saved()) {  // creds entered -> switch to STA
          prov->stop();
          wifi.connect(settings.wifiSsid, settings.wifiPass);
          api.configure(settings.apiBase, settings.apiKey);
          modes.handle(Input::Back, now);  // back to watch
        }
      }
      break;
  }

  // Power ladder. Deep sleep does not return; light sleep yields between ticks.
  PowerTier tier = power.tick(now, settings.profile);
#if defined(ESP_PLATFORM)
  if (tier == PowerTier::DeepSleep && m == Mode::Watch) {
    board::prepareDeepSleep(0);
    esp_deep_sleep_start();  // wakes via touch/button/IMU/RTC -> setup()
  } else if (tier == PowerTier::ScreenOff && m == Mode::Watch) {
    board::prepareLightSleep();
    esp_light_sleep_start();  // brief; wakes on configured sources
  }
#else
  (void)tier;
#endif
}
