// Owner handles: repeated closes, refused closes, IDs, and what keeps an owner
// reachable.

import assert from "node:assert/strict";

import {
  MaplibreError,
  MapProjectionHandle,
  RuntimeEventType,
  RuntimeHandle,
  Status,
} from "../src/index.ts";
import { mapOptions, openRuntime } from "./support/fixture.ts";
import {
  bounded,
  collectGarbageUntil,
  test,
  watchCollection,
} from "./support/harness.ts";
import { captureLeaks, runtimeIsLive } from "./support/internals.ts";
import { emptyStyle } from "./support/styles.ts";

function isInvalidState(error: unknown): boolean {
  return error instanceof MaplibreError && error.status === Status.invalidState;
}

test("repeated asynchronous release reaches native", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openMap();

  // A synchronous close is idempotent.
  const projection = MapProjectionHandle.create(map.getSnapshot().view);
  projection.close();
  projection.close();

  // An asynchronous release after an accepted one reaches native code, which
  // reports the handle stale, as it does for any other call on it.
  await bounded(map.close(), "map close");
  await assert.rejects(map.close(), isInvalidState);
  assert.throws(() => map.getSnapshot(), isInvalidState);
  await assert.rejects(map.setStyleJson(emptyStyle()), isInvalidState);
});

test(
  "a disposer after an explicit close or dispose does nothing",
  async () => {
    const fixture = await openRuntime();

    const closed = await fixture.openMap();
    await bounded(closed.close(), "map close");
    await closed[Symbol.asyncDispose]();
    closed[Symbol.dispose]();

    const disposed = await RuntimeHandle.create();
    disposed.dispose();
    await disposed[Symbol.asyncDispose]();
    await disposed[Symbol.asyncDispose]();
  },
  { needs: ["disposers"] },
);

test("a handle names its id and whether it is closed", async () => {
  const fixture = await openRuntime();
  const projection = MapProjectionHandle.create(
    (await fixture.openMap()).getSnapshot().view,
  );
  const projectionId = `0x${projection.id.toString(16)}`;
  assert.equal(projection.isClosed, false);
  assert.equal(
    projection.toString(),
    `MapProjectionHandle(id=${projectionId}, closed=false)`,
  );
  projection.close();
  assert.equal(projection.isClosed, true);
  assert.equal(
    projection.toString(),
    `MapProjectionHandle(id=${projectionId}, closed=true)`,
  );

  const runtime = await RuntimeHandle.create();
  const runtimeId = `0x${runtime.id.toString(16)}`;
  assert.equal(runtime.isClosed, false);
  assert.equal(String(runtime), `RuntimeHandle(id=${runtimeId}, closed=false)`);
  runtime.dispose();
  assert.equal(runtime.isClosed, true);
  assert.equal(String(runtime), `RuntimeHandle(id=${runtimeId}, closed=true)`);
  // A second dispose is a no-op.
  runtime.dispose();
});

test("a refused close leaves the handle usable for a later close", async () => {
  const runtime = await RuntimeHandle.create();
  const { map, ready } = runtime.createMap(mapOptions);
  await bounded(ready, "map creation");

  // The live map makes native refuse the release before teardown starts.
  await assert.rejects(runtime.close(), isInvalidState);
  await bounded(
    runtime.clearResourceTransform(),
    "a call on the refused runtime",
  );

  await bounded(map.close(), "map close");
  await bounded(runtime.close(), "runtime close");
  assert.equal(runtimeIsLive(runtime.id), false);
});

test("a handle id names its events and outlives close", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();
  const loaded = await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded);
  assert.equal(loaded.source, map.id);

  await bounded(map.close(), "map close");
  // A host routes events that arrive after close by the ID it kept.
  assert.equal(map.id, loaded.source);
});

/** Creates a runtime and a map on it, and keeps only the map. */
async function mapWithoutItsRuntime() {
  const runtime = await RuntimeHandle.create();
  const { map, ready } = runtime.createMap(mapOptions);
  await bounded(ready, "map creation");
  return {
    map,
    runtime: new WeakRef(runtime),
    collected: watchCollection(runtime),
    id: runtime.id,
  };
}

async function unreachableRuntime(): Promise<() => boolean> {
  return watchCollection(await RuntimeHandle.create());
}

test(
  "a map keeps its runtime reachable",
  async () => {
    captureLeaks();
    const { map, runtime, collected, id } = await mapWithoutItsRuntime();
    // The same collections reclaim a runtime that nothing keeps.
    const controlCollected = await unreachableRuntime();
    await collectGarbageUntil(controlCollected, "a runtime without a map");
    assert.equal(collected(), false, "the map's runtime was collected");
    assert.ok(runtimeIsLive(id));

    await bounded(map.close(), "map close");
    await bounded(runtime.deref()!.close(), "runtime close");
  },
  { needs: ["forcedGc"] },
);
