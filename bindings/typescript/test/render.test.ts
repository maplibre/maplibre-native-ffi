// Owned-texture sessions on a core worker and on the JavaScript thread, the
// scope of a borrowed frame view, a drain that finds nothing, and the native
// registrations that a session's attach options take.

import assert from "node:assert/strict";

import {
  type MapHandle,
  MaplibreError,
  QueueLock,
  Status,
  Wake,
} from "../src/index.ts";
import { openRuntime } from "./support/fixture.ts";
import {
  attachSession,
  attachSessionOverOwnGraphics,
  backend,
  centerPixel,
  NativeProbe,
  renderExtent,
  renderSize,
  requestFrame,
  TestSession,
} from "./support/graphics.ts";
import { bounded, test } from "./support/harness.ts";
import { backgroundStyle } from "./support/styles.ts";

const red = [255, 0, 0, 255];

async function openRenderMap(style = backgroundStyle()): Promise<MapHandle> {
  const fixture = await openRuntime();
  const map = await fixture.openMap({ initialExtent: renderExtent });
  await bounded(map.setStyleJson(style), "a command");
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

test(
  "using disposes an attached session, and the map's release then lets the host destroy its graphics objects",
  async () => {
    const map = await openRenderMap();
    const { session, completed, destroyGraphics } =
      attachSessionOverOwnGraphics(map);
    await bounded(completed, "the attachment");
    requestFrame(session, 1n);
    session[Symbol.dispose]();
    session[Symbol.dispose]();
    assert.throws(() => requestFrame(session, 2n), MaplibreError);
    await bounded(map.close(), "the map release");
    destroyGraphics();
  },
  { needs: ["disposers"] },
);

test(
  "await using waits until an attached session has retired, so the host may destroy its graphics objects",
  async () => {
    const map = await openRenderMap();
    const { session, completed, destroyGraphics } =
      attachSessionOverOwnGraphics(map);
    await bounded(completed, "the attachment");
    requestFrame(session, 1n);
    await bounded(session[Symbol.asyncDispose](), "the disposal");
    await session[Symbol.asyncDispose]();
    destroyGraphics();
    await bounded(map.close(), "the map release");
  },
  { needs: ["disposers"] },
);

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

function failsWith(status: Status) {
  return (error: unknown) =>
    error instanceof MaplibreError && error.status === status;
}

function nativeWake(probe: NativeProbe, callback = probe.wake) {
  return Wake.native({
    callback,
    userData: probe.userData,
    releaseUserData: probe.release,
  });
}

test("a native registration reaches native unchanged, a call consumes it, and a refused call releases it", async () => {
  const map = await openRenderMap();

  // Native code calls the probe's wake with the probe's context directly.
  const probe = new NativeProbe();
  const frameWake = nativeWake(probe);
  const { session, completed } = attachSession(map, { frameWake });
  await bounded(completed, "the attachment");
  requestFrame(session, 1n);
  await probe.arrival("wake");

  // The attach call consumed the registration, so another call throws before
  // it reaches native code.
  assert.throws(
    () => attachSession(map, { frameWake }),
    failsWith(Status.invalidState),
  );

  // Native code releases the context that it accepted once the session ends.
  await bounded(session.detach(), "the detach");
  session.dispose();
  await probe.arrival("release");

  // Native code refuses a disabled wake that carries a release, so the
  // binding releases the context.
  const refused = new NativeProbe();
  assert.throws(
    () => attachSession(map, { frameWake: nativeWake(refused, 0n) }),
    failsWith(Status.invalidArgument),
  );
  await refused.arrival("release");

  // A conversion error after the binding took the registration releases it
  // too.
  const unconverted = new NativeProbe();
  assert.throws(
    () =>
      attachSession(map, {
        frameWake: nativeWake(unconverted),
        driverWorkWake: 1 as unknown as Wake,
      }),
    TypeError,
  );
  await unconverted.arrival("release");
  assert.deepEqual(unconverted.events, ["release"]);
});

test("a native queue lock reaches a session whose target takes one", async () => {
  const map = await openRenderMap();
  const probe = new NativeProbe();
  const queueLock = QueueLock.native({
    lock: probe.lock,
    unlock: probe.unlock,
    userData: probe.userData,
    releaseUserData: probe.release,
  });
  if (backend !== "vulkan") {
    // Only a Vulkan target takes a queue lock, and the binding releases the
    // context of the attach that native code refuses.
    assert.throws(
      () => attachSession(map, { queueLock }),
      failsWith(Status.unsupported),
    );
    await probe.arrival("release");
    assert.deepEqual(probe.events, ["release"]);
    return;
  }
  const { session, completed } = attachSession(map, { queueLock });
  await bounded(completed, "the attachment");
  requestFrame(session, 1n);
  await probe.arrival("unlock");
  await bounded(session.detach(), "the detach");
  session.dispose();
  await probe.arrival("release");
  assert.equal(probe.events.at(-1), "release");
});
