# Implementation Plan

Build order for WatchTodo. The strategy: **make the contract real first, then build
the backend, then the web client, then the watch** — so each layer can be verified
against something already working. The API contract ([`specs/03-api-contract.md`](specs/03-api-contract.md))
is the spine; everything else is tested against it.

Legend: ⬜ not started · 🎯 milestone (independently verifiable).

---

## Phase 0 — Repo & tooling scaffold

Goal: `docker compose up` boots an (empty) stack; secrets never touch git.

- ⬜ Create folders: `backend/`, `frontend/`, `firmware/`.
- ⬜ Root `.gitignore`, `.env.example` (from [`specs/09-env-config.md`](specs/09-env-config.md)), `git init`.
- ⬜ `docker-compose.yml` with `redis` + placeholder `backend`/`frontend` (from [`specs/08-deployment.md`](specs/08-deployment.md)).
- ⬜ Pick the exact free-host combo (default: Render + Upstash + Netlify) and note it in README.

🎯 **M0**: `docker compose up redis` runs; `.env` copied from example; repo committed.

---

## Phase 1 — Backend: REST + Redis (no realtime yet)

Goal: full CRUD against Redis, verifiable with `curl`. Ref [`specs/04-backend.md`](specs/04-backend.md).

- ⬜ Django project `config/` (ASGI), minimal `INSTALLED_APPS` (no admin/auth/sessions), dummy DB backend.
- ⬜ Settings read all config from env; `django-cors-headers` wired to `CORS_ALLOWED_ORIGINS`.
- ⬜ `notes/store.py` — `NoteStore` over `redis-py` async: `list/create/update/delete/clear_done`, each returning `(rev, affected)`. Implement the key model (`notes:rev|seq|data|order`) with a pipeline per mutation.
- ⬜ `notes/serializers.py` — `text` 1–280 trimmed; read-only `id/created_at/updated_at`.
- ⬜ `notes/auth.py` — `X-API-Key` permission (constant-time), `/api/health` exempt.
- ⬜ `notes/views.py` + `urls.py` — endpoints per contract §3.3; correct status codes & error shape.
- ⬜ `GET /api/health` pings Redis (503 on failure).
- ⬜ `backend/Dockerfile` (python:3.12-slim → Daphne); wire into compose.
- ⬜ Tests: NoteStore (fakeredis), REST contract (auth 401, validation 400, ordering, `rev` increments).

🎯 **M1**: `curl` create/list/patch/delete/clear works end-to-end in compose; `rev`
increments; tests green. **The contract is now executable.**

---

## Phase 2 — Backend: realtime (WebSocket + broadcast)

Goal: mutations fan out to all sockets sub-second. Ref [`specs/07-realtime.md`](specs/07-realtime.md).

- ⬜ Add Channels + channels-redis; `CHANNEL_LAYERS` → `REDIS_URL`. `asgi.py` `ProtocolTypeRouter`.
- ⬜ `notes/consumers.py` — `NotesConsumer`: key check (`4401` close), join `notes` group, send `hello{rev}`, handle `toggle/delete/pong`, `ping` keepalive every `WS_PING_INTERVAL`.
- ⬜ `notes/events.py` — `broadcast()` via `group_send`; REST mutations call it after write.
- ⬜ Tests: `WebsocketCommunicator` — connect with/without key, receive `hello`, receive broadcast after a REST mutation.

🎯 **M2**: open two `websocat`/browser sockets, POST a note via REST → both receive
`note.created` with a bumped `rev` in < 1s. Realtime backbone proven without any UI.

---

## Phase 3 — Frontend: React web app

Goal: a usable web UI that drives the API and updates live. Ref [`specs/05-frontend.md`](specs/05-frontend.md).

- ⬜ Vite + TS scaffold; `types.ts` mirroring the contract; `api.ts` (adds `X-API-Key`).
- ⬜ `ws.ts` — WebSocket client: reconnect w/ backoff+jitter, `rev` tracking, `hello`→resync, gap→resync.
- ⬜ `store.ts` — notes state + reducer applying events idempotently by `rev`.
- ⬜ Components: `AddNote`, `NoteList`, `NoteItem` (toggle/delete), `StatusBar` (conn state).
- ⬜ `vite.config.ts` dev proxy for `/api` + `/ws` (avoid CORS in dev).
- ⬜ `frontend/Dockerfile` (multi-stage → nginx SPA fallback) + `nginx.conf`; wire into compose.

