from django.urls import path

from .views import ClearDoneView, HealthView, NoteDetailView, NoteListView

urlpatterns = [
    path("health", HealthView.as_view()),
    path("notes", NoteListView.as_view()),
    path("notes/clear-done", ClearDoneView.as_view()),
    path("notes/<str:note_id>", NoteDetailView.as_view()),
]
