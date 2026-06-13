import type { Note } from "../types";

interface Props {
  note: Note;
  onToggle: (id: string, done: boolean) => void;
  onDelete: (id: string) => void;
}

export function NoteItem({ note, onToggle, onDelete }: Props) {
  return (
    <li className={`item${note.done ? " done" : ""}`}>
      <label>
        <input
          type="checkbox"
          checked={note.done}
          onChange={(e) => onToggle(note.id, e.target.checked)}
        />
        <span className="text">{note.text}</span>
      </label>
      <button
        className="del"
        aria-label="Delete"
        onClick={() => onDelete(note.id)}
      >
        ×
      </button>
    </li>
  );
}
