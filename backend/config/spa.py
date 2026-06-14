"""Serve the built React index.html, injecting the API key at request time.

The frontend ships a placeholder `"__WATCHTODO_API_KEY__"` in a runtime-config
script; we replace it with the live API_KEY so the same image works with any key
without rebuilding. The web key is not a real secret (single-user design).
"""
import json
from functools import lru_cache

from django.conf import settings
from django.http import HttpResponse


@lru_cache(maxsize=1)
def _index_html() -> str:
    raw = (settings.WEB_DIR / "index.html").read_text(encoding="utf-8")
    return raw.replace('"__WATCHTODO_API_KEY__"', json.dumps(settings.API_KEY))


def spa_index(request, *args, **kwargs):
    return HttpResponse(_index_html(), content_type="text/html; charset=utf-8")