🎯 **M3**: type a note in browser tab A → appears in tab B instantly. Add/toggle/
delete/clear all work; reconnect after backend restart resyncs cleanly.

---

## Phase 4 — Firmware: T-Watch Ultra app

Goal: the watch shows the live list. Ref [`specs/06-firmware.md`](specs/06-firmware.md).

- ⬜ Start from the **official LilyGo T-Watch Ultra PlatformIO example**; confirm BSP display+touch+LVGL boot on real hardware *before* adding networking.
- ⬜ `include/config.h` + git-ignored `secrets.h` (Wi-Fi, `API_BASE/WS_BASE`, `API_KEY`).
- ⬜ `net/WifiManager` — connect + reconnect + status.
- ⬜ `net/ApiClient` — `GET /api/notes` (stream-parse array, bounded RAM), TLS via `WiFiClientSecure` (CA pin or documented `setInsecure`).
- ⬜ `model/NoteStore` — in-RAM list, `rev` tracking, apply events / full-resync.
- ⬜ `net/NotesSocket` — WS client (`links2004/WebSockets`): connect `?key=`, parse events, backoff reconnect, `hello`→resync, gap→resync.
- ⬜ `ui/ui_todo` (LVGL scrollable list, recycled rows) + `ui/ui_status` (wifi/conn icon).
- ⬜ Optional: tap=toggle, long-press=delete via WS `toggle`/`delete`.
- ⬜ Fallback: after repeated WS failures, poll `GET /api/notes` every `POLL_INTERVAL`.

🎯 **M4**: watch boots → connects Wi-Fi → loads list → a note typed in the browser
appears on the wrist in < 1s; watch survives Wi-Fi drop (reconnect + resync).

---

## Phase 5 — Deploy to free hosting

Goal: live URL, watch points at prod. Ref [`specs/08-deployment.md`](specs/08-deployment.md).

- ⬜ Provision managed Redis (Upstash); enable persistence; get `REDIS_URL`.
- ⬜ Deploy backend container (Render/Railway/Fly); set env group; **verify WS (`wss://`) passes through** the proxy.
- ⬜ Deploy frontend static build (Netlify/Vercel/CF Pages) with prod `VITE_*`.
- ⬜ Set CORS/`ALLOWED_HOSTS` to prod origins; `DEBUG=false`.
- ⬜ Rebuild firmware with prod `API_BASE/WS_BASE` (TLS) + flash.
- ⬜ Add a keepalive ping to `/api/health` to mitigate free-tier cold starts.

🎯 **M5**: end-to-end on the public URL; browser edit reflects on the watch over the
internet.

---

## Phase 6 — Hardening & polish (optional)

- ⬜ DRF throttle (runaway-client guard), structured stdout logging at `LOG_LEVEL`.
- ⬜ Backend pytest + frontend vitest in CI; `pio run` compile check.
- ⬜ Watch power management (backlight dim/sleep, resync on wake).
- ⬜ README quickstart + per-host deploy notes (`render.yaml` / `fly.toml`).

---

## Critical path & parallelism

```
M0 ─▶ M1 ─▶ M2 ─┬─▶ M3 (web) ──┐
                └─▶ M4 (watch) ─┴─▶ M5 ─▶ M6
```

- **M1 (executable contract) is the gate.** Web and watch (M3/M4) can be built in
  parallel once M2 lands, since both code only against the contract.
- Firmware (M4) needs physical hardware + the LilyGo BSP working; de-risk that
  early (boot the official example) even while backend work proceeds.
- The `rev`-resync model means each client can be developed and tested in isolation
  against the deployed/compose backend.

## Risks & mitigations

| Risk                                   | Mitigation                                             |
|----------------------------------------|--------------------------------------------------------|
| Host strips/blocks WebSocket           | Verify `wss://` early (M5 spike); polling fallback ready|
| T-Watch Ultra BSP/pin specifics differ | Build from official example first; don't hand-roll pins |
| Free-tier cold starts blow latency SLO | Keepalive ping; document first-request wake time        |
| ESP32 RAM with large note lists        | Bounded text (280), streamed JSON parse, recycled rows  |
| TLS on device (cert handling)          | Pin host root CA; `setInsecure` only as documented stopgap |
