"""REST contract tests (specs/03-api-contract.md §3.3)."""
import pytest
from rest_framework.test import APIClient

AUTH = {"HTTP_X_API_KEY": "test-key"}


@pytest.fixture
def client():
    return APIClient()


def test_health_is_public(client):
    resp = client.get("/api/health")
    assert resp.status_code == 200
    assert resp.json()["status"] == "ok"


def test_requires_api_key(client):
    assert client.get("/api/notes").status_code == 401
    assert client.get("/api/notes", HTTP_X_API_KEY="wrong").status_code == 401


def test_list_empty(client):
    resp = client.get("/api/notes", **AUTH)
    assert resp.status_code == 200
    assert resp.json() == {"rev": 0, "notes": []}


def test_create_and_list(client):
    resp = client.post("/api/notes", {"text": "buy milk"}, format="json", **AUTH)
    assert resp.status_code == 201
    note = resp.json()
    assert note["text"] == "buy milk"
    assert note["done"] is False
    assert note["id"]

    listed = client.get("/api/notes", **AUTH).json()
    assert listed["rev"] == 1
    assert listed["notes"][0]["id"] == note["id"]


def test_create_validation(client):
    assert client.post("/api/notes", {"text": "   "}, format="json", **AUTH).status_code == 400
    assert client.post("/api/notes", {}, format="json", **AUTH).status_code == 400
    long = "x" * 281
    assert client.post("/api/notes", {"text": long}, format="json", **AUTH).status_code == 400


def test_patch_toggle_and_text(client):
    note = client.post("/api/notes", {"text": "a"}, format="json", **AUTH).json()
    resp = client.patch(
        f"/api/notes/{note['id']}", {"done": True, "text": "b"}, format="json", **AUTH
    )
    assert resp.status_code == 200
    updated = resp.json()
    assert updated["done"] is True
    assert updated["text"] == "b"


def test_patch_missing_404(client):
    resp = client.patch("/api/notes/n_999999", {"done": True}, format="json", **AUTH)
    assert resp.status_code == 404


def test_delete(client):
    note = client.post("/api/notes", {"text": "a"}, format="json", **AUTH).json()
    assert client.delete(f"/api/notes/{note['id']}", **AUTH).status_code == 204
    assert client.delete(f"/api/notes/{note['id']}", **AUTH).status_code == 404


def test_clear_done(client):
    keep = client.post("/api/notes", {"text": "keep"}, format="json", **AUTH).json()
    drop = client.post("/api/notes", {"text": "drop"}, format="json", **AUTH).json()
    client.patch(f"/api/notes/{drop['id']}", {"done": True}, format="json", **AUTH)

    resp = client.post("/api/notes/clear-done", **AUTH)
    assert resp.status_code == 200
    assert resp.json() == {"deleted": 1}

    listed = client.get("/api/notes", **AUTH).json()
    assert [n["id"] for n in listed["notes"]] == [keep["id"]]
