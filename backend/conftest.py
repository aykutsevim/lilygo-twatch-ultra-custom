"""
Test fixtures. Required env is set by config/settings_test.py (the test settings
module), so it is applied before pytest-django imports settings.

These fixtures inject a fakeredis-backed NoteStore so tests need no real Redis;
the in-memory channel layer (test settings) makes broadcasts work without Redis.
"""
import fakeredis
import pytest


@pytest.fixture(autouse=True)
def fake_store(monkeypatch):
    """Replace the process-wide NoteStore with a fakeredis-backed one."""
    from notes import store as store_mod

    client = fakeredis.FakeStrictRedis(decode_responses=True)
    s = store_mod.NoteStore(client)
    monkeypatch.setattr(store_mod, "_STORE", s)
    return s


@pytest.fixture
def api_key():
    return "test-key"
