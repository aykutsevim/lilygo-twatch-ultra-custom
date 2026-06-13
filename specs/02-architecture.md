# 02 — Architecture

## 2.1 High-level diagram

```
                          ┌─────────────────────────────────────────────┐
                          │                Backend (one host)             │
                          │                                               │
  React web  ──HTTP/REST─▶│  Django ASGI app (Daphne/Uvicorn)             │
  (browser)  ◀──WebSocket─│   ├── DRF views      → REST CRUD              │
                          │   ├── Channels consumer → WebSocket /ws/notes │
                          │   └── NoteStore       → Redis client          │
                          │                                               │
  T-Watch    ──HTTP/REST─▶│  Channels layer  ◀──▶  Redis                  │
  Ultra      ◀──WebSocket─│  (pub/sub fan-out)     ├── note data (source) │
                          │                        └── channel layer broker│
                          └─────────────────────────────────────────────┘
```

Redis plays **two roles**:
1. **System of record** for notes (the only persistence).
2. **Channel layer broker** for Channels (pub/sub used to fan messages out to all
   connected WebSocket clients across workers).

A single Redis instance covers both (different key namespaces). On free hosting a
managed Redis (e.g. Upstash) is used; Channels' Redis channel layer and the data
store share it.

## 2.2 Components

### Django backend (`backend/`)
- **DRF** exposes REST endpoints for CRUD (used by web for all ops; used by watch
  for initial load and light actions).
- **Channels** consumer exposes `ws://…/ws/notes/` for live updates.
- **NoteStore** is a thin repository wrapping `redis-py` (async) with the data
  model in [`04-backend.md`](04-backend.md).
- Any mutation (REST or WS) goes through NoteStore, then broadcasts an event to the
  Channels group `notes` so every connected client updates. See
  [`07-realtime.md`](07-realtime.md).

### React frontend (`frontend/`)
- Loads notes via `GET /api/notes`, then opens the WebSocket for live updates.
- Mutations are sent as REST calls; the resulting broadcast updates all clients
  (including the originating one — it reconciles by `id`).

### Firmware (`firmware/`)
- On boot: connect Wi-Fi → `GET /api/notes` for the full list → open WebSocket.
- On WS event: apply incremental change to the in-memory list and re-render (LVGL).
- On WS drop: exponential backoff reconnect, then full REST resync to avoid drift.

## 2.3 Data flow — "add a note from the web"

```
Browser                Django                 Redis                 Watch
  │  POST /api/notes      │                      │                     │
  │─────────────────────▶ │  NoteStore.create    │                     │
  │                       │────── HSET/ZADD ────▶ │                     │
  │                       │  broadcast(note.created) via channel layer  │
  │                       │────── pub/sub ──────▶ │── deliver to group ─▶│ (WS)
  │ ◀─── 201 + note ──────│                      │                     │
  │  (also gets WS echo;  │                      │     WS: note.created │
  │   dedupe by id)       │                      │                     │─▶ render
```

## 2.4 Why this shape

- **Redis-only + Channels Redis layer**: satisfies "no DB" and gives real-time
  fan-out for free with one dependency.
- **REST for writes, WS for reads/push**: writes stay simple/idempotent and
  cache-friendly; the WS is a one-way (server→client) event stream plus optional
  client→server light actions from the watch. Keeps the watch client small.
- **ASGI single process**: fits a free dyno/container. Horizontal scaling is
  unnecessary for one user but the Redis channel layer makes it possible anyway.

## 2.5 Failure & resync model

- **Source of truth is Redis.** WebSocket events are an optimization, never the
  authority. Any client that reconnects performs a full `GET /api/notes` to
  reconcile, so a missed event self-heals.
- Events carry a monotonically increasing `rev` (see [`03-api-contract.md`](03-api-contract.md)).
  Clients ignore events with `rev` ≤ their last applied `rev` and trigger a full
  resync if they detect a gap.
