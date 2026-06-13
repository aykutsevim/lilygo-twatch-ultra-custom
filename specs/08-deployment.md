# 08 — Deployment (Docker + free hosting)

## 8.1 Local: Docker Compose

`docker-compose.yml` runs the whole stack for development:

```yaml
services:
  redis:
    image: redis:7-alpine
    command: ["redis-server", "--appendonly", "yes"]   # AOF persistence
    volumes: [ "redisdata:/data" ]
    ports: [ "6379:6379" ]
    healthcheck:
      test: ["CMD", "redis-cli", "ping"]
      interval: 5s
      timeout: 3s
      retries: 5

  backend:
    build: ./backend
    env_file: .env
    environment:
      REDIS_URL: redis://redis:6379/0
    depends_on:
      redis:
        condition: service_healthy
    ports: [ "8000:8000" ]
    # ASGI server, e.g.: daphne -b 0.0.0.0 -p 8000 config.asgi:application

  frontend:
    build: ./frontend
    env_file: .env            # VITE_* baked at build time
    ports: [ "5173:80" ]      # nginx serving the built SPA
    depends_on: [ backend ]

volumes:
  redisdata:
```

- `docker compose up --build` → web at `http://localhost:5173`, API at
  `http://localhost:8000`. The firmware points at your machine's LAN IP:8000.
- Backend `Dockerfile`: python:3.12-slim → install deps → run Daphne/Uvicorn.
- Frontend `Dockerfile`: multi-stage (node build → nginx serve, SPA fallback).

## 8.2 Free hosting targets

The backend is a single ASGI container + a Redis instance. Pick a free combo:

| Piece     | Free option(s)                                                        |
|-----------|-----------------------------------------------------------------------|
| Backend   | **Render** free web service, **Railway**, **Fly.io**, **Koyeb**       |
| Redis     | **Upstash** (serverless Redis, generous free tier), Redis Cloud 30MB  |
| Frontend  | **Vercel / Netlify / Cloudflare Pages** (static), or same host as API |

Recommended baseline: **Render** (backend container) + **Upstash** (Redis) +
**Cloudflare Pages / Netlify** (static React). All have $0 tiers and support the
WebSocket the real-time feature needs.

### WebSocket support — verify per host
- **Render / Railway / Fly.io / Koyeb**: WebSocket supported. ✅
- Make sure the platform routes `/ws/` to the same service and doesn't strip
  `Upgrade` headers. Use `wss://` (TLS) in production.
- If a chosen host blocks WS, the polling fallback ([`07-realtime.md`](07-realtime.md)
  §7.5) keeps the app functional.

### Free-tier caveats (document for the user)
- **Cold starts / sleep**: free dynos sleep after idle; first request wakes them
  (can take seconds). A periodic ping to `/api/health` mitigates this.
- **Ephemeral disk**: container filesystems are ephemeral. Persistence lives in the
  **managed Redis (Upstash)**, not on the dyno — so enable Redis persistence on the
  provider side; do not rely on a local `redisdata` volume in production.
- **Connection limits**: free Redis tiers cap connections; we use a single shared
  client + the Channels layer, well within limits for one user.

## 8.3 Production run command

ASGI (HTTP + WS in one process):
```
daphne -b 0.0.0.0 -p ${PORT:-8000} config.asgi:application
# or
uvicorn config.asgi:application --host 0.0.0.0 --port ${PORT:-8000}
```
Most hosts inject `$PORT`; bind to it. Run `python manage.py collectstatic` only if
you serve any static via Django (the SPA is served separately, so usually not).

## 8.4 Deploy config per host (sketch)

- **Render**: `render.yaml` — a `web` service `dockerfilePath: backend/Dockerfile`,
  env group with the vars from [`09-env-config.md`](09-env-config.md), `REDIS_URL`
  pointing at Upstash. Add a separate static site for `frontend/`.
- **Railway/Fly**: same idea — service from `backend/Dockerfile`, env vars set in the
  dashboard / `fly.toml`, external `REDIS_URL`.
- **Frontend**: build command `npm run build`, publish `dist/`, set `VITE_*` env.

## 8.5 CI (optional)

- Lint + test backend (pytest) and frontend (vitest) on push.
- Build & push the backend image; trigger host deploy hook.
- Firmware: `pio run` build check (no hardware needed for compile).

## 8.6 Firmware "deployment"

- Flash via `pio run -t upload` over USB.
- OTA is out of scope for v1 (could be added via ESP32 OTA later).
- Wi-Fi/API config is set **on-device at runtime** via the on-watch provisioning AP
  + web UI and persisted to NVS (see [`06-firmware.md`](06-firmware.md) §6.10/§6.12).
  Optional build-time defaults (git-ignored `secrets.h`) can pre-seed a dev unit;
  NVS values take precedence, so changing the target API needs no reflash.
