# WatchTodo

A single-user, no-auth todo list that lives on a **LilyGo T-Watch Ultra** (ESP32-S3)
and is edited from a **React web app**. Notes are stored in **Redis** (no database),
served by a **Django** API, and pushed to the watch **in real time** over WebSocket —
type a note in the browser and it appears on your wrist within a second.

```
┌──────────────┐      REST + WS        ┌──────────────────────┐      WS push      ┌───────────────────┐
│  React web   │  ───────────────────▶ │  Django API + Redis  │ ────────────────▶ │  T-Watch Ultra    │
│  (browser)   │  ◀─────────────────── │  (Channels / DRF)    │ ◀──────────────── │  (ESP32-S3 / LVGL)│
└──────────────┘    live updates       └──────────────────────┘   REST + WS       └───────────────────┘
```

## Components

| Folder      | Tech                                   | Purpose                                        |
|-------------|----------------------------------------|------------------------------------------------|
| `backend/`  | Django, DRF, Channels, Redis           | REST API + WebSocket push, Redis data store    |
| `frontend/` | React (Vite), TypeScript               | Web UI to view/add/complete/delete notes       |
| `firmware/` | PlatformIO, Arduino, LVGL              | T-Watch Ultra app that displays the todo list  |
| `specs/`    | Markdown                               | Full specifications (read these first)         |

## Specs

Read in order:

1. [`specs/01-overview.md`](specs/01-overview.md) — goals, scope, constraints
2. [`specs/02-architecture.md`](specs/02-architecture.md) — system design, data flow
3. [`specs/03-api-contract.md`](specs/03-api-contract.md) — REST + WebSocket contract
4. [`specs/04-backend.md`](specs/04-backend.md) — Django service, Redis data model
5. [`specs/05-frontend.md`](specs/05-frontend.md) — React app
6. [`specs/06-firmware.md`](specs/06-firmware.md) — PlatformIO firmware for the watch
7. [`specs/07-realtime.md`](specs/07-realtime.md) — live-update mechanism
8. [`specs/08-deployment.md`](specs/08-deployment.md) — Docker + free hosting
9. [`specs/09-env-config.md`](specs/09-env-config.md) — environment variables

Then build it: [`IMPLEMENTATION_PLAN.md`](IMPLEMENTATION_PLAN.md) — phased, milestone-gated build order.

## Constraints (north star)

- **One user, no auth.** A static shared API key gates writes; that's it.
- **No relational DB.** Redis is the only persistence layer.
- **Free hosting.** Backend + Redis must run on free tiers (Render / Railway / Fly + Upstash).
- **Real-time.** Web edits reach the watch immediately (target < 1s).
- **Dockerized.** `docker compose up` runs the whole stack locally.
