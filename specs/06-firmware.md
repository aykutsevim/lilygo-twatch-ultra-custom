# 06 — Firmware (PlatformIO · T-Watch Ultra)

The watch is a **multi-mode, low-power** device. Its home screen is a watch face
(time, date, GPS, battery); from there the user opens a menu to view the **task
list** or to **set up Wi-Fi** via an on-watch access point + web page. It returns
to the watch face on a back action or after 30 s of inactivity, and it uses the
ESP32-S3 power-management features aggressively to maximize battery life.

## 6.1 Platform

- **Build system**: PlatformIO (VS Code or CLI).
- **Framework**: Arduino (ESP32-S3 / ESP-IDF-under-Arduino).
- **UI**: **LVGL** (the T-Watch Ultra examples use LVGL; reuse that).
- **Board**: LilyGo T-Watch Ultra (ESP32-S3). Use the **official LilyGo T-Watch
  Ultra board support package (BSP) / library** for display, touch, PMU, RTC, IMU,
  and pin definitions. Do **not** hand-roll drivers or pin maps — depend on the BSP.

> Pin maps, display driver, touch controller, PMU/RTC/IMU init, and wake sources
> differ by board revision and are provided by LilyGo's board package. The firmware
> here is an **application** on top of that BSP: it owns the mode state machine,
> networking, GPS handling, power policy, and the todo UI.

### Hardware notes to verify against your unit / BSP
- **GPS**: the base T-Watch Ultra may **not** have an onboard GNSS receiver. Treat
  GPS as an optional source behind an abstraction (§6.7): read NMEA from an onboard
  module if present, an expansion-port module, or a phone over BLE. If no fix source
  exists, the watch face shows `GPS: --` and the feature degrades gracefully.
- **IMU / RTC / PMU**: used for tilt-to-wake, timekeeping, and battery/charge state.
  Confirm the parts and their interrupt lines from the BSP.

## 6.2 App model — modes & state machine

The app is a small state machine with four user-facing modes plus an orthogonal
**power state** (§6.9). A `ModeManager` owns the current mode and the inactivity
timer.

```
        ┌──────────────────────────────────────────────────────────┐
        │                        WATCH (home)                        │
        │  time · date · GPS · battery · wifi/conn status            │
        └───┬───────────────────────────────────────────────────────┘
            │ tap / button = open menu
            ▼
        ┌───────────────── MENU (Settings) ─────────────────┐
        │  • Task list                                       │
        │  • Wi-Fi setup  (start on-watch AP + web UI)       │
        │  • Brightness / Power profile (optional)           │
        │  • About / status                                  │
        │  • Back to watch                                   │
        └───┬───────────────────────────┬───────────────────┘
            │ "Task list"               │ "Wi-Fi setup"
            ▼                           ▼
   ┌──────────────────┐        ┌─────────────────────────────┐
   │   TASKS (live)   │        │  PROVISIONING (SoftAP + web) │
   │  todo list, WS   │        │  AP "WatchTodo-Setup" + form │
   └──────────────────┘        └─────────────────────────────┘

Return to WATCH from any mode: back gesture/button, OR auto after IDLE_RETURN_MS
(default 30 000 ms) with no touch/button input. Provisioning mode does NOT
auto-return while the AP is serving a client (so the user can finish the form).
```

- **Input**: capacitive touch (LVGL) + the side button(s) per BSP. Back =
  swipe/back button; menu = tap on watch face or button.
- **Inactivity timer**: any input resets it. On expiry the app switches to WATCH
  mode and the power policy begins dimming/sleeping (§6.9).

## 6.3 `platformio.ini` (indicative)

```ini
[env:twatch-ultra]
platform = espressif32
board = lilygo-t-watch-ultra      ; or the board id shipped by the LilyGo BSP
framework = arduino
monitor_speed = 115200
build_flags =
    -DCORE_DEBUG_LEVEL=3
    -DBOARD_HAS_PSRAM
    -DLV_CONF_INCLUDE_SIMPLE
lib_deps =
    lvgl/lvgl @ ^8.4              ; match the version the BSP examples use
    bblanchon/ArduinoJson @ ^7
    links2004/WebSockets @ ^2.4   ; ESP32 WS client (tasks mode)
    mikalhart/TinyGPSPlus @ ^1.0  ; NMEA parsing (if a GPS source is present)
    ; ESP32 Arduino core provides WiFi, WebServer, DNSServer, Preferences (NVS),
    ; esp_sleep / esp_pm — no extra lib_deps needed for those.
    ; + the official LilyGo T-Watch Ultra board support library
    ;   (added via lib_deps git URL or vendored in firmware/lib/)
board_build.partitions = default_16MB.csv   ; match the board's flash size
board_build.arduino.memory_type = qio_opi   ; PSRAM mode per BSP
```

