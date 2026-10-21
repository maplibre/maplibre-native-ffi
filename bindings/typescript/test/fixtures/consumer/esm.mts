// An ES module that imports the installed facade.

import {
  cVersion,
  type RuntimeHandle,
  runtimeCreate,
} from "@maplibre/native-ffi";

// The check names no host types, so it declares the one global it prints with.
declare const console: { log(line: string): void };

const runtime: RuntimeHandle = runtimeCreate();
await runtime.close();
console.log(`loaded C API version ${cVersion()}`);
