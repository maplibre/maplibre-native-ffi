// A script that loads a style with top-level await, waits for its
// style-loaded event, closes its runtime, and ends without calling exit. It
// holds no timer of its own, so the process ends exactly when nothing native
// keeps the event loop alive.

import { RuntimeEventType, RuntimeHandle } from "../../src/index.ts";
import { listen, mapOptions } from "../support/fixture.ts";
import { backgroundStyle } from "../support/styles.ts";

const runtime = RuntimeHandle.create();
const events = await listen(runtime);
const { map, ready } = runtime.createMap(mapOptions);
await ready;
await map.setStyleJson(backgroundStyle());
await events.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
await map.close();
await runtime.close();
console.log("style loaded and runtime closed");