> Pin/clock/partition values must match the LilyGo example `platformio.ini`. Start
> from the official example and add the app's `lib_deps`.

## 6.4 Project layout

```
firmware/
├── platformio.ini
├── partitions.csv                 # if customized
├── include/
│   ├── config.h                   # build-time defaults (overridable, see 6.10)
│   └── secrets.h                  # git-ignored; optional pre-seeded creds
├── lib/
│   └── TWatchUltraBSP/            # vendored official BSP, if not via lib_deps
└── src/
    ├── main.cpp                   # setup(): BSP init -> services -> ModeManager
    ├── app/
    │   ├── ModeManager.{h,cpp}    # mode state machine + inactivity timer
    │   └── Events.{h,cpp}         # input/router events
    ├── ui/
    │   ├── ui_watch.{h,cpp}       # WATCH mode (watch face)
    │   ├── ui_menu.{h,cpp}        # MENU mode
    │   ├── ui_tasks.{h,cpp}       # TASKS mode (todo list)
    │   ├── ui_provision.{h,cpp}   # PROVISIONING mode status screen
    │   └── ui_status.{h,cpp}      # shared status bar (wifi/battery icons)
    ├── net/
    │   ├── WifiManager.{h,cpp}    # STA connect/reconnect, status
    │   ├── Provisioning.{h,cpp}   # SoftAP + DNS captive portal + WebServer
    │   ├── ApiClient.{h,cpp}      # REST: GET /api/notes (+ optional actions)
    │   └── NotesSocket.{h,cpp}    # WebSocket client, reconnect, event parse
    ├── model/
    │   └── NoteStore.{h,cpp}      # in-RAM list<Note>, apply events, rev tracking
    ├── sensors/
    │   └── Gps.{h,cpp}            # GpsProvider: NMEA read, duty-cycle, last fix
    ├── time/
    │   └── Clock.{h,cpp}          # RTC + NTP sync, local time formatting
    ├── power/
    │   └── PowerManager.{h,cpp}   # backlight, CPU/Wi-Fi power, sleep, wake sources
    └── storage/
        └── Settings.{h,cpp}       # NVS (Preferences): wifi + API config persistence
```

## 6.5 WATCH mode (home / watch face)

Renders, refreshed about once per second while awake (driven by an RTC tick, not a
busy loop):

- **Time** — large, from `Clock` (RTC; NTP-corrected when online).
- **Date** — weekday, day, month.
- **GPS** — `lat, lon` and fix age from `GpsProvider`; `--` if no fix/no hardware.
- **Battery** — percent + charging indicator from the PMU (BSP).
- **Connection** — small icon: Wi-Fi associated / syncing / offline; provisioning AP
  active indicator when relevant.

Watch mode is the lowest-interaction screen and the entry point to the power-saving
ladder (§6.9). It does **not** keep the task WebSocket open (see §6.8 connectivity
policy); it only needs time + sensors, which run on RTC/timer wakes.

## 6.6 MENU / Settings mode

A short LVGL list:

| Item              | Action                                                          |
|-------------------|-----------------------------------------------------------------|
| Task list         | Switch to TASKS mode (connects + resyncs, opens WebSocket)      |
| Wi-Fi setup       | Switch to PROVISIONING mode (start SoftAP + web UI)             |
| Brightness        | Adjust backlight (persisted to NVS) — optional                  |
| Power profile     | `Balanced` / `Power-saver` (affects sleep policy) — optional    |
| About / status    | IP, RSSI, battery, firmware version, current API base           |
| Back to watch     | Return to WATCH mode                                            |

Back gesture/button returns to WATCH. The 30 s inactivity timer applies.

## 6.7 GPS (`GpsProvider`)

- Abstraction over a NMEA stream (`TinyGPSPlus`) read from the GPS source's UART
  (pins per BSP/expansion) **or** a null implementation when no source exists.
- **Duty-cycled for power**: GPS receivers are power-hungry. Do **not** keep it on
  continuously. Policy: power the module on a schedule (e.g. on entering WATCH mode
  and then every `GPS_REFRESH_MS`, default 5 min), acquire a fix within a bounded
  window, cache `{lat, lon, fix_time}`, then power it down. The watch face shows the
  **last cached fix** with its age.
- If `Power-saver` profile or low battery: lengthen the interval or disable GPS.

## 6.8 TASKS mode (live todo list)

