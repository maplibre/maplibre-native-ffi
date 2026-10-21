// A Deno script that imports the installed facade through an npm specifier.

import { cVersion, runtimeCreate } from "npm:@maplibre/native-ffi";

const runtime = runtimeCreate();
await runtime.close();
console.log(`loaded C API version ${cVersion()}`);
