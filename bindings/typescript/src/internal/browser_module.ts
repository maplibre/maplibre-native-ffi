// Loads the browser module: the binding's Node-API addon, compiled with the C
// library and a Node-API of its own into a wasm-bindgen package
// (bindings/typescript/web).
//
// Instantiating a module is asynchronous, so no call can load it on demand:
// loadModule() loads it, and every call before then throws.

import { type Addon, bindAddon } from "./addon.ts";
import { installRealm } from "./realm.ts";
import { installWebObjects } from "./web_objects.ts";

/** The package's bindings, `maplibre_native_ffi.js`. */
interface Bindings {
  readonly default: () => Promise<unknown>;
  readonly mlnTsAddon: () => Addon;
  readonly mlnWebObjectInsert: (value: unknown) => number;
  readonly mlnWebObjectGet: (handle: number) => unknown;
}

let loading: Promise<void> | undefined;

/**
 * Loads the browser module from the URL of its package's
 * `maplibre_native_ffi.js`, with the rest of the package beside it.
 *
 * The module runs on the thread that loads it, and never blocks it: load it
 * on a dedicated Web Worker to keep the map's work off the page, and await
 * what it reports. It starts its tile workers' Web Workers and loads its
 * font-face shaper from files of its package.
 *
 * Later calls return the first call's Promise. After a failure, the next call
 * loads again.
 */
export function loadModule(url: string | URL): Promise<void> {
  loading ??= load(String(url)).catch((error: unknown) => {
    loading = undefined;
    throw error;
  });
  return loading;
}

async function load(url: string): Promise<void> {
  const bindings = (await import(
    /* @vite-ignore */ /* webpackIgnore: true */ url
  )) as Bindings;
  await bindings.default();
  // Hosts report collected owners through console.warn, and host callback
  // errors through the global reportError.
  bindAddon(bindings.mlnTsAddon(), {}, installRealm);
  installWebObjects({
    insert: bindings.mlnWebObjectInsert,
    get: bindings.mlnWebObjectGet,
  });
}
