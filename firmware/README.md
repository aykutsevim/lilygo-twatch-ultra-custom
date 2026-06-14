# Firmware — WatchTodo (LilyGo T-Watch Ultra)

PlatformIO / Arduino / LVGL app for the ESP32-S3 T-Watch Ultra. Multi-mode,
low-power: a watch face (time/date/GPS/battery), a live todo list, and on-watch
Wi-Fi setup. See [`../specs/06-firmware.md`](../specs/06-firmware.md).

## Architecture

A `ModeManager` state machine (WATCH ⇄ MENU → TASKS / PROVISIONING) with an
inactivity timer, sitting over modular services. All board-specific code is behind
the `hw/Board` facade so the rest of the app is portable.

```
src/
  main.cpp                 wiring: setup()/loop(), mode transitions, services
  app/ModeManager.*        mode state machine + 30 s idle auto-return  (host-tested)
  model/Note.h NoteStore.* in-RAM notes + rev reconciliation            (host-tested)
  net/  WifiManager ApiClient NotesSocket Provisioning
  ui/   Screens.*          LVGL screens (watch/menu/tasks/provision/status)
  sensors/Gps.*            duty-cycled NMEA GPS (graceful if no source)
  time/Clock.*             RTC + NTP
  power/PowerManager.*     esp_pm DFS + light/deep sleep, backlight ladder
  storage/Settings.*       NVS (provisioned Wi-Fi/API config)
  hw/Board.*               ⟵ INTEGRATION POINT: wire to the LilyGo BSP
include/config.h           build-time defaults; secrets.h (git-ignored) optional
```

## Build & flash (real hardware)

> Requires the **official LilyGo T-Watch Ultra BSP** and its board id. Start from
> the official example's `platformio.ini` (board, partitions, PSRAM mode), then add
> this project's `lib_deps`, and implement `hw/Board_TWatchUltra.cpp` against the BSP
> (every function there is marked `// TODO(BSP)`).

```bash
cd firmware
pio run -e twatch-ultra            # compile
pio run -e twatch-ultra -t upload  # flash over USB
pio device monitor                 # 115200
```

Optional: copy `include/secrets.h.example` → `include/secrets.h` to pre-seed Wi-Fi
and the API target for a dev unit. Otherwise provision on-device (below).

## On-watch Wi-Fi setup (provisioning)

MENU → **Wi-Fi setup** starts a SoftAP `WatchTodo-Setup` + captive web page. Join it
from a phone, open `http://192.168.4.1`, pick a network, and (optionally) set the API
base/WS base/key. Values persist to NVS and take precedence over build-time defaults
— no reflash needed to change networks or the backend target.

## Tests / verification

The Arduino-free core logic is unit-tested on the host:

```bash
cd firmware
pio test -e native                 # Unity tests (needs a system g++/gcc)
```

Without a local compiler you can run the same checks via Docker:

```bash
docker run --rm -v "${PWD}:/src" -w /src gcc:13 sh -c \
  "g++ -std=c++17 -I src tools/host_check.cpp src/model/NoteStore.cpp src/app/ModeManager.cpp -o /tmp/hc && /tmp/hc"
```

This covers the `rev` reconciliation rules (apply / dedupe / gap→resync) and the
mode-transition + idle-return table. **Everything else — display, touch, sleep/wake,
GPS, the provisioning AP — requires the physical T-Watch Ultra and the BSP** and is
not verifiable in CI (see spec §6.13).
