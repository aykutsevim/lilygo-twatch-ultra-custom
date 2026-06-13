# 07 — Real-time updates

The defining feature: a note typed in the browser appears on the watch
**immediately** (target < 1s). This spec pins down the mechanism so all three
clients behave consistently.

## 7.1 Mechanism: WebSocket fan-out via Channels + Redis

- Every client (web + watch) holds an open WebSocket to `/ws/notes/`.
- The server keeps all sockets in a single Channels **group** `notes`.
- On any mutation (from REST or from a watch WS action), the server:
  1. writes to Redis (source of truth) and `INCR notes:rev`,
  2. `group_send` an event to `notes`,
  3. `channels-redis` uses Redis pub/sub to deliver to every worker holding a
     socket, which forwards the frame to the client.
- Result: one round-trip from writer → server → Redis → all sockets. Sub-second on
  normal networks.

```
web POST ─▶ Django ─▶ Redis(write + rev) ─▶ group_send ─▶ Redis pub/sub ─▶ all sockets ─▶ web + watch render
```

## 7.2 Why not alternatives

| Option                | Verdict | Reason                                                       |
|-----------------------|---------|--------------------------------------------------------------|
| **WebSocket (chosen)**| ✅      | True push, low latency, works for both web and ESP32         |
| Short polling         | ❌ v1   | Simple but adds latency/load; "immediately" needs ≤ poll int |
| SSE                   | ➖      | Fine for web; one-way only and clumsier on ESP32 than WS     |
| MQTT broker           | ➖      | Great for IoT push but adds a second server + broker hosting |

WebSocket keeps one server, one Redis, and a single code path for web + watch.
(Short polling is documented in §7.5 as a fallback only.)

## 7.3 Ordering & idempotency — the `rev` counter

- Global monotonic `rev` (`INCR notes:rev`) stamped on every event and returned by
  `GET /api/notes`.
- Each client tracks `local.rev`. Apply rule:
  - `event.rev <= local.rev` → ignore (duplicate/echo).
  - `event.rev == local.rev + 1` → apply, set `local.rev = event.rev`.
  - `event.rev > local.rev + 1` → **gap** → full `GET /api/notes` resync.
- This makes missed/duplicate/out-of-order events self-healing without per-note
  version vectors — important for the tiny watch client.

## 7.4 Connection lifecycle

- **Connect** → server sends `hello{rev}`. Client resyncs if behind.
- **Keepalive** → server `ping` every `WS_PING_INTERVAL` (default 25s) so free-tier
  proxies/load balancers don't idle-close the socket. Clients may `pong`.
- **Drop** → client reconnects with exponential backoff + jitter; `hello` on
  reconnect triggers resync. The server's authority is always Redis, so no events
  are "lost" in a way that matters.

## 7.5 Fallback (degraded mode)

If WebSocket is unavailable (e.g. a host/proxy that blocks WS), clients fall back to
polling `GET /api/notes` every `POLL_INTERVAL` (default 5s) and diff by `rev`. This
sacrifices "immediate" for "eventually within N seconds" but keeps the app working.
The watch implements this fallback after repeated WS connect failures.

## 7.6 Latency budget (target < 1s)

| Hop                              | Budget   |
|----------------------------------|----------|
| Browser → server (POST)          | ~100ms   |
| Redis write + `group_send`       | ~10ms    |
| Redis pub/sub → socket worker    | ~10ms    |
| Server → watch frame + parse     | ~100ms   |
| LVGL re-render                   | ~50ms    |
| **Total typical**                | **< 1s** |

Free-tier cold starts can blow this budget on the *first* request after idle;
document that and consider a lightweight keepalive ping to the health endpoint.
