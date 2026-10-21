// Completions: delivery through the realm's queue, conversion failures,
// rejected submissions, failed dispositions, discarded commands, and waits
// that give up.

import assert from "node:assert/strict";

import { Decoder } from "../src/generated_native.ts";
import {
  AwaitableWake,
  CommandDisposition,
  MapDebugOption,
  MapMode,
  MaplibreError,
  type RuntimeHandle,
  runtimeCreate,
  Status,
} from "../src/index.ts";
import { native } from "../src/internal/addon.ts";
import { queryOwned, run } from "../src/internal/completions.ts";
import { adopt } from "../src/internal/owners.ts";
import { expectCommitted, extent, openRuntime } from "./support/fixture.ts";
import {
  bounded,
  collectGarbageUntil,
  teardown,
  test,
  watchCollection,
} from "./support/harness.ts";
import { captureLeaks, pendingCompletions } from "./support/internals.ts";
import { emptyStyle } from "./support/styles.ts";

test("a completion delivers once and disposes a value that fails to convert", async () => {
  const fixture = await openRuntime();
  let conversions = 0;
  // The map creation of RuntimeHandle.createMap, with an owner that fails to
  // build after the binding took the map from its batch.
  const creation = queryOwned(
    Decoder.map,
    (completion) =>
      native.mln_runtime_create_map(fixture.runtime.id, {}, completion),
    (handle) =>
      adopt(
        handle,
        () => {
          conversions++;
          throw new TypeError("the created map failed to convert");
        },
        (disposed) => native.mln_map_dispose(disposed, undefined),
      ),
  );
  await assert.rejects(bounded(creation, "map creation"), TypeError);
  assert.equal(conversions, 1);

  // The runtime closes only once no map remains, so the transferred map was
  // disposed when its owner failed to build.
  await bounded(fixture.runtime.close(), "runtime close");
});

test("a submission native rejects frees its completion state", async () => {
  const pending = pendingCompletions();
  // Native never knows the null runtime, so it refuses the call.
  await assert.rejects(
    run((completion) =>
      native.mln_runtime_clear_resource_transform(0n, completion),
    ),
    (error) =>
      error instanceof MaplibreError && error.status === Status.invalidArgument,
  );
  assert.equal(pendingCompletions(), pending);

  // A later submission still completes through the realm's queue.
  const fixture = await openRuntime();
  await bounded(fixture.runtime.clearResourceTransform(), "a later call");
  assert.equal(pendingCompletions(), pending);
});

/** Starts a map creation and drops its Promise, keeping a weak reference. */
async function dropMapCreation(
  runtime: RuntimeHandle,
): Promise<{ id: bigint; collected: () => boolean }> {
  let delivered: { id: bigint; collected: () => boolean } | undefined;
  void runtime.createMap({ initialExtent: extent }).then((map) => {
    delivered = { id: map.id, collected: watchCollection(map) };
  });
  // The creation's completion reaches the queue before the later call's.
  await bounded(runtime.clearResourceTransform(), "a later call");
  return delivered!;
}

test(
  "a dropped map creation is disposed once collected",
  async () => {
    const leaks = captureLeaks();
    const runtime = runtimeCreate();
    teardown(() => runtime.dispose());
    const map = await dropMapCreation(runtime);
    assert.equal(map.collected(), false);
    // The live map holds off the runtime's release.
    await assert.rejects(
      runtime.close(),
      (error) =>
        error instanceof MaplibreError && error.status === Status.invalidState,
    );

    // Collecting the map runs its owner token's finalizer, which disposes the
    // map outside any callback, and then the runtime releases.
    await collectGarbageUntil(
      () =>
        map.collected() &&
        leaks.some((line) =>
          line.includes(`MapHandle 0x${map.id.toString(16)}`),
        ),
      "the dropped map",
    );
    await bounded(runtime.clearResourceTransform(), "a later call");
    await bounded(runtime.close(), "runtime close");
  },
  { needs: ["forcedGc"] },
);

test("a failed command reports its disposition as data", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();

  const completion = await bounded(
    map.removeStyleSource("missing-source"),
    "source removal",
  );
  assert.equal(completion.disposition, CommandDisposition.failed);
  assert.equal(completion.status, Status.notFound);
  assert.ok(completion.diagnostic.length > 0);
});

test("discarded commands are fenced and synchronous rejection throws", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();

  // A discarded command returns once native accepts it, and a later
  // completion on the same map follows it.
  map.discarding.setDebugOptions(MapDebugOption.tileBorders);
  map.discarding.addStyleSourceJson(
    "discarded",
    JSON.stringify({
      type: "geojson",
      data: { type: "FeatureCollection", features: [] },
    }),
  );
  const source = await bounded(
    map.getStyleSource("discarded"),
    "the discarded source",
  );
  assert.equal(source?.id, "discarded");

  // A command that native rejects before accepting it throws at once.
  assert.throws(
    () => map.discarding.resize({ width: 0, height: 0, scaleFactor: 0 }),
    (error) =>
      error instanceof MaplibreError && error.status === Status.invalidArgument,
  );
});

/** Rejects with the signal's reason once it aborts. */
function aborted(signal: AbortSignal): Promise<never> {
  return new Promise((_resolve, reject) =>
    signal.addEventListener("abort", () => reject(signal.reason), {
      once: true,
    }),
  );
}

test("a wait that is abandoned leaves its completion to arrive later", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openMap({
    mapMode: MapMode.staticValue,
    initialExtent: extent,
  });
  await expectCommitted(map.setStyleJson(emptyStyle()));

  // A static map with no render session never produces the image, so the
  // request stays owed. A Promise cannot be cancelled; a waiter races it
  // against an AbortSignal and gives up instead.
  const image = map.requestStillImage();
  const giveUp = new AbortController();
  const waiting = Promise.race([image, aborted(giveUp.signal)]);
  giveUp.abort(new Error("the host stopped waiting"));
  await assert.rejects(waiting, /the host stopped waiting/);

  // A wait on a wake takes the same signal, and the wake it withdrew from
  // is kept for the next wait.
  const wake = new AwaitableWake();
  const withdrawn = new AbortController();
  const next = wake.next({ signal: withdrawn.signal });
  withdrawn.abort(new Error("withdrawn"));
  await assert.rejects(next, /withdrawn/);
  wake.wake();
  await bounded(wake.next(), "the kept wake");

  // The binding still settles the abandoned completion, once closing the map
  // cancels the request.
  const cancelled = assert.rejects(
    image,
    (error) =>
      error instanceof MaplibreError && error.status === Status.cancelled,
  );
  await bounded(map.close(), "map close");
  await bounded(cancelled, "the cancelled still image");
});
