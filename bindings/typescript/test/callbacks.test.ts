// Callback registrations: each one roots its host value while native code
// holds it, and releases the root when native code retires the registration.

import assert from "node:assert/strict";

import {
  type CameraTransitionEnd,
  CameraTransitionOutcome,
  CameraUpdateMode,
  MaplibreError,
  Status,
} from "../src/index.ts";
import { rootsOf } from "../src/internal/owners.ts";
import { expectCommitted, openRuntime } from "./support/fixture.ts";
import { bounded, callbackArrival, test } from "./support/harness.ts";
import { liveRoots, released } from "./support/internals.ts";

function isInvalidArgument(error: unknown): boolean {
  return (
    error instanceof MaplibreError && error.status === Status.invalidArgument
  );
}

test("a callback root is released by a removal, a map close, or a refusal", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();
  const own = rootsOf(map).roots;
  const baseline = liveRoots();

  await expectCommitted(
    map.addCustomGeometrySource("source", { fetchTile() {} }),
  );
  assert.equal(own.size, 1);
  assert.equal(liveRoots(), baseline + 1);
  await expectCommitted(map.removeStyleSource("source"));
  await released(
    () => own.size === 0 && liveRoots() === baseline,
    "the release after a removal",
  );

  // A registration that native code refuses is never rooted.
  await assert.rejects(
    map.addCustomGeometrySource("refused", { tileSize: 0, fetchTile() {} }),
    isInvalidArgument,
  );
  assert.equal(own.size, 0);
  assert.equal(liveRoots(), baseline);

  await expectCommitted(
    map.addCustomGeometrySource("source", { fetchTile() {} }),
  );
  assert.equal(own.size, 1);
  await bounded(map.close(), "map close");
  await released(
    () => own.size === 0 && liveRoots() === baseline,
    "the release after a map close",
  );
});

// A camera command carries its end handler two records deep in its input.
// The handler's root lasts until native runs the handler once and retires it,
// and a command that native refuses roots nothing.
test("a camera end handler root is released after the handler runs", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();
  const own = rootsOf(map).roots;
  let end!: (value: CameraTransitionEnd) => void;
  const ended = new Promise<CameraTransitionEnd>((resolve) => (end = resolve));

  await expectCommitted(
    map.updateCamera({
      camera: { zoom: 4 },
      mode: CameraUpdateMode.ease,
      animation: { durationMs: 60_000, transitionId: 3n, endHandler: end },
    }),
  );
  assert.equal(own.size, 1);
  await expectCommitted(map.cancelCameraTransition(3n));
  const result = await callbackArrival(ended, "the end handler");
  assert.equal(result.outcome, CameraTransitionOutcome.cancelled);
  assert.notEqual(result.generation, 0n);
  await released(() => own.size === 0, "the release after the end");

  await assert.rejects(
    map.applyCameraDelta({ scale: -1, animation: { endHandler() {} } }),
    isInvalidArgument,
  );
  assert.equal(own.size, 0);
});
