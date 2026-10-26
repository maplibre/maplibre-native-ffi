// Loads the browser module: the Emscripten build of the addon, over emnapi's
// implementation of Node-API.
//
// Instantiating a module is asynchronous, so no call can load it on demand:
// loadModule() loads it, and every call before then throws.

import { createContext } from "@emnapi/runtime";

import { type Addon, bindAddon } from "./addon.ts";
import { installRealm } from "./realm.ts";
import {
  installWebGraphics,
  type WebGraphicsLibraries,
} from "./web_graphics.ts";

/** Options for {@link loadModule}. */
export interface LoadModuleOptions {
  /**
   * The Web Workers to start for native threads. The default covers one
   * runtime with one map, one GeoJSON source, and one render session. Raise
   * it for more, or new threads wait until the page yields.
   */
  readonly pthreadPoolSize?: number;
}

/** The Emscripten module that the factory resolves with. */
interface BrowserModule extends WebGraphicsLibraries {
  emnapiInit(options: { readonly context: unknown }): Addon;
}

// The module's link reads the pool size as Module.mlnPoolSize, and a size of 0
// selects the link's default.
type Factory = (settings: {
  readonly mlnPoolSize: number;
}) => Promise<BrowserModule>;

let loading: Promise<void> | undefined;

/**
 * Loads the browser module from the URL of its `maplibre_native_ffi.mjs`,
 * with its `.wasm` beside it. The page must be cross-origin isolated.
 *
 * Later calls return the first call's Promise. After a failure, the next call
 * loads again.
 */
export function loadModule(
  url: string | URL,
  options: LoadModuleOptions = {},
): Promise<void> {
  loading ??= load(String(url), options.pthreadPoolSize).catch(
    (error: unknown) => {
      loading = undefined;
      throw error;
    },
  );
  return loading;
}

async function load(url: string, poolSize = 0): Promise<void> {
  if (!Number.isSafeInteger(poolSize) || poolSize < 0) {
    throw new RangeError(
      `pthreadPoolSize must be a positive integer, got ${poolSize}`,
    );
  }
  const { default: factory } = (await import(
    /* @vite-ignore */ /* webpackIgnore: true */ url
  )) as { default: Factory };
  const module = await factory({ mlnPoolSize: poolSize });
  // Hosts report collected owners through console.warn, and host callback
  // errors through the global reportError.
  bindAddon(module.emnapiInit({ context: createContext() }), {}, installRealm);
  installWebGraphics(module);
}
