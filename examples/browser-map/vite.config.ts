import fs from "node:fs";
import path from "node:path";
import process from "node:process";
import { defineConfig } from "vite-plus";

const generatedAssetNames = new Set(["/browser-map.js", "/browser-map.wasm"]);

export default defineConfig({
  plugins: [
    {
      name: "browser-map-generated-assets",
      configureServer(server) {
        server.middlewares.use((request, response, next) => {
          const url = new URL(request.url ?? "/", "http://localhost");
          if (!generatedAssetNames.has(url.pathname)) {
            next();
            return;
          }
          const buildDir = process.env.MLN_FFI_BUILD_DIR;
          if (!buildDir) {
            response.statusCode = 500;
            response.end("MLN_FFI_BUILD_DIR is not set");
            return;
          }
          const assetPath = path.join(
            buildDir,
            "examples",
            "browser-map",
            path.basename(url.pathname),
          );
          if (!fs.existsSync(assetPath)) {
            response.statusCode = 404;
            response.end(`${url.pathname} has not been built`);
            return;
          }
          if (url.pathname.endsWith(".wasm")) {
            response.setHeader("Content-Type", "application/wasm");
          } else {
            response.setHeader("Content-Type", "text/javascript");
          }
          response.setHeader("Cache-Control", "no-store");
          response.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
          response.setHeader("Cross-Origin-Opener-Policy", "same-origin");
          fs.createReadStream(assetPath).pipe(response);
        });
      },
    },
  ],
  server: {
    headers: {
      "Cache-Control": "no-store",
      "Cross-Origin-Embedder-Policy": "require-corp",
      "Cross-Origin-Opener-Policy": "same-origin",
    },
  },
});
