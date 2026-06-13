// Centralizes build-time config. Empty bases => same-origin (Vite dev proxy).

const API_BASE = (import.meta.env.VITE_API_BASE ?? "").replace(/\/$/, "");
const WS_BASE = (import.meta.env.VITE_WS_BASE ?? "").replace(/\/$/, "");

export const API_KEY = import.meta.env.VITE_API_KEY ?? "";

export function apiUrl(path: string): string {
  return `${API_BASE}${path}`;
}

export function wsUrl(path: string): string {
  if (WS_BASE) return `${WS_BASE}${path}`;
  // Same-origin fallback (dev proxy): derive ws(s) from page origin.
  const proto = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${proto}//${window.location.host}${path}`;
}
