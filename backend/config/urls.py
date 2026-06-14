from django.conf import settings
from django.urls import include, path, re_path
from django.views.static import serve

from .spa import spa_index

urlpatterns = [
    path("api/", include("notes.urls")),
]

# When the built SPA is bundled into the image, serve it from this app: hashed
# assets under /assets, everything else falls back to index.html (SPA routing).
# (/ws is handled by Channels, not this HTTP urlconf.)
if settings.SERVE_FRONTEND:
    urlpatterns += [
        re_path(r"^assets/(?P<path>.*)$", serve,
                {"document_root": str(settings.WEB_DIR / "assets")}),
        re_path(r"^.*$", spa_index),  # catch-all AFTER /api and /assets
    ]
