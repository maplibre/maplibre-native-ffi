/**
 * The conformance suite in a browser, over the WebAssembly transport.
 *
 * Node instantiates the same payload, so this is not a second transport. What
 * it adds is the host the payload was built for: the module is threaded and so
 * needs a cross-origin isolated page, its memory grows under a real worker, and
 * a case that reaches the filesystem finds only the one the module carries. A
 * case that passes under Node and fails here is a difference between hosts
 * rather than between suites.
 */

import {
  CONFORMANCE,
  type Expect,
  groupsFor,
  Maplibre,
  MaplibreError,
} from "../src/index.ts";
import {
  type WasmModule,
  wasmTransport,
} from "../src/internal/wasm-transport.ts";
import { beforeAll, describe, expect, it } from "vitest";

// Served rather than imported from disk: a page fetches the payload, and
// Emscripten finds the `.wasm` beside the `.mjs` through that URL. The URL is
// built here rather than written as a literal, because the dev server resolves
// a literal at transform time and refuses to hand over a script it only serves.
// The payload is instantiated once the run has started rather than while this
// file is being collected: a browser collects every file before running any of
// them, so awaiting a multi-megabyte module up there stalls the whole run.
let module_: WasmModule;
let maplibre: Maplibre;

beforeAll(async () => {
  const payload = new URL("/maplibre-native-ffi.mjs", import.meta.url).href;
  const createRuntime = (await import(/* @vite-ignore */ payload))
    .default as () => Promise<WasmModule>;
  module_ = await createRuntime();
  maplibre = Maplibre.fromTransport(wasmTransport(module_));
}, 120_000);

/**
 * What this runner offers.
 *
 * A page has a real WebGL context, which is what a render session attaches to,
 * and no CommonJS loader at all, which is why it resolves no package.
 */
const CAPABILITIES = ["renderContext"] as const;

/**
 * A WebGL context the module owns, made current before it is handed over.
 *
 * Emscripten numbers its contexts rather than addressing them, so this crosses
 * as a handle rather than a pointer. One context serves every case: creating
 * one per case would exhaust what a browser will give a page.
 */
let context: { platform: "webgl"; context: number } | undefined;
function renderContext(): { platform: "webgl"; context: number } {
  if (context === undefined) {
    const gl = (
      module_ as unknown as {
        GL: {
          createContext(canvas: HTMLCanvasElement, attributes: object): number;
          makeContextCurrent(handle: number): boolean;
        };
      }
    ).GL;
    const canvas = document.createElement("canvas");
    canvas.width = 256;
    canvas.height = 256;
    const handle = gl.createContext(canvas, {
      majorVersion: 2,
      minorVersion: 0,
      alpha: true,
      depth: true,
      stencil: true,
      antialias: false,
    });
    if (handle === 0 || !gl.makeContextCurrent(handle)) {
      throw new Error("this browser gave the module no WebGL 2 context");
    }
    context = { platform: "webgl", context: handle };
  }
  return context;
}

/**
 * A texture this page owns, which a caller-owned target draws into.
 *
 * Made through the same context the session attaches to, so the session can
 * see it, and kept by this runner so a case can check the session left it
 * alone.
 */
function hostTexture(
  width: number,
  height: number,
): { texture: number; target: number } {
  const handle = renderContext().context;
  const gl = (
    module_ as unknown as {
      GL: {
        contexts: { GLctx: WebGL2RenderingContext }[];
        getNewId(table: unknown[]): number;
        textures: unknown[];
      };
    }
  ).GL;
  const context = gl.contexts[handle]!.GLctx;
  const texture = context.createTexture();
  context.bindTexture(context.TEXTURE_2D, texture);
  context.texImage2D(
    context.TEXTURE_2D,
    0,
    context.RGBA8,
    width,
    height,
    0,
    context.RGBA,
    context.UNSIGNED_BYTE,
    null,
  );
  // Emscripten addresses a texture by the id it assigned, not by the JS object,
  // so the id is what crosses to the C API.
  const id = gl.getNewId(gl.textures);
  gl.textures[id] = texture;
  return { texture: id, target: context.TEXTURE_2D };
}

/**
 * A page cannot listen on a port.
 *
 * The cases that fetch from an origin of their own declare the capability, and
 * this runner leaves it out, so reaching this is a registration mistake rather
 * than something to work around.
 */
function httpOrigin(): never {
  throw new Error("a page cannot listen for HTTP requests");
}

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

/**
 * Nothing here loads the published package.
 *
 * A browser resolves ES modules and has no CommonJS loader at all, so the case
 * that drives both module formats declares the capability it needs and is left
 * out below rather than half-run here.
 */
function loadPackage(): Promise<typeof import("../src/index.ts")> {
  throw new Error("a browser resolves no package by name");
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

describe("a browser hosting the WebAssembly payload", () => {
  it("is cross-origin isolated, so the module's memory can be shared", () => {
    // The threaded payload cannot start without this, and a page that lost the
    // headers would otherwise fail somewhere deeper with a stranger message.
    expect(globalThis.crossOriginIsolated).toBe(true);
  });

  /** What this runner loads, so a case restricted to another one is skipped. */
  const TRANSPORT = "wasm";

  for (const group of groupsFor(CONFORMANCE, {
    transport: TRANSPORT,
    capabilities: CAPABILITIES,
  })) {
    describe(group.name, () => {
      for (const entry of group.cases) {
        it(entry.name, async () => {
          await entry.run({
            maplibre,
            expect: assertions,
            cacheDirectory,
            loadPackage,
            renderContext,
            hostTexture,
            httpOrigin,
          });
        });
      }
    });
  }
});
