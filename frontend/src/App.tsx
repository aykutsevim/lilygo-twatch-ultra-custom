import { AddNote } from "./components/AddNote";
import { NoteList } from "./components/NoteList";
import { StatusBar } from "./components/StatusBar";
import { useNotes } from "./useNotes";

export default function App() {
  const { notes, status, add, toggle, remove, clearDone } = useNotes();
  const doneCount = notes.filter((n) => n.done).length;
  const remaining = notes.length - doneCount;

  return (
    <main className="app">
      <h1>WatchTodo</h1>
      <AddNote onAdd={add} />
      <NoteList notes={notes} onToggle={toggle} onDelete={remove} />
      <StatusBar
        status={status}
        remaining={remaining}
        doneCount={doneCount}
        onClearDone={clearDone}
      />
    </main>
  );
}