This is the only mode that needs the realtime path; it follows the API contract in
[`03-api-contract.md`](03-api-contract.md) and the resync model in
[`07-realtime.md`](07-realtime.md).

**On entering TASKS mode**
1. Ensure Wi-Fi STA is connected (raise from modem-sleep / reconnect if needed).
2. `ApiClient.getNotes()` → seed `NoteStore`, record `rev`. Render the LVGL list.
3. `NotesSocket.connect()` to `${WS_BASE}/ws/notes/?key=…`.

**While in TASKS mode**
- `lv_timer_handler()` drives LVGL; `NotesSocket.loop()` pumps the WS client.
- WS events (`hello` / `note.created|updated|deleted` / `notes.cleared`): apply by
  `rev` — if `event.rev > local.rev` apply and advance; if `event.rev >
  local.rev + 1` do a full `getNotes()` resync; ignore `event.rev ≤ local.rev`.
- `ping` → optional `pong`; resets the WS watchdog.

**Light actions (optional)**: tap a row = toggle `done` (WS `{type:"toggle",id}`),
long-press = delete (WS `{type:"delete",id}`). UI updates optimistically, then
reconciles on the resulting broadcast/`rev`. The watch does **not** create notes.

**On WS drop / leaving TASKS mode**: backoff-reconnect while in mode (1s→max 30s,
jitter); when leaving TASKS mode, close the socket to save power. Re-entry resyncs.

### Connectivity & power policy (the key trade-off)
"Immediate" updates matter only while the user is **looking at the task list**.
Therefore:
- **TASKS mode** keeps the WebSocket open for sub-second push.
- **WATCH mode** does **not** hold the WebSocket. Depending on profile:
  - `Balanced` (default): Wi-Fi stays *associated* with **modem sleep** (DTIM) so
    re-entering TASKS mode is fast; the WS is (re)opened on entry and a resync makes
    the list current. No events are missed in a way that matters (Redis is the
    source of truth; `rev` resync self-heals).
  - `Power-saver`: Wi-Fi radio is dropped in WATCH mode and during sleep; it is
    brought up only when entering TASKS mode or for a periodic background sync.
- This keeps idle (watch-face) power low while preserving real-time where it counts.

## 6.9 Power management (maximize uptime)

Goal: long battery life. Implemented in `PowerManager` using ESP32-S3 facilities
(`esp_pm` DFS/light-sleep, `esp_sleep` deep-sleep + wake sources, Wi-Fi modem sleep,
PMU/backlight control from the BSP). An **inactivity ladder** runs on top of the
mode machine; any input resets it to ACTIVE.

| Tier | Trigger | Display | CPU / radio | Wake sources |
|------|---------|---------|-------------|--------------|
| **ACTIVE** | user interacting | full brightness | 240 MHz; Wi-Fi per mode | n/a |
| **DIMMED** | `DIM_MS` idle (e.g. 8 s) | backlight low; force WATCH mode | DFS down to ~80 MHz | touch/button |
| **SCREEN-OFF / LIGHT SLEEP** | `SLEEP_MS` idle (e.g. 20 s) | off | automatic light sleep between RTC ticks; Wi-Fi modem sleep | touch IRQ, button, IMU tilt, RTC tick |
| **DEEP SLEEP** | `DEEP_SLEEP_MS` idle (e.g. 5 min, `Power-saver`) | off | radio off; RAM mostly powered down | touch IRQ, button, IMU tilt, RTC alarm |

Principles:
- **Enable `esp_pm` automatic light sleep** with DFS so the CPU idles between the
  1 Hz watch-face tick and sensor reads instead of spinning.
- **Wi-Fi modem sleep** (`WIFI_PS_MIN/MAX_MODEM`) whenever associated and not in an
  active transfer; drop the radio entirely in `Power-saver` WATCH/sleep.
- **Backlight is the biggest draw** — dim fast, turn off on screen-off. Use the PMU
  to gate the panel where supported.
- **Wake**: configure touch controller IRQ, side button, and IMU tilt as wake
  sources; an RTC alarm wakes periodically (timekeeping / optional background sync).
- **On deep-sleep wake**: re-init the minimum needed, restore time from RTC, and —
  if entering TASKS — reconnect Wi-Fi and `getNotes()` resync. Real-time is traded
  for battery while asleep; the `rev` model makes the catch-up correct.
- **GPS duty-cycling** (§6.7) and avoiding continuous WS in WATCH mode are the other
  large savings.

All thresholds (`DIM_MS`, `SLEEP_MS`, `DEEP_SLEEP_MS`, `IDLE_RETURN_MS`,
`GPS_REFRESH_MS`) are tunable constants (config / NVS) so power vs. responsiveness
can be balanced without code changes.

