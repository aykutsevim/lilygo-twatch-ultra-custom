from django.urls import path

from .consumers import NotesConsumer

websocket_urlpatterns = [
    path("ws/notes/", NotesConsumer.as_asgi()),
]
