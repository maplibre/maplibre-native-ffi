// The Kotlin/Wasm binding's runtime: the browser package of one render
// backend, which a bundler takes in through this import of the package's
// entry (index.js), and the module object over it (runtime.mjs).

import * as bindings from "./index.js";
import { instantiateFrom } from "./runtime.mjs";

/** Instantiates the package's module and returns the module object. */
export default function instantiate(options) {
  return instantiateFrom(bindings, options);
}
