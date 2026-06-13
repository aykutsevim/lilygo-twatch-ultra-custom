"""
Test settings. Sets the env the base settings read BEFORE importing them, so
values are deterministic regardless of when pytest-django imports settings.
No real Redis is needed (fakeredis via fixtures; in-memory channel layer).
"""
import os

os.environ["DJANGO_SECRET_KEY"] = "test-secret"
os.environ["API_KEY"] = "test-key"
os.environ.setdefault("REDIS_URL", "redis://localhost:6379/0")
os.environ["USE_INMEMORY_CHANNEL_LAYER"] = "1"
os.environ["WS_PING_INTERVAL"] = "1"

from .settings import *  # noqa: F401,F403,E402
