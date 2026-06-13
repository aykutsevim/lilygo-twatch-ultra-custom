import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// Dev server on :5173. The app talks to the backend via VITE_API_BASE /
// VITE_WS_BASE (absolute URLs); the backend's CORS already allows :5173.
// A proxy is also provided so you can instead point the app at same-origin
// "/api" + "/ws" by leaving VITE_API_BASE empty.
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      "/api": { target: "http://localhost:8000", changeOrigin: true },
      "/ws": { target: "ws://localhost:8000", ws: true },
    },
  },
});
