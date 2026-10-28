// Completions: dispatch through the realm's queue, conversion failures,
// rejected submissions, failed commands, discarded commands, and waits that
// give up.

import assert from "node:assert/strict";

import {
  AwaitableWake,
  MapDebugFlag,
  MapMode,
  MaplibreError,
  Status,
} from "../src/index.ts";
import { native } from "../src/internal/addon.ts";
import { run } from "../src/internal/completions.ts";
import { mapOptions, openRuntime } from "./support/fixture.ts";
import { bounded, test } from "./support/harness.ts";
import { pendingCompletions } from "./support/internals.ts";
import { emptyStyle } from "./support/styles.ts";

test("a completion delivers once and reports a value that fails to convert", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();
  const pending = pendingCompletions();
  // The addon converts a JSON value through the realm's JSON.parse, so a
  // parse that throws fails the conversion.
  const parse = JSON.parse;
  const failure = new Error("the conversion failed");
  JSON.parse = () => {
    throw failure;
  };
  try {
    await assert.rejects(
      bounded(map.getGlobalState(), "the conversion"),
      (error) => error === failure,
    );
  } finally {
    JSON.parse = parse;
  }
  // The completion settled its Promise once and left nothing pending.
  assert.equal(pendingCompletions(), pending);
  await bounded(map.getGlobalState(), "a later call");
  assert.equal(pendingCompletions(), pending);
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

test("a failed command rejects with its status and generation", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();
  const committed = await bounded(
    map.setDebugOptions(MapDebugFlag.tileBorders),
    "a committed command",
  );

  await assert.rejects(
    bounded(map.removeStyleSource("missing-source"), "source removal"),
    (error) =>
      error instanceof MaplibreError &&
      error.status === Status.notFound &&
      error.diagnostic.length > 0 &&
      error.generation > committed.generation,
  );
});

test("discarded commands are fenced and synchronous rejection throws", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();

  // A discarded command returns once native accepts it, and a later
  // completion on the same map follows it.
  map.discarding.setDebugOptions(MapDebugFlag.tileBorders);
  map.discarding.addStyleSourceJson("discarded", {
    type: "geojson",
    data: { type: "FeatureCollection", features: [] },
  });
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
    ...mapOptions,
    mapMode: MapMode.staticValue,
  });
  await bounded(map.setStyleJson(emptyStyle()), "a command");

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
