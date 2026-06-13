import { API_KEY, apiUrl } from "./config";
import type { Note, Snapshot } from "./types";

async function req<T>(path: string, init: RequestInit = {}): Promise<T> {
  const res = await fetch(apiUrl(path), {
    ...init,
    headers: {
      "Content-Type": "application/json",
      "X-API-Key": API_KEY,
      ...(init.headers ?? {}),
    },
  });
  if (!res.ok) {
    throw new Error(`${init.method ?? "GET"} ${path} -> ${res.status}`);
  }
  if (res.status === 204) return undefined as T;
  return (await res.json()) as T;
}

export const api = {
  getNotes: () => req<Snapshot>("/api/notes"),
  createNote: (text: string) =>
    req<Note>("/api/notes", { method: "POST", body: JSON.stringify({ text }) }),
  updateNote: (id: string, patch: Partial<Pick<Note, "text" | "done">>) =>
    req<Note>(`/api/notes/${id}`, {
      method: "PATCH",
      body: JSON.stringify(patch),
    }),
  deleteNote: (id: string) =>
    req<void>(`/api/notes/${id}`, { method: "DELETE" }),
  clearDone: () =>
    req<{ deleted: number }>("/api/notes/clear-done", { method: "POST" }),
};
