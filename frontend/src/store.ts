import type { Note, ServerEvent } from "./types";

export interface State {
  rev: number;
  notes: Note[];
}

export const initialState: State = { rev: 0, notes: [] };

export type Action =
  | { type: "snapshot"; rev: number; notes: Note[] }
  | { type: "apply"; event: ServerEvent };

function byCreated(a: Note, b: Note): number {
  return a.created_at - b.created_at;
}

function upsert(notes: Note[], note: Note): Note[] {
  const next = notes.some((n) => n.id === note.id)
    ? notes.map((n) => (n.id === note.id ? note : n))
    : [...notes, note];
  return next.sort(byCreated);
}

export function reducer(state: State, action: Action): State {
  switch (action.type) {
    case "snapshot":
      return { rev: action.rev, notes: [...action.notes].sort(byCreated) };

    case "apply": {
      const e = action.event;
      if (!("rev" in e) || e.rev <= state.rev) return state; // echo/dedupe
      switch (e.type) {
        case "note.created":
        case "note.updated":
          return { rev: e.rev, notes: upsert(state.notes, e.note) };
        case "note.deleted":
          return { rev: e.rev, notes: state.notes.filter((n) => n.id !== e.id) };
        case "notes.cleared":
          return {
            rev: e.rev,
            notes: state.notes.filter((n) => !e.ids.includes(n.id)),
          };
        default:
          return state;
      }
    }

    default:
      return state;
  }
}
