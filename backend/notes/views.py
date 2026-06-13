"""DRF views implementing the REST contract (specs/03-api-contract.md §3.3)."""
from rest_framework import status
from rest_framework.permissions import AllowAny
from rest_framework.response import Response
from rest_framework.views import APIView

from . import events
from .serializers import NoteCreateSerializer, NoteUpdateSerializer
from .store import NotFound, get_store


class HealthView(APIView):
    public = True
    authentication_classes = []
    permission_classes = [AllowAny]

    def get(self, request):
        try:
            get_store().ping()
        except Exception:
            return Response(
                {"status": "redis_unavailable"},
                status=status.HTTP_503_SERVICE_UNAVAILABLE,
            )
        return Response({"status": "ok"})


class NoteListView(APIView):
    def get(self, request):
        rev, notes = get_store().list()
        return Response({"rev": rev, "notes": notes})

    def post(self, request):
        serializer = NoteCreateSerializer(data=request.data)
        if not serializer.is_valid():
            return Response(
                {"error": "validation failed", "field": "text"},
                status=status.HTTP_400_BAD_REQUEST,
            )
        rev, note = get_store().create(serializer.validated_data["text"])
        events.broadcast(events.make_created(rev, note))
        return Response(note, status=status.HTTP_201_CREATED)


class NoteDetailView(APIView):
    def patch(self, request, note_id: str):
        serializer = NoteUpdateSerializer(data=request.data)
        if not serializer.is_valid():
            return Response(
                {"error": "validation failed"},
                status=status.HTTP_400_BAD_REQUEST,
            )
        try:
            rev, note = get_store().update(note_id, **serializer.validated_data)
        except NotFound:
            return Response({"error": "not found"}, status=status.HTTP_404_NOT_FOUND)
        events.broadcast(events.make_updated(rev, note))
        return Response(note)

    def delete(self, request, note_id: str):
        try:
            rev = get_store().delete(note_id)
        except NotFound:
            return Response({"error": "not found"}, status=status.HTTP_404_NOT_FOUND)
        events.broadcast(events.make_deleted(rev, note_id))
        return Response(status=status.HTTP_204_NO_CONTENT)


class ClearDoneView(APIView):
    def post(self, request):
        rev, ids = get_store().clear_done()
        if ids:
            events.broadcast(events.make_cleared(rev, ids))
        return Response({"deleted": len(ids)})