## 6.10 Wi-Fi provisioning (on-watch AP + web interface)

Entered from MENU → "Wi-Fi setup". Lets the user configure Wi-Fi (and optionally the
API target) **without** rebuilding firmware.

Flow:
1. Stop STA; start **SoftAP** with SSID `WatchTodo-Setup` (open or a fixed WPA2 pass
   shown on screen). Start a **DNS** server answering all names → the watch IP
   (captive-portal style) and an HTTP `WebServer`.
2. The watch screen (`ui_provision`) shows: AP SSID, password (if any), and the URL
   (e.g. `http://192.168.4.1`). A QR code is a nice-to-have.
3. User connects a phone/laptop to the AP and opens the page:
   - `GET /` → HTML form. Lists nearby networks (`WiFi.scanNetworks()`), fields for
     **SSID**, **password**, and (optional) **API base**, **WS base**, **API key**.
   - `POST /save` → validate, persist to **NVS** (`Settings`), respond "saved,
     connecting…".
   - `GET /status` → JSON `{ connected, ip, rssi }` for the page to poll.
4. Watch stops the AP, switches to STA, connects with the new creds, and returns to
   WATCH mode. On success the connection icon goes green; on failure it returns to
   provisioning with an error.

Notes:
- Provisioning mode is comparatively power-hungry (AP + radio active) but
  short-lived and user-initiated; the inactivity auto-return is suspended while a
  client is connected so the form can be completed.
- The provisioning web UI is **local-only** (served by the watch) and unrelated to
  the backend API; it does not change the API contract.

## 6.11 Boot & main loop

**`setup()`**
1. BSP init: PMU, display, touch, backlight, RTC, IMU (per LilyGo example).
2. Init LVGL (display flush + touch callbacks from BSP); buffers in PSRAM.
3. `Settings.load()` from NVS. `PowerManager.begin()` (configure `esp_pm`, wake
   sources). `Clock.begin()` (read RTC).
4. If Wi-Fi creds exist → `WifiManager.connect()` (async); else hint the user toward
   MENU → Wi-Fi setup. Start `GpsProvider` duty cycle.
5. `ModeManager` starts in **WATCH** mode.

**`loop()`**
- `lv_timer_handler()` (UI) and `ModeManager.tick()` (mode + inactivity timer).
- Pump only what the current mode needs: `NotesSocket.loop()` in TASKS;
  `Provisioning.loop()` (DNS+HTTP) in PROVISIONING; sensor/clock reads otherwise.
- `PowerManager.tick()` advances the inactivity ladder and enters light/deep sleep;
  keep heavy work off the LVGL render path.

## 6.12 Configuration & storage

Two layers:
- **NVS (`Preferences`) at runtime** — the primary store, written by provisioning:
  `wifi_ssid`, `wifi_pass`, `api_base`, `ws_base`, `api_key`, plus UI prefs
  (brightness, power profile). Survives reboot; no reflash to change Wi-Fi/API.
- **Build-time defaults** — `include/config.h` (committed, placeholders) and a
  git-ignored `include/secrets.h` may pre-seed values for development so a freshly
  flashed unit can connect without provisioning. NVS values, once set, take
  precedence over build-time defaults.

```c
// include/config.h (defaults; real values via secrets.h or provisioning)
#define DEFAULT_API_BASE   "https://watchtodo.example.com"   // REST
#define DEFAULT_WS_BASE     "wss://watchtodo.example.com"     // WebSocket
#define WS_PATH             "/ws/notes/"
#define AP_SSID             "WatchTodo-Setup"
#define IDLE_RETURN_MS      30000
#define DIM_MS              8000
#define SLEEP_MS            20000
#define DEEP_SLEEP_MS       300000
#define GPS_REFRESH_MS      300000
```

- TLS: for `https`/`wss`, use `WiFiClientSecure`. Prefer pinning the host root CA
  (`setCACert`); `setInsecure()` is a documented hobby stopgap. Same `api_key` as
  the backend `API_KEY` / web `VITE_API_KEY` (the contract's `X-API-Key` / `?key=`).

## 6.13 What can be verified without hardware
- `pio run` **compile check** in CI (no device needed).
- Logic units that don't touch hardware (e.g. `NoteStore` apply/`rev` rules, NMEA
  parsing, mode-transition table) can be unit-tested natively where practical.
- Full behavior (display, touch, sleep/wake, GPS, provisioning AP) requires the
  physical T-Watch Ultra and the LilyGo BSP.
