/**
 * The published shapes, built from one TypeScript source.
 *
 * A consumer reaches this package through an ESM import or a CommonJS require,
 * and both have to give the same API over the same runtime payload.
 */

import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";

const require = createRequire(import.meta.url);
const distribution = fileURLToPath(new URL("../dist/", import.meta.url));

describe("the built package", () => {
  it("loads through CommonJS and drives the native library", async () => {
    const api = require(
      `${distribution}index.cjs`,
    ) as typeof import("../src/index.ts");
    const maplibre = await api.Maplibre.load();
    const runtime = maplibre.createRuntime();
    try {
      const map = runtime.createMap({ width: 64, height: 48 });
      expect(map.getSize().width).toBe(64);
      map.close();
    } finally {
      runtime.close();
    }
  });

  it("loads through ESM and reports the same names", async () => {
    const api = (await import(
      `${distribution}index.mjs`
    )) as typeof import("../src/index.ts");
    const maplibre = await api.Maplibre.load();
    const runtime = maplibre.createRuntime();
    try {
      const map = runtime.createMap({ width: 64, height: 48 });
      expect(map.getSize().height).toBe(48);
      // The two builds are one implementation, so an error from either is the
      // same class to a consumer that catches it.
      expect(api.MaplibreError.name).toBe("MaplibreError");
      map.close();
    } finally {
      runtime.close();
    }
  });
});
