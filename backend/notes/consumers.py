"""
NotesConsumer — WebSocket endpoint /ws/notes/?key=<key>.

Server -> client: hello, note.*, notes.cleared, ping (see specs/03-api-contract.md).
Client -> server: toggle, delete, pong (light watch actions).

The source of truth is Redis; events are an optimization. Clients reconcile via
the `rev` counter and a full GET /api/notes on reconnect.
"""
import asyncio
from urllib.parse import parse_qs

from asgiref.sync import sync_to_async
from channels.generic.websocket import AsyncJsonWebsocketConsumer
from django.conf import settings

from . import events
from .auth import check_key
from .store import NotFound, get_store

GROUP = events.GROUP
WS_AUTH_CLOSE_CODE = 4401


class NotesConsumer(AsyncJsonWebsocketConsumer):
    async def connect(self):
        query = parse_qs(self.scope.get("query_string", b"").decode())
        key = (query.get("key") or [None])[0]
        if not check_key(key):
            await self.close(code=WS_AUTH_CLOSE_CODE)
            return

        await self.channel_layer.group_add(GROUP, self.channel_name)
        await self.accept()

        rev = await sync_to_async(get_store().get_rev)()
        await self.send_json({"type": "hello", "rev": rev})

        self._ping_task = asyncio.create_task(self._ping_loop())

    async def disconnect(self, code):
        task = getattr(self, "_ping_task", None)
        if task:
            task.cancel()
        await self.channel_layer.group_discard(GROUP, self.channel_name)

    async def receive_json(self, content, **kwargs):
        msg_type = content.get("type")
        note_id = content.get("id")
        store = get_store()

        if msg_type == "toggle" and note_id:
            try:
                rev, note = await sync_to_async(store.toggle)(note_id)
            except NotFound:
                return
            await events.abroadcast(events.make_updated(rev, note))
        elif msg_type == "delete" and note_id:
            try:
                rev = await sync_to_async(store.delete)(note_id)
            except NotFound:
                return
            await events.abroadcast(events.make_deleted(rev, note_id))
        elif msg_type == "pong":
            return
        # Unknown types are ignored.

    async def note_event(self, message):
        """Group handler: forward the broadcast frame to this socket."""
        await self.send_json(message["payload"])

    async def _ping_loop(self):
        interval = settings.WS_PING_INTERVAL
        try:
            while True:
                await asyncio.sleep(interval)
                await self.send_json({"type": "ping"})
        except asyncio.CancelledError:
            pass
