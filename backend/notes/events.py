"""
Realtime broadcast helpers. Every mutation fans out to the `notes` Channels
group so all connected clients (web + watch) update. See specs/07-realtime.md.
"""
from asgiref.sync import async_to_sync
from channels.layers import get_channel_layer

GROUP = "notes"


def make_created(rev: int, note: dict) -> dict:
    return {"type": "note.created", "rev": rev, "note": note}


def make_updated(rev: int, note: dict) -> dict:
    return {"type": "note.updated", "rev": rev, "note": note}


def make_deleted(rev: int, note_id: str) -> dict:
    return {"type": "note.deleted", "rev": rev, "id": note_id}


def make_cleared(rev: int, ids: list[str]) -> dict:
    return {"type": "notes.cleared", "rev": rev, "ids": ids}


async def abroadcast(event: dict) -> None:
    layer = get_channel_layer()
    if layer is None:
        return
    # Consumer handler is `note_event`; payload is the wire frame.
    await layer.group_send(GROUP, {"type": "note.event", "payload": event})


def broadcast(event: dict) -> None:
    """Sync entrypoint for DRF views."""
    async_to_sync(abroadcast)(event)
