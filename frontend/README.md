# Frontend — WatchTodo web app

React 18 + TypeScript + Vite. Loads notes via REST, then opens the WebSocket for
live updates; mutations are REST calls and the UI updates from the resulting WS
broadcast. See [`../specs/05-frontend.md`](../specs/05-frontend.md).

## Dev

```bash
cd frontend
cp .env.example .env        # set VITE_API_KEY = backend API_KEY
npm install
npm run dev                 # http://localhost:5173
```

The backend must be running (e.g. `docker compose up` from the repo root, or
`daphne ... config.asgi:application`). CORS for `:5173` is already configured on
the backend, so the absolute `VITE_API_BASE` / `VITE_WS_BASE` work directly.
Alternatively leave those empty to use the Vite dev proxy (same-origin `/api` + `/ws`).

## Build (production static)

```bash
npm run build               # type-checks then emits dist/
npm run preview             # serve the build locally
```

`VITE_*` are baked in at build time. For the Docker image, pass them as build args
(see `Dockerfile` and the `frontend` service in the repo `docker-compose.yml`).

## Layout

```
src/
  config.ts        env + URL helpers
  api.ts           REST client (adds X-API-Key)
  ws.ts            WebSocket client (reconnect + backoff)
  store.ts         reducer applying events by `rev`
  useNotes.ts      orchestration hook (REST seed + WS reconcile + mutations)
  App.tsx          composition
  components/      AddNote, NoteList, NoteItem, StatusBar
```
