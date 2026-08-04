/**
 * Loading the WebAssembly payload, which is what a browser runs.
 *
 * `Maplibre.load()` discovers a Node-API payload, and a browser resolves no
 * such thing, so a page loads the library through here instead. Everything
 * past the load is the same public API every other host gets. This is a
 * separate entry point so a Node application never pulls WebAssembly loading
 * in, and a page never pulls package discovery in.
 *
 * ## The page has to be cross-origin isolated
 *
 * The payload is threaded, and a browser withholds `SharedArrayBuffer` from a
 * page that is not cross-origin isolated, so the module cannot start its
 * threads at all without these response headers on the document:
 *
 * ```http
 * Cross-Origin-Opener-Policy: same-origin
 * Cross-Origin-Embedder-Policy: require-corp
 * ```
 *
 * Instantiating checks for the isolation first, because a page that lost the
 * headers otherwise fails somewhere deeper with a stranger message.
 *
 * ## Where the payload comes from
 *
 * The payload ships as `@maplibre/native-ffi-runtime-emscripten-wasm32-opengl`,
 * and that is what these functions load when told nothing else. The specifier
 * is resolved by the host rather than by a bundler, so it reaches a payload
 * wherever the host resolves packages at runtime: Node, Deno, Bun, or a page
 * with an import map.
 *
 * A bundled page names the payload itself, in one of two ways:
 *
 * - Import the package and pass what it gave back as `module`. The bundler then
 *   emits the payload's `.wasm` and its worker script as assets of the
 *   application.
 * - Copy `maplibre-native-ffi.mjs` and `maplibre-native-ffi.wasm` out of the
 *   payload package, serve them side by side, and pass the `.mjs` URL as
 *   `moduleUrl`. The module finds the `.wasm` beside the URL it was loaded
 *   from.
 */

import { MaplibreError } from "./errors.ts";
import { type WasmModule, wasmTransport } from "./internal/wasm-transport.ts";
import { Maplibre } from "./maplibre.ts";
import type { OpenGlContext } from "./render.ts";

export type { WasmModule } from "./internal/wasm-transport.ts";

/** The published payload package this entry point loads by default. */
const DEFAULT_RUNTIME_PACKAGE =
  "@maplibre/native-ffi-runtime-emscripten-wasm32-opengl";

export interface WasmPayloadOptions {
  /**
   * Imports the payload from this URL instead of resolving a package.
   *
   * The URL names the payload's `maplibre-native-ffi.mjs`, which is what an
   * application that serves the payload as static files has.
   */
  readonly moduleUrl?: string | URL;
  /** Resolves this payload package instead of the published one. */
  readonly runtimePackage?: string;
  /**
   * What the payload's Emscripten factory is given.
   *
   * The module needs nothing here to run. This is for a host that has to
   * redirect where the module fetches its files from, or watch its output.
   */
  readonly moduleOptions?: Record<string, unknown>;
}

export interface BrowserLoadOptions extends WasmPayloadOptions {
  /**
   * Uses a payload this caller already instantiated.
   *
   * A caller that needs the module for itself, to make a WebGL context or to
   * reach the module's filesystem, instantiates it once and passes it here,
   * because a second instantiation is a second copy of the whole library.
   */
  readonly module?: WasmModule;
}

/**
 * Instantiates the payload module.
 *
 * Most callers want {@link loadBrowser} instead. This is the half that a caller
 * who also needs the module itself performs on its own.
 *
 * The page has to be cross-origin isolated, which means the document was served
 * with `Cross-Origin-Opener-Policy: same-origin` and
 * `Cross-Origin-Embedder-Policy: require-corp`. The module is threaded, and a
 * browser withholds `SharedArrayBuffer` from a page that is not isolated, so
 * this reports a page that lost the headers rather than failing deeper.
 */
export async function instantiateWasmPayload(
  options: WasmPayloadOptions = {},
): Promise<WasmModule> {
  requireCrossOriginIsolation();
  const factory = await payloadFactory(options);
  return await factory(options.moduleOptions ?? {});
}

/**
 * Loads the library over the WebAssembly payload.
 *
 * The payload comes from `@maplibre/native-ffi-runtime-emscripten-wasm32-opengl`
 * unless `module` or `moduleUrl` names another source, and the page has to be
 * cross-origin isolated for it to start; see {@link instantiateWasmPayload}.
 *
 * The handshake runs here as it does for every other payload, so a module built
 * from other headers is refused rather than dispatched into.
 */
