# 01 — Overview

## 1.1 Goal

Build a personal todo list that is authored on a web app and displayed live on a
LilyGo T-Watch Ultra. The system is intentionally minimal: one user, no accounts,
no database. Notes live in Redis and are pushed to every connected client
(browser and watch) the moment they change.

## 1.2 Primary use case

1. User opens the React web app on a laptop/phone browser.
2. User types a new note and presses Enter.
3. The note is persisted to Redis via the Django API.
4. The watch — already connected over WebSocket — receives the new note and
   renders it on screen within ~1 second, no manual refresh.
5. User can mark notes done or delete them from either the web (full control) or
   the watch (toggle done / delete, see firmware spec).

## 1.3 In scope

- CRUD for notes: create, list, toggle done, delete, clear-completed.
- Real-time fan-out of changes to all connected clients.
- A watch UI that shows the list, scrolls, and reflects live changes.
- Local dev via Docker Compose; deploy to a free hosting tier.
- A single static API key shared by web + firmware (kept out of source via env).

## 1.4 Out of scope (explicitly)

- User accounts, login, OAuth, multi-tenant data. **No auth beyond the shared key.**
- Relational/SQL database, migrations of business data. **Redis only.**
- Offline editing/sync conflict resolution on the watch (watch is mostly a viewer
  with light actions; see [`06-firmware.md`](06-firmware.md)).
- Push notifications, alarms, calendar, or any non-todo watch feature.
- Native mobile apps.

## 1.5 Target hardware

**LilyGo T-Watch Ultra**
- SoC: ESP32-S3 (dual-core Xtensa LX7, Wi-Fi 802.11 b/g/n + BLE 5)
- PSRAM + large SPI flash (OPI PSRAM / multi-MB flash — use board's PSRAM for LVGL buffers)
- Display: ~2.06" AMOLED, capacitive touch
- Power management IC, RTC, vibration motor, battery

> Exact display driver, touch controller, and pin map come from the **official
> LilyGo T-Watch Ultra board support package (BSP) / examples**. The firmware spec
> builds on that BSP rather than re-deriving pins. Do not hard-code pins in app
> code — use the BSP's `pins_config.h` / board init.

## 1.6 Quality targets

| Aspect            | Target                                                        |
|-------------------|---------------------------------------------------------------|
| Web→watch latency | < 1 s under normal Wi-Fi                                       |
| Watch reconnect   | Auto-reconnect WebSocket within 5 s of drop; REST resync       |
| Data durability   | Redis persistence (AOF/RDB) enabled where the host allows it   |
| Note count        | Designed for up to ~200 notes (personal scale)                |
| Cost              | $0 — free tiers only                                          |

## 1.7 Repository layout

```
.
├── backend/        # Django project (API + WebSocket + Redis access)
├── frontend/       # React (Vite + TS) web app
├── firmware/       # PlatformIO project for T-Watch Ultra
├── docker-compose.yml
├── .env.example
└── specs/          # this folder
```
