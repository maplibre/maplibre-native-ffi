// The entry of a spawned @ohos.worker realm that loads the binding as an
// application does, from the OHPM package that test-arkts.sh packed: its
// bundled ArkTS entry, and the addon and C++ runtime in its libs. It shares no
// module with the suite, so the package alone must load.

import { cVersion, RuntimeHandle } from "@maplibre/native-ffi";

import { postToParent, realmData } from "./realm-child.ts";

await realmData();
const runtime = await RuntimeHandle.create();
await runtime.close();
postToParent({ version: cVersion() });
