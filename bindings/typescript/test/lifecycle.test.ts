// Owner handles: repeated releases, refused releases, scoped cleanup, IDs, and
// what keeps an owner reachable.

import assert from "node:assert/strict";

import {
  MaplibreError,
  MapProjectionHandle,
  RuntimeEventType,
  RuntimeHandle,
  Status,
} from "../src/index.ts";
import { mapOptions, openRuntime } from "./support/fixture.ts";
import { bounded, test, turn } from "./support/harness.ts";
import { capabilities } from "./support/host-capabilities.ts";
import { emptyStyle } from "./support/styles.ts";

function isInvalidState(error: unknown): boolean {
  return error instanceof MaplibreError && error.status === Status.invalidState;
}

test(
  "a call before the browser module loads throws",
  () => {
    const error = capabilities.beforeLoad!();
    assert.ok(error instanceof Error, "the call did not throw");
    assert.match(error.message, /call loadModule\(\) first/);
  },
  { needs: ["beforeLoad"] },
);

test("repeated asynchronous release reaches native", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openMap();

  // A synchronous release is idempotent.
  const projection = MapProjectionHandle.create(map.getSnapshot().view);
  projection.release();
  projection.release();

  // An asynchronous release after an accepted one reaches native code, which
  // reports the handle stale, as it does for any other call on it.
  await bounded(map.release(), "map release");
  await assert.rejects(map.release(), isInvalidState);
  assert.throws(() => map.getSnapshot(), isInvalidState);
  await assert.rejects(map.setStyleJson(emptyStyle()), isInvalidState);
});

test(
  "a disposer after an explicit release or dispose does nothing",
  async () => {
    const fixture = await openRuntime();

    const released = await fixture.openMap();
    await bounded(released.release(), "map release");
    await released[Symbol.asyncDispose]();
    released[Symbol.dispose]();

    const disposed = RuntimeHandle.create();
    disposed.dispose();
    await disposed[Symbol.asyncDispose]();
    await disposed[Symbol.asyncDispose]();
  },
  { needs: ["disposers"] },
);

test(
  "await using waits for the release, and using disposes",
  async () => {
    const fixture = await openRuntime();
    const map = await fixture.openMap();
    const release = map.release.bind(map);
    let retired = false;
    map.release = () =>
      release().then(() => {
        retired = true;
      });
    await map[Symbol.asyncDispose]();
    assert.equal(retired, true);

    const disposed = fixture.runtime.createMap(mapOptions);
    await bounded(disposed.ready, "map creation");
    disposed.release = () => assert.fail("using released the map");
    disposed[Symbol.dispose]();
    assert.equal(disposed.isClosed, true);
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
  projection.release();
  assert.equal(projection.isClosed, true);
  assert.equal(
    projection.toString(),
    `MapProjectionHandle(id=${projectionId}, closed=true)`,
  );

  const runtime = RuntimeHandle.create();
  const runtimeId = `0x${runtime.id.toString(16)}`;
  assert.equal(runtime.isClosed, false);
  assert.equal(String(runtime), `RuntimeHandle(id=${runtimeId}, closed=false)`);
  runtime.dispose();
  assert.equal(runtime.isClosed, true);
  assert.equal(String(runtime), `RuntimeHandle(id=${runtimeId}, closed=true)`);
  // A second dispose is a no-op.
  runtime.dispose();
});

test("a refused release leaves the handle usable for a later release", async () => {
  const runtime = RuntimeHandle.create();
  const map = runtime.createMap(mapOptions);
  await bounded(map.ready, "map creation");

  // The live map makes native refuse the release before teardown starts.
  await assert.rejects(runtime.release(), isInvalidState);
  await bounded(
    runtime.clearResourceTransform(),
    "a call on the refused runtime",
  );

  await bounded(map.release(), "map release");
  await bounded(runtime.release(), "runtime release");
});

test("a handle id names its events and outlives its release", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();
  const loaded = await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded);
  assert.equal(loaded.source, map.id);

  await bounded(map.release(), "map release");
  // A host routes events that arrive after the release by the ID it kept.
  assert.equal(map.id, loaded.source);
});

/** Creates a runtime and a map on it, and keeps only the map. */
async function mapWithoutItsRuntime() {
  const runtime = RuntimeHandle.create();
  const map = runtime.createMap(mapOptions);
  await bounded(map.ready, "map creation");
  return { map, runtime: new WeakRef(runtime) };
}

test(
  "a map keeps its runtime reachable",
  async () => {
    const { map, runtime } = await mapWithoutItsRuntime();
    // A new WeakRef holds its target until the job that made it ends, so the
    // collection waits a turn. Had the collector reclaimed the runtime, its
    // owner token would have disposed the runtime and the map, and both
    // releases would fail.
    await turn();
    capabilities.forcedGc!();
    await turn();
    await bounded(map.release(), "map release");
    await bounded(runtime.deref()!.release(), "runtime release");
  },
  { needs: ["forcedGc"] },
);
