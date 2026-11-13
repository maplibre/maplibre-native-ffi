// Offscreen sessions on a core worker, a borrowed texture serviced on the
// JavaScript thread, the scope of a frame view, a drain that finds nothing,
// and the native registrations that attach options and GPU devices take.

import assert from "node:assert/strict";

import {
  GpuDeviceHandle,
  GpuSyncKind,
  type MapHandle,
  MaplibreError,
  QueueLock,
  Status,
  Wake,
} from "../src/index.ts";
import { openRuntime } from "./support/fixture.ts";
import {
  attachCallerSession,
  attachSession,
  attachVulkanSession,
  backend,
  centerPixel,
  NativeProbe,
  renderExtent,
  renderSize,
  demandFrame,
  TestSession,
} from "./support/graphics.ts";
import { bounded, test, turn } from "./support/harness.ts";
import { backgroundStyle } from "./support/styles.ts";

const red = [255, 0, 0, 255];

async function openRenderMap(style = backgroundStyle()): Promise<MapHandle> {
  const fixture = await openRuntime();
  const map = await fixture.openMap({ initialExtent: renderExtent });
  await bounded(map.setStyleJson(style), "a command");
  return map;
}

test("an offscreen session renders a frame that reads back as pixels", async () => {
  const map = await openRenderMap();
  const worker = await TestSession.offscreen(map);

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
  "using disposes an attached session, which then refuses calls, and the map releases",
  async () => {
    const map = await openRenderMap();
    const session = attachSession(map, {});
    await bounded(session.ready, "the attachment");
    demandFrame(session, 1n);
    session[Symbol.dispose]();
    session[Symbol.dispose]();
    assert.throws(() => demandFrame(session, 2n), MaplibreError);
    await bounded(map.release(), "the map release");
  },
  { needs: ["disposers"] },
);

test(
  "await using waits until an attached session has retired, so the map takes another",
  async () => {
    const map = await openRenderMap();
    const session = attachSession(map, {});
    await bounded(session.ready, "the attachment");
    demandFrame(session, 1n);
    await bounded(session[Symbol.asyncDispose](), "the disposal");
    await session[Symbol.asyncDispose]();
    // Native code refuses an attach while the map still has a session.
    const next = attachSession(map, {});
    await bounded(next.ready, "the next attachment");
  },
  { needs: ["disposers"] },
);

test("a custom source answers through the map it borrows, which it can't release", async () => {
  const map = await openRenderMap();
  const calls: [bigint, string, unknown][] = [];
  await bounded(
    map.addCustomGeometrySource("custom", {
      fetchTile: (borrowed, sourceId, tile) => {
        borrowed.discarding.setCustomGeometrySourceTileData(sourceId, tile, {
          type: "FeatureCollection",
          features: [],
        });
        let refusal: unknown;
        try {
          borrowed.dispose();
        } catch (error) {
          refusal = error;
        }
        calls.push([borrowed.id, sourceId, refusal]);
      },
    }),
    "a command",
  );
  await bounded(
    map.addStyleLayerJson({ id: "dots", type: "circle", source: "custom" }),
    "a command",
  );
  const worker = await TestSession.offscreen(map);
  // The map asks for its tiles as it renders.
  for (let frame = 0; frame < 32 && calls.length === 0; frame++) {
    await worker.renderFrame();
  }
  assert.ok(calls.length > 0, "the source fetched no tile");
  const [id, sourceId, refusal] = calls[0]!;
  assert.equal(id, map.id);
  assert.equal(sourceId, "custom");
  assert.ok(refusal instanceof MaplibreError, String(refusal));
  assert.equal(refusal.status, Status.invalidState);
  assert.equal(map.isClosed, false);
});

test("a drain before any demand returns no results", async () => {
  const map = await openRenderMap();
  const worker = await TestSession.offscreen(map);
  // Native writes a null batch, which reads as no results.
  assert.deepEqual(worker.session.drainFrameResults(), []);
});

test("a session disposed while it attaches rejects its ready without an unhandled rejection", async () => {
  const map = await openRenderMap();
  const session = attachCallerSession(map);
  session.dispose();
  // The disposal fails the attach before the map's release finishes, and a
  // later turn reports a rejection that nothing handled.
  await bounded(map.release(), "the map release");
  await turn();
  await assert.rejects(
    session.ready,
    (error: unknown) =>
      error instanceof MaplibreError && error.status === Status.targetLost,
  );
});

test("a caller-driven session is serviced from the event loop", async () => {
  const map = await openRenderMap();
  const caller = await TestSession.borrowed(map, "caller");

  // A borrowed texture has no readback, so the frame's result shows that the
  // session rendered it.
  const result = await caller.renderFrame();
  assert.ok(result.frameGeneration > 0n);
});

/** A session whose rendered frames this binding can acquire and view. */
async function frameSession(map: MapHandle) {
  const attached = await TestSession.borrowed(map);
  await attached.renderFrame();
  return attached.session;
}

test("a frame view expires with its scope and holds off release inside it", async () => {
  const map = await openRenderMap();
  const session = await frameSession(map);
  const frame = session.acquireFrame();
  assert.ok(frame, "the session exposed no frame");
  const done = { kind: GpuSyncKind.cpuComplete, handle: 0n, value: 0n };

  // Reads a view's width, a plain value, and the texture object it lends.
  type Reader = { width(): number; texture(): unknown };
  const withTexture = <T>(use: (view: Reader) => T): T =>
    backend === "metal"
      ? frame.withMetalTexture((view) =>
          use({ width: () => view.width, texture: () => view.texture }),
        )
      : backend === "vulkan"
        ? frame.withVulkanTexture((view) =>
            use({ width: () => view.width, texture: () => view.image }),
          )
        : backend === "webgpu"
          ? frame.withWebGpuTexture((view) =>
              use({ width: () => view.width, texture: () => view.texture }),
            )
          : frame.withOpenGlTexture((view) =>
              use({ width: () => view.width, texture: () => view.texture }),
            );
  let escaped: Reader | undefined;
  const width = withTexture((view) => {
    escaped = view;
    view.texture();
    // The view borrows the frame, so native refuses to release the frame or
    // abandon its session while the view is open.
    assert.throws(
      () => frame.release(done),
      (error) => error instanceof MaplibreError,
    );
    assert.throws(
      () => session.abandon(),
      (error) => error instanceof MaplibreError && error.status === Status.busy,
    );
    return view.width();
  });
  assert.equal(width, renderSize);
  // A view that escapes its scope refuses to read the texture it lends, and
  // still reads its plain values.
  assert.equal(escaped!.width(), renderSize);
  assert.throws(
    () => escaped!.texture(),
    (error) =>
      error instanceof MaplibreError && error.status === Status.invalidState,
  );

  frame.release(done);
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
  const session = attachSession(map, { frameWake });
  await bounded(session.ready, "the attachment");
  demandFrame(session, 1n);
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

test("a native queue lock reaches a session on a Vulkan device", async () => {
  const map = await openRenderMap();
  const probe = new NativeProbe();
  const queueLock = QueueLock.native({
    lock: probe.lock,
    unlock: probe.unlock,
    userData: probe.userData,
    releaseUserData: probe.release,
  });
  if (backend !== "vulkan") {
    // Only a Vulkan device takes a queue lock, and the binding releases the
    // context of the call that native code refuses.
    assert.throws(
      () => GpuDeviceHandle.create({ queueLock }),
      failsWith(Status.unsupported),
    );
    await probe.arrival("release");
    assert.deepEqual(probe.events, ["release"]);
    return;
  }
  const session = attachVulkanSession(map, queueLock);
  await bounded(session.ready, "the attachment");
  demandFrame(session, 1n);
  await probe.arrival("unlock");
  await bounded(session.detach(), "the detach");
  session.dispose();
  await probe.arrival("release");
  assert.equal(probe.events.at(-1), "release");
});
