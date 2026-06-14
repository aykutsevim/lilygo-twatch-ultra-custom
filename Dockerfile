# Single image: builds the React SPA, then bundles it into the Django ASGI app
# which serves the SPA + REST + WebSocket on one port. Build context = repo root.

# ── Stage 1: build the frontend ──
FROM node:20-alpine AS web
WORKDIR /web
COPY frontend/package.json frontend/package-lock.json* ./
RUN npm ci || npm install
COPY frontend/ ./
RUN npm run build           # -> /web/dist

# ── Stage 2: backend + bundled SPA ──
FROM python:3.12-slim
ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    PIP_NO_CACHE_DIR=1
WORKDIR /app
COPY backend/requirements.txt .
RUN pip install -r requirements.txt
COPY backend/ .
# Django serves these (settings.WEB_DIR = /app/web; see config/urls.py).
COPY --from=web /web/dist ./web

EXPOSE 8000
# ASGI server: HTTP (REST + SPA) and WebSocket in one process. $PORT is injected
# by most hosts; default 8000 for local/compose.
CMD ["sh", "-c", "daphne -b 0.0.0.0 -p ${PORT:-8000} config.asgi:application"]
