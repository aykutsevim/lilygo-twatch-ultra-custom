# 05 — Frontend (React)

## 5.1 Stack

- **React 18** + **TypeScript**
- **Vite** (dev server + build)
- Plain `fetch` + native `WebSocket` (no heavy data lib required). Optional:
  TanStack Query if preferred — not required for this scale.
- Minimal styling: CSS modules or a tiny utility set. No component library mandated.

## 5.2 Responsibilities

- Render the todo list (ordered by `created_at`, same as API).
- Add a note (input + Enter / button).
- Toggle done (checkbox), delete (button), "Clear completed".
- Reflect live changes pushed from the server (other clients, or the watch).
- Show connection status (connected / reconnecting / offline).

## 5.3 Layout

```
frontend/
├── index.html
├── package.json
├── vite.config.ts
├── Dockerfile            # multi-stage: build → nginx static serve
├── nginx.conf            # SPA fallback to index.html
├── .env.example          # VITE_API_BASE, VITE_WS_BASE, VITE_API_KEY
└── src/
    ├── main.tsx
    ├── App.tsx
    ├── api.ts            # REST calls (adds X-API-Key)
    ├── ws.ts             # WebSocket client w/ reconnect + rev tracking
    ├── store.ts          # notes state + reducer (apply events)
    ├── types.ts          # Note, events (mirror 03-api-contract.md)
    └── components/
        ├── NoteList.tsx
        ├── NoteItem.tsx
        ├── AddNote.tsx
        └── StatusBar.tsx
```

## 5.4 Config (build-time env)

Vite exposes `import.meta.env.VITE_*`:

| Var             | Example                              | Use                          |
|-----------------|--------------------------------------|------------------------------|
| `VITE_API_BASE` | `https://watchtodo.example.com`      | REST base URL                |
| `VITE_WS_BASE`  | `wss://watchtodo.example.com`        | WebSocket base               |
| `VITE_API_KEY`  | `dev-shared-key`                     | Shared API key (X-API-Key)   |

> Note: in a single-user, no-auth system the API key shipped to the browser is not
> a real secret — it only deters casual access. This is an accepted trade-off
> (see [`01-overview.md`](01-overview.md) §1.3/1.4). Do not treat the web key as
> confidential.

## 5.5 Data flow / state

1. On mount: `GET /api/notes` → seed `store` with `notes` and `rev`.
2. Open WS `${VITE_WS_BASE}/ws/notes/?key=…`.
3. On `hello`: if `server.rev > local.rev`, re-fetch `GET /api/notes` (resync).
4. On `note.*` / `notes.cleared` events: apply to store **only if** `event.rev >
   local.rev`; bump `local.rev`. If `event.rev > local.rev + 1` → full resync.
5. Mutations are **REST** calls. The optimistic update is optional; simplest is to
   wait for the WS echo (sub-second) to update the list, deduping by `id`. Choose
   one approach and keep it consistent.

## 5.6 Reconnection

- WS client uses exponential backoff (e.g. 0.5s → max 10s) with jitter.
- While disconnected, the status bar shows "reconnecting"; mutations still work via
  REST (server broadcasts; this client resyncs on reconnect).
- On `visibilitychange` back to visible, re-check connection and resync if needed.

## 5.7 Accessibility / UX niceties

- Input autofocus; Enter to add; Escape clears input.
- Empty state ("No todos — add one").
- Each item: checkbox, text (strike-through when done), delete button.
- Live region announces connection status changes.

## 5.8 Build & serve

- Dev: `npm run dev` (Vite, proxies `/api` and `/ws` to backend in `vite.config.ts`
  to avoid CORS during development).
- Prod: `npm run build` → static assets served by **nginx** (Dockerfile multi-stage)
  with SPA fallback. The frontend can also be deployed to any static host
  (Netlify/Vercel/Cloudflare Pages free tier) pointing at the deployed API.
