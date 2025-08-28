import tailwindcss from "@tailwindcss/vite";
import { defineConfig } from "vite";
import solidPlugin from "vite-plugin-solid";

export default defineConfig({
  plugins: [solidPlugin(), tailwindcss()],
  build: {
    outDir: "../resources/web",
    emptyOutDir: true,
  },
  server: {
    port: 5173,
  },
  build: {
    target: "esnext",
  },
});
