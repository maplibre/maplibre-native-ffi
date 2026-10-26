// A spawned realm that runs one scenario from test/support/scenarios.ts and
// posts its result, or posts once a scenario that never ends has started.

import process from "node:process";

import { RuntimeHandle } from "../../src/index.ts";
import { mapOptions } from "../support/fixture.ts";
import { execute } from "../support/harness.ts";
import { postToParent, realmData } from "../support/realm-child.ts";
import {
  burst,
  liveNativeWork,
  loadStyle,
  routeRequests,
} from "../support/scenarios.ts";
import { emptyStyle } from "../support/styles.ts";

type Request =
  | { scenario: "loadStyle" }
  | { scenario: "routeRequests"; scheme: string }
  | { scenario: "burst" }
  | { scenario: "liveNativeWork" }
  | { scenario: "exit" };

/**
 * Leaves a runtime with a styled map open and ends the worker through
 * process.exit(). The binding's exit listener, armed by the first runtime, runs
 * before the one this scenario adds, which tells the parent that it has run.
 */
async function exit(): Promise<never> {
  const runtime = await RuntimeHandle.create();
  const { map, ready } = runtime.createMap(mapOptions);
  await ready;
  await map.setStyleJson(emptyStyle());
  process.on("exit", () => postToParent({ exited: true }));
  process.exit(0);
}

const request = await realmData<Request>();
switch (request.scenario) {
  case "loadStyle":
    await execute({ name: "loadStyle", needs: [], run: loadStyle });
    postToParent({ loaded: true });
    break;
  case "routeRequests": {
    let result: unknown;
    await execute({
      name: "routeRequests",
      needs: [],
      run: async () => {
        result = await routeRequests(request.scheme);
      },
    });
    postToParent(result);
    break;
  }
  case "burst":
    await burst(() => postToParent({ bursting: true }));
    break;
  case "liveNativeWork":
    // The case never ends, so its teardowns never close the work before the
    // parent terminates this realm.
    void execute({
      name: "liveNativeWork",
      needs: [],
      run: async () => {
        await liveNativeWork();
        postToParent({ live: true });
        await new Promise<never>(() => {});
      },
    });
    break;
  case "exit":
    await exit();
    break;
}
