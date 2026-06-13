"""
NoteStore — the only data-access layer. Notes live in Redis; there is no SQL DB.

Redis key model (see specs/04-backend.md §4.3):
    notes:rev    string(int)  global revision counter, INCR on every mutation
    notes:seq    string(int)  id sequence, INCR -> n_%06d
    notes:data   hash         id -> JSON of the note
    notes:order  zset         id -> score=created_at(ms), gives creation order
"""
from __future__ import annotations

import json
import time

import redis
from django.conf import settings

REV_KEY = "notes:rev"
SEQ_KEY = "notes:seq"
DATA_KEY = "notes:data"
ORDER_KEY = "notes:order"

MAX_TEXT_LEN = 280


def now_ms() -> int:
    return int(time.time() * 1000)


class NotFound(Exception):
    """Raised when a note id does not exist."""


class NoteStore:
    """Thin synchronous repository over redis-py.

    Every mutation bumps notes:rev inside a pipeline so the data write and the
    revision bump commit together. Methods return (rev, payload) so the caller
    can broadcast the matching realtime event.
    """

    def __init__(self, client: "redis.Redis"):
        self.r = client

    # ─── reads ──────────────────────────────────────────────────────────
    def get_rev(self) -> int:
        return int(self.r.get(REV_KEY) or 0)

    def ping(self) -> bool:
        return bool(self.r.ping())

    def list(self) -> tuple[int, list[dict]]:
        ids = self.r.zrange(ORDER_KEY, 0, -1)  # by created_at asc
        notes: list[dict] = []
        if ids:
            raw = self.r.hmget(DATA_KEY, ids)
            notes = [json.loads(item) for item in raw if item]
        return self.get_rev(), notes

    def get(self, note_id: str) -> dict:
        raw = self.r.hget(DATA_KEY, note_id)
        if raw is None:
            raise NotFound(note_id)
        return json.loads(raw)

    # ─── writes ─────────────────────────────────────────────────────────
    def create(self, text: str) -> tuple[int, dict]:
        seq = self.r.incr(SEQ_KEY)
        note_id = f"n_{seq:06d}"
        ts = now_ms()
        note = {
            "id": note_id,
            "text": text,
            "done": False,
            "created_at": ts,
            "updated_at": ts,
        }
        pipe = self.r.pipeline()
        pipe.hset(DATA_KEY, note_id, json.dumps(note))
        pipe.zadd(ORDER_KEY, {note_id: ts})
        pipe.incr(REV_KEY)
        rev = pipe.execute()[-1]
        return int(rev), note

    def update(self, note_id: str, *, text: str | None = None,
               done: bool | None = None) -> tuple[int, dict]:
        note = self.get(note_id)  # raises NotFound
        if text is not None:
            note["text"] = text
        if done is not None:
            note["done"] = bool(done)
        note["updated_at"] = now_ms()
        pipe = self.r.pipeline()
        pipe.hset(DATA_KEY, note_id, json.dumps(note))
        pipe.incr(REV_KEY)
        rev = pipe.execute()[-1]
        return int(rev), note

    def toggle(self, note_id: str) -> tuple[int, dict]:
        note = self.get(note_id)
        return self.update(note_id, done=not note["done"])

    def delete(self, note_id: str) -> int:
        # Confirm existence so callers can 404.
        if not self.r.hexists(DATA_KEY, note_id):
            raise NotFound(note_id)
        pipe = self.r.pipeline()
        pipe.hdel(DATA_KEY, note_id)
        pipe.zrem(ORDER_KEY, note_id)
        pipe.incr(REV_KEY)
        rev = pipe.execute()[-1]
        return int(rev)

    def clear_done(self) -> tuple[int, list[str]]:
        _, notes = self.list()
        done_ids = [n["id"] for n in notes if n.get("done")]
        if not done_ids:
            # Nothing changed; do not bump rev.
            return self.get_rev(), []
        pipe = self.r.pipeline()
        pipe.hdel(DATA_KEY, *done_ids)
        pipe.zrem(ORDER_KEY, *done_ids)
        pipe.incr(REV_KEY)
        rev = pipe.execute()[-1]
        return int(rev), done_ids


_STORE: NoteStore | None = None


def get_store() -> NoteStore:
    """Process-wide NoteStore singleton (lazy)."""
    global _STORE
    if _STORE is None:
        client = redis.from_url(settings.REDIS_URL, decode_responses=True)
        _STORE = NoteStore(client)
    return _STORE
