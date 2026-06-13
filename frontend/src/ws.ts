import { API_KEY, wsUrl } from "./config";
import type { ConnStatus, ServerEvent } from "./types";

/**
 * WebSocket client for /ws/notes/. Auto-reconnects with exponential backoff +
 * jitter and reports status. Frame parsing/`rev` reconciliation is left to the
 * caller (see useNotes). See specs/07-realtime.md.
 */
export class NotesSocket {
  private ws: WebSocket | null = null;
  private backoff = 500; // ms
  private readonly maxBackoff = 10_000;
  private closedByUser = false;
  private reconnectTimer: number | null = null;

  constructor(
    private readonly onEvent: (e: ServerEvent) => void,
    private readonly onStatus: (s: ConnStatus) => void,
  ) {}

  connect(): void {
    this.closedByUser = false;
    this.onStatus(this.backoff === 500 ? "connecting" : "reconnecting");
    const ws = new WebSocket(wsUrl(`/ws/notes/?key=${encodeURIComponent(API_KEY)}`));
    this.ws = ws;

    ws.onopen = () => {
      this.backoff = 500;
      this.onStatus("online");
    };

    ws.onmessage = (ev) => {
      let frame: ServerEvent;
      try {
        frame = JSON.parse(ev.data) as ServerEvent;
      } catch {
        return;
      }
      if (frame.type === "ping") {
        ws.send(JSON.stringify({ type: "pong" }));
        return;
      }
      this.onEvent(frame);
    };

    ws.onclose = () => {
      this.ws = null;
      if (this.closedByUser) return;
      this.onStatus("reconnecting");
      this.scheduleReconnect();
    };

    // onerror is followed by onclose; let onclose drive reconnection.
    ws.onerror = () => ws.close();
  }

  // Optional client->server light actions (used by the watch; web uses REST).
  send(msg: Record<string, unknown>): void {
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(msg));
    }
  }

  close(): void {
    this.closedByUser = true;
    if (this.reconnectTimer) window.clearTimeout(this.reconnectTimer);
    this.ws?.close();
    this.onStatus("offline");
  }

  private scheduleReconnect(): void {
    const jitter = Math.random() * 0.3 * this.backoff;
    const delay = Math.min(this.backoff, this.maxBackoff) + jitter;
    this.reconnectTimer = window.setTimeout(() => this.connect(), delay);
    this.backoff = Math.min(this.backoff * 2, this.maxBackoff);
  }
}
