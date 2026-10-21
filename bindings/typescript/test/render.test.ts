// Owned-texture sessions on a core worker and on the JavaScript thread, the
// scope of a borrowed frame view, and a drain that finds nothing.

import assert from "node:assert/strict";

import { type MapHandle, MaplibreError, Status } from "../src/index.ts";
import { expectCommitted, openRuntime } from "./support/fixture.ts";
import {
  backend,
  centerPixel,
  renderExtent,
  renderSize,
  TestSession,
} from "./support/graphics.ts";
import { test } from "./support/harness.ts";
import { backgroundStyle } from "./support/styles.ts";

const red = [255, 0, 0, 255];

async function openRenderMap(style = backgroundStyle()): Promise<MapHandle> {
  const fixture = await openRuntime();
  const map = await fixture.openMap({ initialExtent: renderExtent });
  await expectCommitted(map.setStyleJson(style));
  return map;
}

test("an owned texture renders a frame that reads back as pixels", async () => {
  const map = await openRenderMap();
  const worker = await TestSession.attach(map);

  const result = await worker.renderFrame();
  assert.ok(result.frameGeneration > 0n);
  const image = await worker.settle(
    worker.session.readTexture(),
    "the readback",
  );
  assert.equal(image.info.width, renderSize);
  assert.equal(image.info.height, renderSize);
  assert.deepEqual(centerPixel(image), red);
});

test("a drain before any demand returns no batch", async () => {
  const map = await openRenderMap();
  const worker = await TestSession.attach(map);
  // Native reports the drain as not ready, which reads as undefined.
  assert.equal(worker.session.drainFrameResults(), undefined);
});

test("a caller-driven session is serviced from the event loop", async () => {
  const map = await openRenderMap();
  const caller = await TestSession.attach(map, "caller");

  await caller.renderFrame();
  const image = await caller.settle(
    caller.session.readTexture(),
    "the readback",
  );
  assert.deepEqual(centerPixel(image), red);
});

/** A session whose rendered frames this binding can acquire and view. */
async function frameSession(map: MapHandle) {
  // An EGL core-worker session keeps its frames in its own context, so only a
  // session that shares the host's context exposes them.
  const attached = await TestSession.attach(
    map,
    backend === "egl" ? "caller" : "worker",
  );
  await attached.renderFrame();
  return attached.session;
}

test("a frame view expires with its scope and holds off release inside it", async () => {
  const map = await openRenderMap();
  const session = await frameSession(map);
  const frame = session.acquireFrame();
  assert.ok(frame, "the session exposed no frame");

  const withTexture = <T>(use: (view: { readonly width: number }) => T): T =>
    backend === "metal"
      ? frame.withMetalTexture(use)
      : backend === "vulkan"
        ? frame.withVulkanTexture(use)
        : frame.withOpenglTexture(use);
  let escaped: { readonly width: number } | undefined;
  const width = withTexture((view) => {
    escaped = view;
    // The view borrows the frame, so native refuses to release the frame or
    // abandon its session while the view is open.
    assert.throws(
      () => frame.release(),
      (error) => error instanceof MaplibreError,
    );
    assert.throws(
      () => session.abandon(),
      (error) => error instanceof MaplibreError && error.status === Status.busy,
    );
    return view.width;
  });
  assert.equal(width, renderSize);
  // A view that escapes its scope refuses every read.
  assert.throws(
    () => escaped!.width,
    (error) =>
      error instanceof MaplibreError && error.status === Status.invalidState,
  );

  frame.release();
  // A released frame reaches native code, which reports it stale.
  assert.throws(
    () => frame.getResult(),
    (error) =>
      error instanceof MaplibreError && error.status === Status.invalidState,
  );
});
