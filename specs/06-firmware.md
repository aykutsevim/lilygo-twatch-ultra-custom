# 06 — Firmware (PlatformIO · T-Watch Ultra)

## 6.1 Platform

- **Build system**: PlatformIO (VS Code or CLI).
- **Framework**: Arduino (ESP32-S3 / ESP-IDF-under-Arduino).
- **UI**: **LVGL** (the T-Watch Ultra examples use LVGL; reuse that).
- **Board**: LilyGo T-Watch Ultra (ESP32-S3). Use the **official LilyGo T-Watch
  Ultra BSP / library** for display, touch, PMU, and pin definitions. Do **not**
  hand-roll display/touch drivers or pin maps — depend on the BSP.

> Pin maps, display driver, touch controller, and power-management init differ by
> board revision and are provided by LilyGo's board package. The firmware here is
> an **application** on top of that BSP: it owns Wi-Fi, networking, and the todo UI.

## 6.2 `platformio.ini` (indicative)

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
    lvgl/lvgl @ ^8.4          ; match the version the BSP examples use
    bblanchon/ArduinoJson @ ^7
    links2004/WebSockets @ ^2.4   ; ESP32 WS client (or gilmaimon/ArduinoWebsockets)
    ; + the official LilyGo T-Watch Ultra board support library
    ;   (added via lib_deps git URL or as a vendored lib in firmware/lib/)
board_build.partitions = default_16MB.csv   ; match the board's flash size
board_build.arduino.memory_type = qio_opi   ; PSRAM mode per BSP
```

> Pin/clock/partition values must match the LilyGo example `platformio.ini`. Start
> from the official example and add the app's `lib_deps`.

## 6.3 Project layout

```
firmware/
├── platformio.ini
├── partitions.csv              # if customized
├── include/
│   └── config.h                # build-time defaults (overridable, see 6.7)
├── lib/
│   └── TWatchUltraBSP/          # vendored official BSP, if not pulled via lib_deps
└── src/
    ├── main.cpp                # setup()/loop(): BSP init → wifi → net → ui
    ├── net/
    │   ├── WifiManager.{h,cpp} # connect, reconnect, status
    │   ├── ApiClient.{h,cpp}   # GET /api/notes, optional POST/PATCH/DELETE
    │   └── NotesSocket.{h,cpp} # WebSocket client, reconnect, event parse
    ├── model/
    │   └── NoteStore.{h,cpp}   # in-RAM list<Note>, apply events, rev tracking
    └── ui/
        ├── ui_todo.{h,cpp}     # LVGL list screen
        └── ui_status.{h,cpp}   # wifi/connection indicator
```

## 6.4 Runtime behavior

**Boot sequence (`setup`)**
1. BSP init: power management, display, touch, backlight (per LilyGo example).
2. Init LVGL + display flush + touch read callbacks (from BSP).
3. Show "Connecting Wi-Fi…" splash.
4. `WifiManager.connect()` using `WIFI_SSID` / `WIFI_PASS`.
5. `ApiClient.getNotes()` → seed `NoteStore`, record `rev`. Render list.
6. `NotesSocket.connect()` to `${WS_BASE}/ws/notes/?key=…`.

**Main loop (`loop`)**
- `lv_timer_handler()` to drive LVGL.
- `NotesSocket.loop()` to pump the WS client (the WS lib is polled).
- Periodic Wi-Fi health check; if disconnected, reconnect then resync.
- Keep work off the LVGL render path; apply note changes then call `ui_todo_refresh()`.

**On WS events** (see [`03-api-contract.md`](03-api-contract.md) §3.4)
- `hello{rev}`: if `rev > local.rev` → `ApiClient.getNotes()` resync.
- `note.created` / `note.updated` / `note.deleted` / `notes.cleared`: if
  `event.rev > local.rev` apply to `NoteStore`, set `local.rev = event.rev`,
  refresh UI. If `event.rev > local.rev + 1` → full resync.
- `ping`: optionally reply `pong`; reset a watchdog timer.

**On WS drop**: exponential backoff reconnect (1s→max 30s, jitter). On reconnect,
the `hello` frame triggers resync if behind. This guarantees the watch never drifts
even if it misses events while asleep/disconnected.

## 6.5 Watch-side actions (optional, light)

The watch is primarily a **viewer**. Optional touch actions:
- Tap a note → toggle `done`. Send WS `{ "type":"toggle", "id":… }` (or `PATCH`).
- Long-press → delete. Send WS `{ "type":"delete", "id":… }` (or `DELETE`).

The watch does **not** create notes (no good text entry); creation is web-only.
Local UI updates optimistically, then reconciles on the resulting broadcast/`rev`.

## 6.6 Memory & rendering notes

- Use PSRAM for LVGL draw buffers (board has PSRAM; set in BSP / `lv_conf`).
- Cap displayed notes (e.g. render a scrollable LVGL list; ~200 items is fine with
  recycled objects). Store note text bounded to 280 chars per the contract.
- Parse JSON incrementally with ArduinoJson `JsonDocument` sized for one event;
  for the full snapshot, stream-parse the array to bound RAM.
- TLS: if the API is `https`/`wss`, use `WiFiClientSecure`. Provide the CA via
  `setCACert` (bundle the host's root CA) or, for a hobby setup, `setInsecure()`
  with the trade-off documented. Prefer pinning the root CA.

## 6.7 Firmware configuration

Embedded apps can't read a `.env` at runtime as easily; use **build-time config**
with an untracked override file (mirrors env-config philosophy):

- `include/config.h` holds **defaults** and is committed with placeholders.
- `include/secrets.h` (git-ignored) holds real `WIFI_SSID`, `WIFI_PASS`,
  `API_KEY`, `API_BASE`, `WS_BASE`. `config.h` `#include`s it if present.
- Alternatively inject via `platformio.ini` `build_flags = -DWIFI_SSID='"…"'`
  sourced from the environment in CI.
- Optional nicety: a first-boot Wi-Fi captive portal (WiFiManager lib) so SSID/pass
  aren't compiled in. API base/key can still be build-time.

Keys defined:
```c
#define WIFI_SSID   "…"
#define WIFI_PASS   "…"
#define API_BASE    "https://watchtodo.example.com"   // for REST
#define WS_BASE     "wss://watchtodo.example.com"      // for WebSocket
#define API_KEY     "dev-shared-key"
#define WS_PATH     "/ws/notes/"
```

## 6.8 Power (nice-to-have, not required for v1)

- Dim/sleep backlight after inactivity (BSP PMU). On wake, pump WS and resync via
  `hello`/`rev`. Keeping the socket alive during light sleep is optional; the
  resync-on-reconnect model means deep sleep + reconnect is acceptable.
