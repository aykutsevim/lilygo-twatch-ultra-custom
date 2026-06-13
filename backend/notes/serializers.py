"""Validation for note input. The Note object itself is a plain dict (Redis)."""
from rest_framework import serializers

from .store import MAX_TEXT_LEN


class NoteCreateSerializer(serializers.Serializer):
    text = serializers.CharField(
        trim_whitespace=True, allow_blank=False, max_length=MAX_TEXT_LEN
    )


class NoteUpdateSerializer(serializers.Serializer):
    text = serializers.CharField(
        required=False, trim_whitespace=True, allow_blank=False,
        max_length=MAX_TEXT_LEN,
    )
    done = serializers.BooleanField(required=False)

    def validate(self, attrs):
        if "text" not in attrs and "done" not in attrs:
            raise serializers.ValidationError("Provide 'text' and/or 'done'.")
        return attrs
