// A CommonJS module that imports the installed facade, which TypeScript
// compiles to require(). The facade is an ES module, which require() loads.

import {
  cVersion,
  type RuntimeHandle,
  runtimeCreate,
} from "@maplibre/native-ffi";

// The check names no host types, so it declares the one global it prints with.
declare const console: { log(line: string): void };

const runtime: RuntimeHandle = runtimeCreate();
void runtime.close().then(() => {
  console.log(`loaded C API version ${cVersion()}`);
});
