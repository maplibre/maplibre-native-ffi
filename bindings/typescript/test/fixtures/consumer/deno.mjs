// A Deno script that imports the installed facade through an npm specifier.

import { cVersion, RuntimeHandle } from "npm:@maplibre/native-ffi";

const runtime = await RuntimeHandle.create();
await runtime.close();
console.log(`loaded C API version ${cVersion()}`);
