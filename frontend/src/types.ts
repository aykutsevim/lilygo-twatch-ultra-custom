// Mirrors specs/03-api-contract.md.

export interface Note {
  id: string;
  text: string;
  done: boolean;
  created_at: number; // epoch ms
  updated_at: number; // epoch ms
}

export interface Snapshot {
  rev: number;
  notes: Note[];
}

// Server -> client WebSocket frames.
export type ServerEvent =
  | { type: "hello"; rev: number }
  | { type: "ping"; ts?: number }
  | { type: "note.created"; rev: number; note: Note }
  | { type: "note.updated"; rev: number; note: Note }
  | { type: "note.deleted"; rev: number; id: string }
  | { type: "notes.cleared"; rev: number; ids: string[] };

export type ConnStatus = "connecting" | "online" | "reconnecting" | "offline";
