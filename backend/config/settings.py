"""
Django settings for the WatchTodo backend.

12-factor: every value comes from the environment. No relational database is
configured (Redis is the only persistence). See specs/04-backend.md and
specs/09-env-config.md.
"""
import os
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent


def env(key: str, default: str | None = None) -> str | None:
    return os.environ.get(key, default)


def env_bool(key: str, default: bool = False) -> bool:
    return env(key, str(default)).strip().lower() in ("1", "true", "yes", "on")


def env_list(key: str, default: str = "") -> list[str]:
    raw = env(key, default) or ""
    return [item.strip() for item in raw.split(",") if item.strip()]


# ─── Core ────────────────────────────────────────────────────────────────
SECRET_KEY = env("DJANGO_SECRET_KEY", "insecure-dev-key-change-me")
DEBUG = env_bool("DEBUG", False)
ALLOWED_HOSTS = env_list("ALLOWED_HOSTS", "localhost,127.0.0.1")

# Render injects the external hostname; trust it automatically so the host does
# not have to be hard-coded in env. (Harmless elsewhere — only added if set.)
_render_host = env("RENDER_EXTERNAL_HOSTNAME")
if _render_host and _render_host not in ALLOWED_HOSTS:
    ALLOWED_HOSTS.append(_render_host)

# App-specific config
API_KEY = env("API_KEY", "")
REDIS_URL = env("REDIS_URL", "redis://localhost:6379/0")
WS_PING_INTERVAL = int(env("WS_PING_INTERVAL", "25"))
POLL_INTERVAL = int(env("POLL_INTERVAL", "5"))

# ─── Apps / middleware ───────────────────────────────────────────────────
INSTALLED_APPS = [
    "daphne",
    "corsheaders",
    "rest_framework",
    "channels",
    "notes",
]

MIDDLEWARE = [
    "corsheaders.middleware.CorsMiddleware",
    "django.middleware.common.CommonMiddleware",
]

ROOT_URLCONF = "config.urls"
ASGI_APPLICATION = "config.asgi.application"

TEMPLATES = [
    {
        "BACKEND": "django.template.backends.django.DjangoTemplates",
        "DIRS": [],
        "APP_DIRS": True,
        "OPTIONS": {"context_processors": []},
    },
]

# ─── No relational DB ────────────────────────────────────────────────────
# Django insists on a DATABASES entry; use the dummy backend so nothing ever
# touches SQL. All data lives in Redis (notes/store.py).
DATABASES = {"default": {"ENGINE": "django.db.backends.dummy"}}

# ─── DRF ─────────────────────────────────────────────────────────────────
REST_FRAMEWORK = {
    # No Django auth — a single shared API key gates everything (notes/auth.py).
    "DEFAULT_AUTHENTICATION_CLASSES": ["notes.auth.ApiKeyAuthentication"],
    "DEFAULT_PERMISSION_CLASSES": ["notes.auth.HasApiKey"],
    "UNAUTHENTICATED_USER": None,
}

# ─── Channels ────────────────────────────────────────────────────────────
if env_bool("USE_INMEMORY_CHANNEL_LAYER", False):
    CHANNEL_LAYERS = {
        "default": {"BACKEND": "channels.layers.InMemoryChannelLayer"},
    }
else:
    CHANNEL_LAYERS = {
        "default": {
            "BACKEND": "channels_redis.core.RedisChannelLayer",
            "CONFIG": {"hosts": [REDIS_URL]},
        },
    }

# ─── CORS ────────────────────────────────────────────────────────────────
CORS_ALLOWED_ORIGINS = env_list("CORS_ALLOWED_ORIGINS", "http://localhost:5173")
CORS_ALLOW_HEADERS = [
    "accept",
    "content-type",
    "x-api-key",
]

# ─── i18n / static ───────────────────────────────────────────────────────
LANGUAGE_CODE = "en-us"
TIME_ZONE = "UTC"
USE_I18N = False
USE_TZ = True

DEFAULT_AUTO_FIELD = "django.db.models.BigAutoField"

# ─── Single-image deploy: serve the built React SPA from this app ─────────
# The Docker build copies frontend/dist -> /app/web. When present, Django serves
# the SPA (same origin as the API + WebSocket), so no separate frontend service,
# no CORS, and no cross-service URLs are needed. Absent (plain `manage.py` /
# API-only dev) -> only /api is served and the React dev server is used instead.
WEB_DIR = BASE_DIR / "web"
SERVE_FRONTEND = (WEB_DIR / "index.html").exists()

# ─── Logging (stdout, 12-factor) ─────────────────────────────────────────
LOGGING = {
    "version": 1,
    "disable_existing_loggers": False,
    "handlers": {"console": {"class": "logging.StreamHandler"}},
    "root": {"handlers": ["console"], "level": env("LOG_LEVEL", "INFO")},
}
