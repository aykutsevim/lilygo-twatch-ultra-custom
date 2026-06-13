"""
The only "auth": a single shared API key (single-user system).

REST: X-API-Key header. An authentication class validates it (so DRF returns a
      proper 401, not 403, on failure) and a permission gates the views.
WS:   ?key=<key> query param, checked in the consumer (see check_key).
"""
import hmac

from django.conf import settings
from rest_framework.authentication import BaseAuthentication
from rest_framework.exceptions import AuthenticationFailed
from rest_framework.permissions import BasePermission


def check_key(provided: str | None) -> bool:
    expected = settings.API_KEY or ""
    if not expected:
        # Misconfiguration: refuse everything rather than allow all.
        return False
    return hmac.compare_digest(provided or "", expected)


class _ApiUser:
    """Sentinel principal for the single user. Not a DB-backed Django user."""

    is_authenticated = True
    is_active = True

    def __str__(self):  # pragma: no cover - cosmetic
        return "api"


API_USER = _ApiUser()


class ApiKeyAuthentication(BaseAuthentication):
    def authenticate(self, request):
        provided = request.headers.get("X-API-Key")
        if provided is None:
            return None  # anonymous; the permission denies -> 401
        if not check_key(provided):
            raise AuthenticationFailed("unauthorized")
        return (API_USER, None)

    def authenticate_header(self, request):
        # Returning a value makes DRF answer 401 (not 403) on auth failure.
        return "X-API-Key"


class HasApiKey(BasePermission):
    message = "unauthorized"

    def has_permission(self, request, view) -> bool:
        if getattr(view, "public", False):
            return True
        return bool(getattr(request.user, "is_authenticated", False))
