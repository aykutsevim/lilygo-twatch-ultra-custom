import { useCallback, useEffect, useReducer, useRef, useState } from "react";

import { api } from "./api";
import { NotesSocket } from "./ws";
import { initialState, reducer } from "./store";
import type { ConnStatus } from "./types";

/**
 * Owns the notes list: seeds from REST, applies live WS events with `rev`
 * reconciliation (dedupe echoes, full resync on gap), and exposes mutations.
 * Mutations are REST calls; the UI updates when the resulting WS broadcast
 * arrives (see specs/05-frontend.md §5.5).
 */
export function useNotes() {
  const [state, dispatch] = useReducer(reducer, initialState);
  const [status, setStatus] = useState<ConnStatus>("connecting");
  const revRef = useRef(0);

  const resync = useCallback(async () => {
    const snap = await api.getNotes();
    revRef.current = snap.rev;
    dispatch({ type: "snapshot", rev: snap.rev, notes: snap.notes });
  }, []);

  useEffect(() => {
    resync().catch(() => {});

    const sock = new NotesSocket((e) => {
      if (e.type === "hello") {
        if (e.rev > revRef.current) resync().catch(() => {});
        return;
      }
      if (e.type === "ping") return; // handled in the socket; ignore here
      const evRev = e.rev;
      if (evRev <= revRef.current) return; // echo / duplicate
      if (evRev > revRef.current + 1) {
        resync().catch(() => {}); // gap -> full resync
        return;
      }
      revRef.current = evRev;
      dispatch({ type: "apply", event: e });
    }, setStatus);

    sock.connect();

    const onVisible = () => {
      if (document.visibilityState === "visible") resync().catch(() => {});
    };
    document.addEventListener("visibilitychange", onVisible);

    return () => {
      document.removeEventListener("visibilitychange", onVisible);
      sock.close();
    };
  }, [resync]);

  const add = useCallback((text: string) => api.createNote(text), []);
  const toggle = useCallback(
    (id: string, done: boolean) => api.updateNote(id, { done }),
    [],
  );
  const remove = useCallback((id: string) => api.deleteNote(id), []);
  const clearDone = useCallback(() => api.clearDone(), []);

  return { notes: state.notes, status, add, toggle, remove, clearDone };
}
