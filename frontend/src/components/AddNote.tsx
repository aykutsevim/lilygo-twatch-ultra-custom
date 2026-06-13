import { useState } from "react";

export function AddNote({ onAdd }: { onAdd: (text: string) => Promise<unknown> }) {
  const [text, setText] = useState("");
  const [busy, setBusy] = useState(false);

  async function submit(e: React.FormEvent) {
    e.preventDefault();
    const value = text.trim();
    if (!value || busy) return;
    setBusy(true);
    try {
      await onAdd(value);
      setText("");
    } catch {
      // Surface failure minimally; the input keeps its text for retry.
    } finally {
      setBusy(false);
    }
  }

  return (
    <form className="add" onSubmit={submit}>
      <input
        autoFocus
        value={text}
        maxLength={280}
        placeholder="Add a todo…"
        onChange={(e) => setText(e.target.value)}
        onKeyDown={(e) => {
          if (e.key === "Escape") setText("");
        }}
      />
      <button type="submit" disabled={busy || !text.trim()}>
        Add
      </button>
    </form>
  );
}
