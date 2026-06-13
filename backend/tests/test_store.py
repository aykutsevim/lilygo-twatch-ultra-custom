"""Unit tests for NoteStore against fakeredis."""
import pytest

from notes.store import NotFound


def test_create_list_order_and_rev(fake_store):
    rev0, notes0 = fake_store.list()
    assert rev0 == 0 and notes0 == []

    rev1, a = fake_store.create("first")
    rev2, b = fake_store.create("second")

    assert rev1 == 1 and rev2 == 2
    assert a["id"] != b["id"]
    assert a["done"] is False
    assert a["created_at"] <= b["created_at"]

    rev, notes = fake_store.list()
    assert rev == 2
    assert [n["text"] for n in notes] == ["first", "second"]  # creation order


def test_update_text_and_done_bumps_rev(fake_store):
    _, a = fake_store.create("x")
    rev, updated = fake_store.update(a["id"], text="y", done=True)
    assert rev == 2
    assert updated["text"] == "y"
    assert updated["done"] is True
    assert updated["updated_at"] >= a["updated_at"]


def test_toggle(fake_store):
    _, a = fake_store.create("x")
    _, t1 = fake_store.toggle(a["id"])
    assert t1["done"] is True
    _, t2 = fake_store.toggle(a["id"])
    assert t2["done"] is False


def test_delete(fake_store):
    _, a = fake_store.create("x")
    rev = fake_store.delete(a["id"])
    assert rev == 2
    _, notes = fake_store.list()
    assert notes == []
    with pytest.raises(NotFound):
        fake_store.delete(a["id"])


def test_update_missing_raises(fake_store):
    with pytest.raises(NotFound):
        fake_store.update("n_999999", text="z")


def test_clear_done(fake_store):
    _, a = fake_store.create("keep")
    _, b = fake_store.create("drop")
    fake_store.update(b["id"], done=True)
    rev_before = fake_store.get_rev()

    rev, ids = fake_store.clear_done()
    assert ids == [b["id"]]
    assert rev == rev_before + 1
    _, notes = fake_store.list()
    assert [n["text"] for n in notes] == ["keep"]


def test_clear_done_noop_does_not_bump_rev(fake_store):
    fake_store.create("keep")
    rev_before = fake_store.get_rev()
    rev, ids = fake_store.clear_done()
    assert ids == []
    assert rev == rev_before
