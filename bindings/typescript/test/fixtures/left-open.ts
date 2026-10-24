// Loads a style and returns from the script with the runtime, its map, and
// their registrations still open. Nothing else holds the event loop, so the
// process ends when the script does.

import { RuntimeEventType, runtimeCreate } from "../../src/index.ts";
import { listen, styleRoute } from "../support/fixture.ts";
import { emptyStyle } from "../support/styles.ts";

const runtime = runtimeCreate();
const events = await listen(runtime);
await runtime.setResourceProvider({
  routes: [styleRoute("open://style.json")],
  callback() {},
});
const map = await runtime.createMap();
await map.setStyleJson(emptyStyle());
await map.addCustomGeometrySource("custom", { fetchTile() {} });
await events.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
console.log("returning with the runtime open");
