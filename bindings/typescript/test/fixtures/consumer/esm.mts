// An ES module that imports the installed facade.

import { cVersion, RuntimeHandle } from "@maplibre/native-ffi";

// The check names no host types, so it declares the one global it prints with.
declare const console: { log(line: string): void };

const runtime: RuntimeHandle = await RuntimeHandle.create();
await runtime.close();
console.log(`loaded C API version ${cVersion()}`);
