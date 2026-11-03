// A script that loads a style with top-level await, waits for its
// style-loaded event, releases its runtime, and ends without calling exit. It
// holds no timer of its own, so the process ends exactly when nothing native
// keeps the event loop alive.

import { RuntimeEventType, RuntimeHandle } from "../../src/index.ts";
import { listen, mapOptions } from "../support/fixture.ts";
import { backgroundStyle } from "../support/styles.ts";

const runtime = await RuntimeHandle.create();
const events = await listen(runtime);
const map = runtime.createMap(mapOptions);
await map.ready;
await map.setStyleJson(backgroundStyle());
await events.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
await map.release();
await runtime.release();
console.log("style loaded and runtime released");
