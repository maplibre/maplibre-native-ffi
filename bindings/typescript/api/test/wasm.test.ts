/**
 * The conformance suite over the WebAssembly transport.
 *
 * The same public layer runs here and over the Node-API addon; only what a
 * pointer is differs. Running the same cases against both is what says the
 * shared layer is actually shared rather than merely written once.
 *
 * The module is a portable JavaScript-host build, so Node instantiates the same
 * artifact a browser would.
 */

import {
  CONFORMANCE,
  type Expect,
  Maplibre,
  MaplibreError,
} from "../src/index.ts";
import {
  type WasmModule,
  wasmTransport,
} from "../src/internal/wasm-transport.ts";
import { existsSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";

const modulePath = fileURLToPath(
  new URL("../../runtime-wasm/maplibre-native-ffi.mjs", import.meta.url),
);

// The payload is built by `mise run //bindings/typescript:build-wasm`, which the
// test task runs first. A missing payload is a build that did not happen, so it
// fails rather than skipping.
if (!existsSync(modulePath)) {
  throw new Error(
    `no WebAssembly payload at ${modulePath}; run //bindings/typescript:build-wasm`,
  );
}

const createRuntime = (await import(modulePath))
  .default as () => Promise<WasmModule>;
const maplibre = Maplibre.fromTransport(wasmTransport(await createRuntime()));

const assertions: Expect = {
  equal(actual, expected, what) {
    expect(actual, what).toEqual(expected);
  },
  ok(actual, what) {
    expect(actual, what).toBe(true);
  },
  throws(body, what) {
    let thrown: unknown;
    try {
      body();
    } catch (error) {
      thrown = error;
    }
    expect(thrown, what).toBeInstanceOf(MaplibreError);
    return thrown as MaplibreError;
  },
};

describe("the WebAssembly transport", () => {
  it("agrees with the Node-API payload about the ABI", () => {
    // One generation produced both, so a difference here means one of them was
    // built from other headers.
    expect(maplibre.cVersion).toBe(0);
  });

  for (const group of CONFORMANCE) {
    describe(group.name, () => {
      for (const entry of group.cases) {
        it(entry.name, async () => {
          await entry.run({ maplibre, expect: assertions });
        });
      }
    });
  }
});
