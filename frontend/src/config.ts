// Centralizes config. Priority: runtime config injected by the backend
// (window.__WT_CONFIG__) > build-time VITE_* (vite dev). Empty bases => the app
// talks to its own origin (single-image deploy / Vite dev proxy).

type RuntimeConfig = { apiBase?: string; wsBase?: string; apiKey?: string };
const rc: RuntimeConfig = (window as unknown as { __WT_CONFIG__?: RuntimeConfig })
  .__WT_CONFIG__ ?? {};

// Ignore the un-replaced placeholder when running `vite dev`.
const injectedKey =
  rc.apiKey && !rc.apiKey.startsWith("__") ? rc.apiKey : undefined;

const API_BASE = (rc.apiBase ?? import.meta.env.VITE_API_BASE ?? "").replace(/\/$/, "");
const WS_BASE = (rc.wsBase ?? import.meta.env.VITE_WS_BASE ?? "").replace(/\/$/, "");

export const API_KEY = injectedKey ?? import.meta.env.VITE_API_KEY ?? "";

export function apiUrl(path: string): string {
  return `${API_BASE}${path}`;
}

export function wsUrl(path: string): string {
  if (WS_BASE) return `${WS_BASE}${path}`;
  // Same-origin: derive ws(s) from the page origin.
  const proto = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${proto}//${window.location.host}${path}`;
}
