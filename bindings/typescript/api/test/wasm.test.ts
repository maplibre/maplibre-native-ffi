/**
 * The conformance suite over the WebAssembly transport.
 *
 * The same public layer runs here and over the Node-API addon; only what a
 * pointer is differs. Running the same cases against both is what says the
 * shared layer is actually shared rather than merely written once.
 *
 * Node instantiates the same artifact a browser would. The cases avoid the
 * module's default HTTP path, which is Emscripten Fetch and so needs XHR: a
 * non-browser host serves resources through a provider instead.
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
const module_ = await createRuntime();
const maplibre = Maplibre.fromTransport(wasmTransport(module_));

/**
 * Offline work needs a database.
 *
 * The module has its own in-memory filesystem and cannot see the host's, so the
 * directory is made inside the module rather than beside the test.
 */
let cacheSequence = 0;
function cacheDirectory(): Promise<string> {
  const filesystem = (module_ as { FS?: { mkdir(path: string): void } }).FS;
  if (filesystem === undefined) {
    throw new Error("the WebAssembly module exposes no filesystem");
  }
  cacheSequence += 1;
  const path = `/maplibre-conformance-${cacheSequence}`;
  filesystem.mkdir(path);
  return Promise.resolve(path);
}

const assertions: Expect = {
  equal(actual, expected, what) {
    expect(actual, what).toEqual(expected);
  },
  notEqual(actual, unexpected, what) {
    expect(actual, what).not.toEqual(unexpected);
  },
  ok(actual, what) {
    expect(actual, what).toBe(true);
  },
  closeTo(actual, expected, digits, what) {
    expect(actual, what).toBeCloseTo(expected, digits);
  },
  defined(actual, what) {
    expect(actual, what).not.toBeUndefined();
    expect(actual, what).not.toBeNull();
    return actual as NonNullable<typeof actual>;
  },
  absent(actual, what) {
    expect(actual, what).toBeUndefined();
  },
  contains(haystack, needle, what) {
    expect(haystack, what).toContain(needle);
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
  throwsAny(body, what) {
    let thrown: unknown;
    try {
      body();
    } catch (error) {
      thrown = error;
    }
    expect(thrown, what).toBeInstanceOf(Error);
    return thrown as Error;
  },
  fail(what) {
    expect.unreachable(what);
  },
};

describe("the WebAssembly transport", () => {
  it("agrees with the Node-API payload about the ABI", () => {
    // One generation produced both, so a difference here means one of them was
    // built from other headers.
    expect(maplibre.cVersion).toBe(0);
  });

  /** What this runner loads, so a case restricted to another one is skipped. */
  const TRANSPORT = "wasm";

  for (const group of CONFORMANCE) {
    describe(group.name, () => {
      const cases = group.cases.filter(
        (entry) =>
          entry.transports === undefined ||
          entry.transports.includes(TRANSPORT),
      );
      for (const entry of cases) {
        it(entry.name, async () => {
          await entry.run({ maplibre, expect: assertions, cacheDirectory });
        });
      }
    });
  }
});

describe("WebAssembly memory bounds", () => {
  it("refuses a read that leaves the module's memory", () => {
    const transport = wasmTransport(module_);
    // `slice` would clamp rather than reject, so a stale pointer would become a
    // short copy that fails somewhere else with a message naming nothing.
    expect(() => transport.readForeign(0xffff_fff0n as never, 64)).toThrow(
      MaplibreError,
    );
    expect(() => transport.readForeign(0n as never, 1)).toThrow(MaplibreError);
    expect(() => transport.readForeignCString(0xffff_fff0n as never)).toThrow(
      MaplibreError,
    );
    // A wasm32 module has no address this large at all.
    expect(() => transport.readForeign((1n << 40n) as never, 1)).toThrow(
      MaplibreError,
    );
    // The null pointer is the one address that reads as absent rather than
    // failing, because the C API uses it for an absent string.
    expect(transport.readForeignCString(0n as never)).toBeNull();
  });
});