export async function loadBrowser(
  options: BrowserLoadOptions = {},
): Promise<Maplibre> {
  const module = options.module ?? (await instantiateWasmPayload(options));
  return Maplibre.fromTransport(wasmTransport(module));
}

/**
 * A WebGL context this payload owns.
 *
 * This is the arm of the render API's `OpenGlContext` a browser produces, so it
 * goes to a render session as it is.
 */
export type WebGlContext = Extract<OpenGlContext, { platform: "webgl" }>;

/** What a browser context this payload can render through is made with. */
export interface WebGlContextOptions {
  readonly alpha?: boolean;
  readonly depth?: boolean;
  readonly stencil?: boolean;
  readonly antialias?: boolean;
  readonly premultipliedAlpha?: boolean;
  readonly preserveDrawingBuffer?: boolean;
  readonly powerPreference?: "default" | "low-power" | "high-performance";
}

/**
 * Makes a WebGL 2 context the payload owns, and leaves it current.
 *
 * A render session attaches to a context by the number Emscripten assigned it
 * rather than by address, so the context has to come from the module's own
 * registry: one this page asked the canvas for is an object the module cannot
 * name. The session attaches to whatever context is current when it is created,
 * which is why this leaves the new one current.
 */
export function createWebGlContext(
  module: WasmModule,
  canvas: HTMLCanvasElement | OffscreenCanvas,
  options: WebGlContextOptions = {},
): WebGlContext {
  const gl = (module as unknown as { GL?: EmscriptenGl }).GL;
  if (gl === undefined) {
    throw new MaplibreError(
      "unsupported",
      "this payload was linked without WebGL support",
    );
  }
  const handle = gl.createContext(canvas, {
    majorVersion: 2,
    minorVersion: 0,
    alpha: options.alpha ?? true,
    depth: options.depth ?? true,
    stencil: options.stencil ?? true,
    antialias: options.antialias ?? false,
    premultipliedAlpha: options.premultipliedAlpha ?? true,
    preserveDrawingBuffer: options.preserveDrawingBuffer ?? false,
    powerPreference: options.powerPreference ?? "default",
  });
  if (handle === 0) {
    throw new MaplibreError(
      "unsupported",
      "this browser gave the payload no WebGL 2 context",
    );
  }
  if (!gl.makeContextCurrent(handle)) {
    throw new MaplibreError(
      "invalidState",
      "the payload's WebGL context could not be made current",
    );
  }
  return { platform: "webgl", context: handle };
}

/** The WebGL registry an Emscripten module keeps, which is not the C API. */
interface EmscriptenGl {
  createContext(
    canvas: HTMLCanvasElement | OffscreenCanvas,
    attributes: Record<string, unknown>,
  ): number;
  makeContextCurrent(handle: number): boolean;
}

/** What the payload's module file exports, whichever way it was reached. */
type PayloadFactory = (options: Record<string, unknown>) => Promise<WasmModule>;

function requireCrossOriginIsolation(): void {
  // Only a browser answers this at all, so a host that leaves it undefined has
  // nothing to check. Node, Deno, and Bun all do.
  if (globalThis.crossOriginIsolated === false) {
    throw new MaplibreError(
      "unsupported",
      "this page is not cross-origin isolated, so a browser withholds the " +
        "SharedArrayBuffer the threaded payload needs; serve the document with " +
        "Cross-Origin-Opener-Policy: same-origin and " +
        "Cross-Origin-Embedder-Policy: require-corp",
    );
  }
}

async function payloadFactory(
  options: WasmPayloadOptions,
): Promise<PayloadFactory> {
  const specifier =
    options.moduleUrl === undefined
      ? (options.runtimePackage ?? DEFAULT_RUNTIME_PACKAGE)
      : String(options.moduleUrl);
  let payload: { createRuntime?: unknown; default?: unknown };
  try {
    // The specifier is a value rather than a literal, so a bundler leaves it
    // for the host to resolve. That is the point: a served payload is found by
    // URL at runtime, and a bundled one is passed in as `module` instead.
    payload = (await import(/* @vite-ignore */ specifier)) as typeof payload;
  } catch (error) {
    throw new MaplibreError(
      "invalidState",
      `no MapLibre Native WebAssembly payload could be loaded from ${specifier} ` +
        `(${error instanceof Error ? error.message : String(error)})`,
    );
  }
  // The payload package names its factory, and the module file it wraps is a
  // default export, so either is a payload.
  const factory = payload.createRuntime ?? payload.default;
  if (typeof factory !== "function") {
    throw new MaplibreError(
      "invalidState",
      `${specifier} is not a MapLibre Native WebAssembly payload`,
    );
  }
  return factory as PayloadFactory;
}
