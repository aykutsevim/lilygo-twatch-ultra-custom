"""
Consumer tests using Channels' WebsocketCommunicator + in-memory channel layer.

The consumer reads the store via get_store(); the fake_store fixture is process
-wide so the communicator and any broadcast share it.
"""
import pytest
from channels.testing import WebsocketCommunicator

from config.asgi import application


def ws(key="test-key"):
    return WebsocketCommunicator(application, f"/ws/notes/?key={key}")


@pytest.mark.asyncio
async def test_rejects_bad_key():
    comm = WebsocketCommunicator(application, "/ws/notes/?key=wrong")
    connected, _ = await comm.connect()
    assert connected is False


@pytest.mark.asyncio
async def test_connect_sends_hello(fake_store):
    fake_store.create("seed")  # rev -> 1
    comm = ws()
    connected, _ = await comm.connect()
    assert connected is True
    hello = await comm.receive_json_from()
    assert hello == {"type": "hello", "rev": 1}
    await comm.disconnect()


@pytest.mark.asyncio
async def test_toggle_broadcasts_update(fake_store):
    _, note = fake_store.create("a")
    comm = ws()
    await comm.connect()
    await comm.receive_json_from()  # hello

    await comm.send_json_to({"type": "toggle", "id": note["id"]})
    event = await comm.receive_json_from()
    assert event["type"] == "note.updated"
    assert event["note"]["id"] == note["id"]
    assert event["note"]["done"] is True
    await comm.disconnect()
