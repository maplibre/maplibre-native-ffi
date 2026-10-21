// A script that loads a style with top-level await, waits for its
// style-loaded event, closes its runtime, and ends without calling exit. It
// holds no timer of its own, so the process ends exactly when nothing native
// keeps the event loop alive.

import {
  CommandDisposition,
  RuntimeEventType,
  runtimeCreate,
} from "../../src/index.ts";
import { listen } from "../support/fixture.ts";
import { backgroundStyle } from "../support/styles.ts";

const runtime = runtimeCreate();
const events = await listen(runtime);
const map = await runtime.createMap({
  initialExtent: { width: 64, height: 64, scaleFactor: 1 },
});
const receipt = await map.setStyleJson(backgroundStyle());
if (receipt.disposition !== CommandDisposition.committed) {
  throw new Error(`setStyleJson did not commit: ${receipt.diagnostic}`);
}
await events.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
await map.close();
await runtime.close();
console.log("style loaded and runtime closed");
