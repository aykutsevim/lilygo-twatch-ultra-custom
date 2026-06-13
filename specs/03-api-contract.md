# 03 — API Contract

The contract is shared by the React web app and the watch firmware. Treat this file
as the source of truth; backend, frontend, and firmware must agree on it.

## 3.1 Conventions

- Base URL: `${API_BASE}` (e.g. `https://watchtodo.example.com`).
- All bodies are JSON; `Content-Type: application/json`.
- **Auth**: every request (REST and WS) must send the shared API key.
  - REST: header `X-API-Key: <key>`.
  - WebSocket: query param `?key=<key>` (header support is unreliable on embedded
    WS clients). Connections without a valid key are closed with code `4401`.
- Times are Unix epoch **milliseconds** (integer, UTC).
- IDs are server-generated, URL-safe strings (e.g. base62 of a counter, or UUIDv4
  hex). Clients never invent IDs.

## 3.2 Note object

```jsonc
{
  "id":        "n_000123",      // string, server-assigned, stable
  "text":      "Buy oat milk",  // string, 1..280 chars, trimmed, required
  "done":      false,           // boolean
  "created_at": 1718300000000,  // epoch ms
  "updated_at": 1718300000000   // epoch ms
}
```

Validation:
- `text` is required on create, trimmed, length 1–280 after trim. Reject empty.
- Unknown fields are ignored. `id`, `created_at`, `updated_at` are read-only.

## 3.3 REST endpoints

| Method | Path                       | Body                | Returns                  | Notes                        |
|--------|----------------------------|---------------------|--------------------------|------------------------------|
| GET    | `/api/health`              | —                   | `{"status":"ok"}`        | No auth; for host healthcheck|
| GET    | `/api/notes`               | —                   | `{ "rev": N, "notes": [Note…] }` | Full snapshot (resync) |
| POST   | `/api/notes`               | `{ "text": "…" }`   | `201` + `Note`           | Create                       |
| PATCH  | `/api/notes/{id}`          | `{ "text"?, "done"? }` | `200` + `Note`        | Partial update               |
| DELETE | `/api/notes/{id}`          | —                   | `204`                    | Delete one                   |
| POST   | `/api/notes/clear-done`    | —                   | `200` + `{ "deleted": M }` | Delete all completed       |

- `GET /api/notes` returns notes **sorted by `created_at` ascending** plus the
  current global `rev` (see §3.5). The watch and web both rely on this ordering.
- 404 for unknown `{id}` on PATCH/DELETE.
- 400 with `{ "error": "…", "field": "text" }` on validation failure.
- 401 `{ "error": "unauthorized" }` when `X-API-Key` is missing/wrong.

## 3.4 WebSocket: `/ws/notes/?key=<key>`

Server→client event frames (JSON):

```jsonc
// emitted on any mutation, to every connected client
{ "type": "note.created", "rev": 12, "note": { …Note } }
{ "type": "note.updated", "rev": 13, "note": { …Note } }
{ "type": "note.deleted", "rev": 14, "id": "n_000123" }
{ "type": "notes.cleared","rev": 15, "ids": ["n_…","n_…"] }

// sent once right after connect so a client can detect it is behind
{ "type": "hello", "rev": 15 }

// server keepalive (every ~25s); clients may also send their own ping
{ "type": "ping", "ts": 1718300000000 }
```

Client→server frames (optional; used by the watch for light actions and by both
for liveness). The server validates and then performs the same logic as the REST
handler, emitting the corresponding broadcast:

```jsonc
{ "type": "toggle", "id": "n_000123" }   // flip done
{ "type": "delete", "id": "n_000123" }
{ "type": "pong" }                         // reply to ping (optional)
```

Rules:
- After `hello`, if a client's last-applied `rev` < server `rev`, it must call
  `GET /api/notes` to resync, then resume applying events.
- Clients ignore any event whose `rev` ≤ their last applied `rev` (dedupe/echo).
- If an incoming event's `rev` is more than 1 greater than the last applied `rev`,
  the client assumes a gap and triggers a full resync.

## 3.5 Revision counter (`rev`)

- A single global integer in Redis (`INCR notes:rev`) bumped on every successful
  mutation. It is returned by `GET /api/notes` and stamped on every WS event.
- Purpose: cheap, ordering-correct way for tiny clients (the watch) to know whether
  they are current without diffing the whole list.

## 3.6 Errors (shape)

```jsonc
{ "error": "human readable message", "field": "text" /* optional */ }
```

Status codes: 200/201/204 success; 400 validation; 401 auth; 404 not found;
429 rate-limited (optional, see backend spec); 500 server error.

## 3.7 CORS

- Allow the frontend origin(s) from `CORS_ALLOWED_ORIGINS` (env). The watch is not
  a browser, so CORS does not gate it, but the same origins list is used for the
  web app. Do **not** use `*` together with credentials.
