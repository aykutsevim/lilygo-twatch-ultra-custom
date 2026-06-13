# Backend — WatchTodo API

Django (ASGI) + DRF + Channels, with **Redis** as the only data store. Serves the
REST CRUD API and a WebSocket (`/ws/notes/`) that pushes changes to all clients in
real time. See [`../specs/04-backend.md`](../specs/04-backend.md) and
[`../specs/03-api-contract.md`](../specs/03-api-contract.md).

## Run with Docker (recommended)

From the repo root:

```bash
cp .env.example .env        # then edit API_KEY etc.
docker compose up --build   # backend on :8000, redis on :6379
```

Health check: `GET http://localhost:8000/api/health` → `{"status":"ok"}`.

## Run locally (without Docker)

Needs a Redis reachable at `REDIS_URL` (e.g. `docker run -p 6379:6379 redis:7-alpine`).

```bash
cd backend
python -m venv .venv
. .venv/Scripts/activate        # Windows; use source .venv/bin/activate on *nix
pip install -r requirements.txt

# minimal env
export API_KEY=dev-key DJANGO_SECRET_KEY=dev REDIS_URL=redis://localhost:6379/0

daphne -b 0.0.0.0 -p 8000 config.asgi:application
```

## Quick smoke test (curl)

```bash
KEY=dev-key
curl -s localhost:8000/api/health
curl -s -X POST localhost:8000/api/notes -H "X-API-Key: $KEY" \
     -H 'Content-Type: application/json' -d '{"text":"buy milk"}'
curl -s localhost:8000/api/notes -H "X-API-Key: $KEY"
```

Open a WebSocket (e.g. `websocat "ws://localhost:8000/ws/notes/?key=$KEY"`) in
another terminal, then POST a note — the socket receives a `note.created` frame.

## Tests

```bash
cd backend
pip install -r requirements.txt
pytest               # uses fakeredis + in-memory channel layer; no services needed
```

19 tests cover the Redis store, the REST contract (auth/validation/ordering/`rev`),
and the WebSocket consumer (auth, `hello`, broadcast on action).

## Layout

```
config/      settings (env-driven), asgi entrypoint, urls, test settings
notes/       store.py (Redis repo) · views · serializers · auth · consumers · events · routing
tests/       store / api / ws tests   (conftest.py at backend/ root)
```
