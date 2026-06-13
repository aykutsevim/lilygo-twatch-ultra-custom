# 04 — Backend (Django + DRF + Channels + Redis)

## 4.1 Stack

- **Python** 3.12
- **Django** 5.x (ASGI)
- **Django REST Framework** — REST CRUD
- **Channels** 4.x + **channels-redis** — WebSocket + Redis channel layer
- **redis** (redis-py, async client) — data store access
- **Daphne** (or Uvicorn) — ASGI server
- **gunicorn** is **not** used for the ASGI app (it's sync); run Daphne/Uvicorn.

> One ASGI process serves both HTTP (DRF) and WebSocket (Channels) via a
> `ProtocolTypeRouter`. No separate WS server.

## 4.2 Project layout

```
backend/
├── manage.py
├── pyproject.toml          # or requirements.txt
├── Dockerfile
├── .dockerignore
├── config/
│   ├── settings.py         # 12-factor: all config from env
│   ├── asgi.py             # ProtocolTypeRouter(http=DRF, websocket=Channels)
│   └── urls.py
└── notes/
    ├── store.py            # NoteStore: Redis repository (the only data access)
    ├── views.py            # DRF APIViews / ViewSet
    ├── serializers.py      # validation (text length, etc.)
    ├── consumers.py        # NotesConsumer (Channels)
    ├── events.py           # broadcast helpers (group send)
    ├── auth.py             # X-API-Key check (REST) + key check (WS)
    └── urls.py             # /api/... routes
```

## 4.3 Redis data model

Single Redis instance, namespaced keys:

| Key                | Type        | Contents                                             |
|--------------------|-------------|------------------------------------------------------|
| `notes:rev`        | string(int) | Global revision counter (`INCR` on each mutation)    |
| `notes:seq`        | string(int) | ID sequence (`INCR` → `n_%06d`)                      |
| `notes:data`       | hash        | field = note id, value = JSON of the Note object     |
| `notes:order`      | zset        | member = note id, score = `created_at` (ms)          |

- **List** = `ZRANGE notes:order 0 -1` → ids in creation order → `HMGET notes:data …`.
- **Create**: `INCR notes:seq` → build Note → `HSET notes:data id json` +
  `ZADD notes:order created_at id` → `INCR notes:rev` → broadcast `note.created`.
- **Update**: read JSON from `notes:data`, mutate `text`/`done`, bump `updated_at`,
  `HSET` → `INCR notes:rev` → broadcast `note.updated`.
- **Delete**: `HDEL notes:data id` + `ZREM notes:order id` → `INCR notes:rev` →
  broadcast `note.deleted`.
- **Clear done**: scan `notes:data`, collect done ids, pipeline `HDEL`/`ZREM` →
  one `INCR notes:rev` → broadcast `notes.cleared` with the id list.

Use a **`MULTI`/pipeline** per mutation so the `notes:rev` bump and the data write
commit together. `NoteStore` exposes async methods: `list()`, `create(text)`,
`update(id, **fields)`, `delete(id)`, `clear_done()`, each returning the new `rev`
and the affected note(s) so the caller can broadcast.

### Channel layer keys
`channels-redis` uses its own `asgi:*` keys in the same Redis. Namespaces don't
collide with `notes:*`. Configure `CHANNEL_LAYERS` with `REDIS_URL`.

## 4.4 Auth

- A single shared secret `API_KEY` (env). 
- REST: a DRF permission / middleware checks `X-API-Key` on every `/api/*` route
  except `/api/health`. Mismatch → 401.
- WS: `NotesConsumer.connect()` reads `?key=` from the query string; mismatch →
  `close(code=4401)`.
- Constant-time compare (`hmac.compare_digest`). The key is never logged.
- This is deliberately the **only** auth (single-user system). It keeps casual
  access out; it is not a hardened multi-user scheme.

## 4.5 Consumer behavior (`NotesConsumer`)

- `connect`: validate key → `accept()` → join group `notes` → send `hello` with
  current `rev`.
- `receive_json`: handle `toggle` / `delete` / `pong`. For `toggle`/`delete`, call
  `NoteStore`, then broadcast (same path as REST). Ignore unknown types.
- Group message handler forwards the broadcast frame to the socket as JSON.
- `disconnect`: leave group.
- Server sends `ping` every `WS_PING_INTERVAL` (env, default 25s) so idle
  proxies/free-tier load balancers don't drop the socket.

## 4.6 Broadcasting (`events.py`)

```python
async def broadcast(event: dict):
    layer = get_channel_layer()
    await layer.group_send("notes", {"type": "note.event", "payload": event})
```
`NotesConsumer.note_event(self, msg)` → `await self.send_json(msg["payload"])`.
REST views call `async_to_sync(broadcast)(event)` (or are async views) after a
successful mutation.

## 4.7 Settings (env-driven)

All config via env (see [`09-env-config.md`](09-env-config.md)). Notably:
`DJANGO_SECRET_KEY`, `DEBUG`, `ALLOWED_HOSTS`, `CORS_ALLOWED_ORIGINS`,
`REDIS_URL`, `API_KEY`, `WS_PING_INTERVAL`. No database is configured —
remove/disable `django.db` usage; set `DATABASES = {}` is not allowed by Django,
so use the dummy backend:
```python
DATABASES = {"default": {"ENGINE": "django.db.backends.dummy"}}
```
Do not enable `django.contrib.admin`/`auth`/`sessions` migrations that require a DB;
keep `INSTALLED_APPS` minimal (no admin, no sessions). Use signed-cookie session
backend only if ever needed (not needed here).

## 4.8 Health & ops

- `GET /api/health` returns `{"status":"ok"}` and pings Redis (`PING`); returns 503
  if Redis is unreachable so the host can restart the container.
- Structured logging to stdout (12-factor). Log level from `LOG_LEVEL` env.
- Optional simple rate limit (DRF throttle, e.g. 120/min) keyed by API key/IP to
  avoid runaway loops from a buggy client.

## 4.9 Tests

- `NoteStore` unit tests against a real Redis (the compose `redis` service or
  `fakeredis` in CI).
- REST contract tests (DRF `APIClient`): create/list/patch/delete/clear, auth 401,
  validation 400, ordering, `rev` increments.
- Channels tests (`ChannelsLiveServerTestCase` / `WebsocketCommunicator`): connect
  with/without key, receive `hello`, receive broadcast after a REST mutation.

## 4.10 Dependencies (indicative)

```
django>=5.0
djangorestframework
channels>=4
channels-redis
redis>=5
daphne
django-cors-headers
```
