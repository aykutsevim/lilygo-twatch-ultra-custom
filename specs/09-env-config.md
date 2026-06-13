# 09 — Environment & configuration

All runtime config is supplied via environment variables (12-factor). No secrets in
source. A root `.env.example` documents every variable; copy to `.env` for local dev
and set the same vars in the hosting dashboard for production.

## 9.1 Root `.env.example`

```dotenv
# ─── Shared ──────────────────────────────────────────────────────────────
# Single shared key gating the API (the only "auth"). Use a long random string.
API_KEY=change-me-to-a-long-random-string

# ─── Backend (Django) ────────────────────────────────────────────────────
DJANGO_SECRET_KEY=change-me
DEBUG=false
ALLOWED_HOSTS=localhost,127.0.0.1,watchtodo.example.com
CORS_ALLOWED_ORIGINS=http://localhost:5173,https://watchtodo-web.example.com
# Redis: local compose uses redis://redis:6379/0; prod uses the Upstash URL.
REDIS_URL=redis://redis:6379/0
# WebSocket keepalive ping interval (seconds)
WS_PING_INTERVAL=25
# Optional polling fallback hint surfaced to clients (seconds)
POLL_INTERVAL=5
LOG_LEVEL=INFO
# Port is usually injected by the host; default for local/compose:
PORT=8000

# ─── Frontend (Vite, build-time) ─────────────────────────────────────────
# These are baked into the static bundle at build time.
VITE_API_BASE=http://localhost:8000
VITE_WS_BASE=ws://localhost:8000
VITE_API_KEY=change-me-to-a-long-random-string   # = API_KEY (not a real secret in-browser)
```

## 9.2 Variable reference

| Variable               | Component | Required | Default            | Description                                         |
|------------------------|-----------|----------|--------------------|-----------------------------------------------------|
| `API_KEY`              | all       | yes      | —                  | Shared key; `X-API-Key` header / `?key=` on WS      |
| `DJANGO_SECRET_KEY`    | backend   | yes      | —                  | Django crypto signing key                           |
| `DEBUG`                | backend   | no       | `false`            | Never `true` in production                          |
| `ALLOWED_HOSTS`        | backend   | yes      | `localhost`        | Comma-separated hostnames Django will serve         |
| `CORS_ALLOWED_ORIGINS` | backend   | yes      | —                  | Comma-separated web origins allowed (no `*` w/ creds)|
| `REDIS_URL`            | backend   | yes      | —                  | Redis connection (data store + channel layer)       |
| `WS_PING_INTERVAL`     | backend   | no       | `25`               | Seconds between server keepalive pings              |
| `POLL_INTERVAL`        | backend   | no       | `5`                | Fallback poll interval hint (seconds)               |
| `LOG_LEVEL`            | backend   | no       | `INFO`             | Python log level                                    |
| `PORT`                 | backend   | no       | `8000`             | Bind port (host usually injects this)               |
| `VITE_API_BASE`        | frontend  | yes      | —                  | REST base URL (build-time)                          |
| `VITE_WS_BASE`         | frontend  | yes      | —                  | WebSocket base URL (build-time)                     |
| `VITE_API_KEY`         | frontend  | yes      | —                  | Same value as `API_KEY`                             |

## 9.3 Firmware config (runtime NVS primary, build-time optional)

The watch doesn't read `.env`. Its config is **provisioned on-device at runtime**
(on-watch AP + web UI) and persisted to **NVS**; git-ignored build-time defaults can
pre-seed a dev unit. NVS takes precedence. See [`06-firmware.md`](06-firmware.md)
§6.10/§6.12.

| Key (NVS) / Macro (build-time) | Maps to         | Notes                              |
|--------------------------------|-----------------|------------------------------------|
| `wifi_ssid` / `WIFI_SSID`      | —               | Wi-Fi network name (provisioned)   |
| `wifi_pass` / `WIFI_PASS`      | —               | Wi-Fi password (provisioned)       |
| `api_base` / `DEFAULT_API_BASE`| `VITE_API_BASE` | REST base (`https://…` in prod)    |
| `ws_base`  / `DEFAULT_WS_BASE` | `VITE_WS_BASE`  | WebSocket base (`wss://…` in prod) |
| `api_key`  / (secrets.h)       | `API_KEY`       | Same shared key (`X-API-Key`/`?key=`) |

Real build-time values go in a git-ignored `firmware/include/secrets.h` (or
PlatformIO `build_flags`); placeholders live in committed `include/config.h`.

## 9.4 Secrets hygiene

- `.env`, `firmware/include/secrets.h`, and any `*.local` files are git-ignored.
- Only `.env.example` and `config.h` (placeholders) are committed.
- Production secrets live in the host's env/secret store, never in the repo.
- `API_KEY` shipped to the browser is **not** confidential by design (single-user,
  no-auth product). It deters casual access only; do not reuse it for anything that
  must stay secret. Rotate it by changing the env var on backend + frontend build.

## 9.5 `.gitignore` (root, indicative)

```
.env
*.local
firmware/include/secrets.h
**/__pycache__/
node_modules/
dist/
.pio/
```
