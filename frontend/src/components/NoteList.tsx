import type { Note } from "../types";
import { NoteItem } from "./NoteItem";

interface Props {
  notes: Note[];
  onToggle: (id: string, done: boolean) => void;
  onDelete: (id: string) => void;
}

export function NoteList({ notes, onToggle, onDelete }: Props) {
  if (notes.length === 0) {
    return <p className="empty">No todos — add one above.</p>;
  }
  return (
    <ul className="list">
      {notes.map((n) => (
        <NoteItem key={n.id} note={n} onToggle={onToggle} onDelete={onDelete} />
      ))}
    </ul>
  );
}
