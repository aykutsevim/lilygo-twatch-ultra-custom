#pragma once
// Build-time DEFAULTS (committed, placeholders). Real per-device values come
// from on-watch provisioning (NVS) and take precedence; an optional git-ignored
// secrets.h may pre-seed a dev unit. See specs/06-firmware.md §6.12.

#if __has_include("secrets.h")
#include "secrets.h"
#endif

// API target (overridable at runtime via provisioning -> NVS).
#ifndef DEFAULT_API_BASE
#define DEFAULT_API_BASE "https://watchtodo.example.com"   // REST base
#endif
#ifndef DEFAULT_WS_BASE
#define DEFAULT_WS_BASE "wss://watchtodo.example.com"       // WebSocket base
#endif
#ifndef DEFAULT_API_KEY
#define DEFAULT_API_KEY ""                                   // X-API-Key / ?key=
#endif
#define WS_PATH "/ws/notes/"
#define API_NOTES_PATH "/api/notes"

// Optional pre-seeded Wi-Fi (normally provisioned on-device instead).
#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""
#endif
#ifndef DEFAULT_WIFI_PASS
#define DEFAULT_WIFI_PASS ""
#endif

// Provisioning access point.
#define AP_SSID "WatchTodo-Setup"
#define AP_PASS ""            // empty = open AP (shown on screen)

// Timing (ms) — power vs. responsiveness. Tunable; also stored in NVS.
#define IDLE_RETURN_MS 30000  // auto-return to watch face
#define DIM_MS 8000           // dim backlight
#define SLEEP_MS 20000        // screen off / light sleep
#define DEEP_SLEEP_MS 300000  // deep sleep (Power-saver)
#define GPS_REFRESH_MS 300000 // GPS duty-cycle interval
#define WS_PING_TIMEOUT_MS 40000

// TLS: set to 1 to skip cert validation (hobby stopgap). Prefer pinning a CA.
#ifndef TLS_INSECURE
#define TLS_INSECURE 1
#endif

#define FW_VERSION "0.1.0"
