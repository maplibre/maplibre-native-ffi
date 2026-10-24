// Realms: the main thread and workers, each with its own delivery queue and
// wake, sharing the native core without a host pump.

import assert from "node:assert/strict";

import { RuntimeEventType } from "../src/index.ts";
import { openRuntime } from "./support/fixture.ts";
import { bounded, test } from "./support/harness.ts";
import {
  nextMessage,
  spawnRealm,
  startRealm,
  terminateRealm,
} from "./support/realms.ts";
import { loadStyle, routeRequests } from "./support/scenarios.ts";
import { emptyStyle } from "./support/styles.ts";

test(
  "two runtimes on one thread and in two workers load styles without a pump",
  async () => {
    const workers = [
      spawnRealm({ scenario: "loadStyle" }),
      spawnRealm({ scenario: "loadStyle" }),
    ];
    // This thread waits on binding Promises and event waits alone, with no
    // timer and no pump.
    await Promise.all([loadStyle(), loadStyle()]);
    for (const [index, worker] of workers.entries()) {
      assert.deepEqual(await nextMessage(worker, `worker ${index}`), {
        loaded: true,
      });
    }
  },
  { needs: ["realmSpawn"] },
);

test(
  "provider requests and tile fetches reach only the realm that registered them",
  async () => {
    const worker = spawnRealm({ scenario: "routeRequests", scheme: "worker" });
    const own = await routeRequests("main");
    const other = await nextMessage<typeof own>(worker, "the worker's routes");

    assert.deepEqual(own.requests, ["main://style.json"]);
    assert.deepEqual(other.requests, ["worker://style.json"]);
    assert.ok(own.tiles.length > 0, "the main realm fetched no tile");
    assert.ok(other.tiles.length > 0, "the worker fetched no tile");
    assert.ok(
      own.tiles.every((tile) => tile.startsWith("main/")),
      String(own.tiles),
    );
    assert.ok(
      other.tiles.every((tile) => tile.startsWith("worker/")),
      String(other.tiles),
    );
  },
  { needs: ["realmSpawn"] },
);

test(
  "terminating a worker mid-burst leaves the other realm working",
  async () => {
    const fixture = await openRuntime();
    const map = await fixture.openStyledMap();
    // Native threads keep posting completions, wakes, and end handlers to the
    // worker's queue while the worker's environment is torn down. After each
    // termination, this realm still receives its own completions and events.
    for (let iteration = 0; iteration < 3; iteration++) {
      const worker = startRealm({ scenario: "burst" });
      await nextMessage(worker, `burst ${iteration}`);
      await terminateRealm(worker, `termination ${iteration}`);

      await bounded(
        map.setStyleJson(
          JSON.stringify({
            version: 8,
            sources: {},
            layers: [{ id: `layer-${iteration}`, type: "background" }],
          }),
        ),
        "a command",
      );
      await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
    }
    await bounded(
      fixture.runtime.clearResourceTransform(),
      "a call after the bursts",
    );
  },
  { needs: ["realmSpawn"] },
);

test(
  "a worker that calls process.exit leaves the other realm working",
  async () => {
    const fixture = await openRuntime();
    const map = await fixture.openMap();
    // The worker's exit event ends only its own realm. Its queue closes with
    // its environment, and every other realm keeps receiving.
    const worker = spawnRealm({ scenario: "exit" });
    assert.deepEqual(await nextMessage(worker, "the worker's exit"), {
      exited: true,
    });
    await bounded(map.setStyleJson(emptyStyle()), "a command");
    await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
  },
  { needs: ["realmSpawn", "processExit"] },
);
