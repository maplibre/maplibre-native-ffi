// Graphics objects and textures from tests/graphics, through the test graphics
// addon, the render sessions that the suite attaches offscreen or with them,
// and the native code that fills a native registration.

import {
  AwaitableWake,
  FrameDemandFlag,
  GpuDeviceHandle,
  type LogicalExtent,
  type LogicalSize,
  type MapHandle,
  type OpenGlContextDescriptor,
  OpenGlContextProviderFlag,
  openGlSupportedContextProviderMask,
  RenderBackend,
  RenderBackendFlag,
  RenderDriverKind,
  type QueueLock,
  type RenderFrameResult,
  RenderResult,
  type RenderSessionAttachOptions,
  type RenderSessionHandle,
  type RenderTarget,
  supportedRenderBackendMask,
} from "../../src/index.ts";
import {
  bounded,
  callbackArrival,
  nextWake,
  teardown,
  turn,
} from "./harness.ts";
import { capabilities, type TestCanvas } from "./host-capabilities.ts";

interface GraphicsContext {
  readonly graphics: bigint;
  readonly metalDevice: bigint;
  readonly eglDisplay: bigint;
  readonly eglConfig: bigint;
  readonly eglContext: bigint;
  readonly wglDeviceContext: bigint;
  readonly wglContext: bigint;
  readonly getProcAddress: bigint;
  readonly vulkanInstance: bigint;
  readonly vulkanPhysicalDevice: bigint;
  readonly vulkanDevice: bigint;
  readonly vulkanQueue: bigint;
  readonly vulkanQueueFamilyIndex: number;
  readonly vulkanGetInstanceProcAddr: bigint;
  readonly vulkanGetDeviceProcAddr: bigint;
  readonly webGlContext: number;
  readonly webGpuDevice: number;
}

interface ProbeAddresses {
  readonly wake: bigint;
  readonly lock: bigint;
  readonly unlock: bigint;
  readonly release: bigint;
  readonly userData: bigint;
}

/**
 * A texture from the test graphics, and the handles that a borrowed texture
 * descriptor names. Only the fields of the backend under test are set.
 */
interface TestTexture {
  readonly texture: bigint;
  readonly vulkanImage: bigint;
  readonly vulkanImageView: bigint;
  readonly metalTexture: bigint;
  readonly webGpuTexture: number;
  readonly width: number;
  readonly height: number;
  readonly format: number;
  readonly vulkanFinalLayout: number;
  readonly openGlTexture: number;
  readonly openGlTarget: number;
}

interface GraphicsAddon {
  create(backend: number): GraphicsContext;
  makeCurrent(graphics: bigint): boolean;
  destroy(graphics: bigint): void;
  textureCreate(graphics: bigint, width: number, height: number): TestTexture;
  textureDestroy(texture: bigint): void;
  nativeProbe(listener: (event: string) => void): ProbeAddresses;
}

const addon = capabilities.testGraphics() as GraphicsAddon;

/**
 * The test graphics number of each backend, with OpenGL named by its context
 * provider. The browser's test graphics create a WebGL context or lend the
 * page's WebGPU device for the browser's two.
 */
const backends = {
  metal: 1,
  vulkan: 2,
  egl: 3,
  wgl: 4,
  webgl: 5,
  webgpu: 6,
} as const;
export type Backend = keyof typeof backends;

/** The library's flag for each backend that a test task can name. */
const backendFlags: Readonly<Record<string, RenderBackendFlag>> = {
  metal: RenderBackendFlag.metal,
  vulkan: RenderBackendFlag.vulkan,
  opengl: RenderBackendFlag.openGl,
  webgpu: RenderBackendFlag.webGpu,
};

/**
 * The backend under test, which the host names, with OpenGL named by the
 * library's context provider. A library carries several backends, so a
 * session draws with this one rather than the library's default.
 */
function testBackend(): Backend {
  const named = capabilities.testBackend();
  if (!named) {
    throw new Error("MLN_FFI_TEST_BACKEND must name the backend under test");
  }
  const flag = backendFlags[named];
  if (flag === undefined) {
    throw new Error(`${named} is no render backend`);
  }
  if ((supportedRenderBackendMask() & flag) === 0) {
    throw new Error(`the library lacks the ${named} backend under test`);
  }
  if (named !== "opengl") {
    return named as Backend;
  }
  const providers = openGlSupportedContextProviderMask();
  if ((providers & OpenGlContextProviderFlag.wgl) !== 0) {
    return "wgl";
  }
  if ((providers & OpenGlContextProviderFlag.webGl) !== 0) {
    return "webgl";
  }
  if ((providers & OpenGlContextProviderFlag.egl) !== 0) {
    return "egl";
  }
  throw new Error(
    `no test graphics backend serves OpenGL context provider mask ${providers}`,
  );
}

let chosen: Backend | undefined;

/**
 * The backend under test. The host may learn it only once the suite runs,
 * so the first case to need it reads it.
 */
export function backend(): Backend {
  chosen ??= testBackend();
  return chosen;
}

/** The library's value for the backend under test. */
export function renderBackend(): RenderBackend {
  switch (backend()) {
    case "metal":
      return RenderBackend.metal;
    case "vulkan":
      return RenderBackend.vulkan;
    case "webgpu":
      return RenderBackend.webGpu;
    default:
      return RenderBackend.openGl;
  }
}

/** Whether the backend under test draws with OpenGL. */
function drawsWithOpenGl(): boolean {
  const current = backend();
  return current === "egl" || current === "wgl" || current === "webgl";
}

/**
 * mln-rs's browser build renders into canvases only, so a browser's sessions
 * present to canvases of the page's, which the host's webCanvas lends, and
 * the page reads their pixels.
 */
const browser = capabilities.host === "browser";

export const renderSize = 32;
export const renderLogicalSize = {
  width: renderSize,
  height: renderSize,
} satisfies LogicalSize;
export const renderExtent: LogicalExtent = {
  ...renderLogicalSize,
  scaleFactor: 1,
};

/** Creates a graphics object of the backend under test, which a teardown destroys. */
function openGraphics(): GraphicsContext {
  const context = addon.create(backends[backend()]);
  teardown(() => addon.destroy(context.graphics));
  return context;
}

/**
 * Wraps the graphics object's device, or its OpenGL context, in a GPU device
 * that a teardown releases. Vulkan takes `queueLock`, the lock on the queue
 * that the graphics object submits to.
 */
function wrapDevice(
  context: GraphicsContext,
  queueLock?: QueueLock,
): GpuDeviceHandle {
  let device: GpuDeviceHandle;
  const current = backend();
  if (current === "webgpu") {
    device = GpuDeviceHandle.wrapWebGpu({ device: context.webGpuDevice });
  } else if (current === "metal") {
    device = GpuDeviceHandle.wrapMetal({ device: context.metalDevice });
  } else if (current === "vulkan") {
    device = GpuDeviceHandle.wrapVulkan({
      context: {
        instance: context.vulkanInstance,
        physicalDevice: context.vulkanPhysicalDevice,
        device: context.vulkanDevice,
        graphicsQueue: context.vulkanQueue,
        graphicsQueueFamilyIndex: context.vulkanQueueFamilyIndex,
        getInstanceProcAddr: context.vulkanGetInstanceProcAddr,
        getDeviceProcAddr: context.vulkanGetDeviceProcAddr,
      },
      queueLock,
    });
  } else {
    const openGl: OpenGlContextDescriptor =
      current === "webgl"
        ? { kind: "webGl", value: { context: context.webGlContext } }
        : current === "wgl"
          ? {
              kind: "wgl",
              value: {
                deviceContext: context.wglDeviceContext,
                shareContext: context.wglContext,
                getProcAddress: context.getProcAddress,
              },
            }
          : {
              kind: "egl",
              value: {
                display: context.eglDisplay,
                config: context.eglConfig,
                shareContext: context.eglContext,
                getProcAddress: context.getProcAddress,
              },
            };
    device = GpuDeviceHandle.wrapOpenGl(openGl);
  }
  teardown(() => device.release());
  return device;
}

/**
 * Attaches a texture that the test graphics create on `device`, which a
 * teardown destroys, so release the session in a teardown registered after
 * this call. An OpenGL session shares the graphics object's context, which
 * must be current on the calling thread.
 */
function attachBorrowedTexture(
  context: GraphicsContext,
  device: GpuDeviceHandle,
  map: MapHandle,
  options: RenderSessionAttachOptions,
): RenderSessionHandle {
  const texture = addon.textureCreate(context.graphics, renderSize, renderSize);
  teardown(() => addon.textureDestroy(texture.texture));
  const ring = {
    logicalSize: renderLogicalSize,
    physicalWidth: texture.width,
    physicalHeight: texture.height,
  };
  let target: RenderTarget;
  const current = backend();
  if (current === "webgpu") {
    target = {
      kind: "webGpuBorrowedTextures",
      value: {
        ...ring,
        textures: [{ texture: texture.webGpuTexture }],
        format: "rgba8unorm",
      },
    };
  } else if (current === "metal") {
    target = {
      kind: "metalBorrowedTextures",
      value: { ...ring, textures: [{ texture: texture.metalTexture }] },
    };
  } else if (current === "vulkan") {
    target = {
      kind: "vulkanBorrowedTextures",
      value: {
        ...ring,
        textures: [
          { image: texture.vulkanImage, imageView: texture.vulkanImageView },
        ],
        format: texture.format,
        finalLayout: texture.vulkanFinalLayout,
      },
    };
  } else {
    target = {
      kind: "openGlBorrowedTextures",
      value: {
        ...ring,
        textures: [{ texture: texture.openGlTexture }],
        target: texture.openGlTarget,
      },
    };
  }
  return map.attach(target, device, options);
}

/**
 * The target of a session on a device of its own: offscreen, or in a browser
 * a canvas, which a teardown closes.
 */
function ownTarget(): { target: RenderTarget; canvas?: TestCanvas } {
  if (!browser) {
    return {
      target: { kind: "offscreen", value: { logicalSize: renderLogicalSize } },
    };
  }
  const canvas = capabilities.webCanvas!(renderSize);
  teardown(() => canvas.close());
  return {
    target: {
      kind: "webCanvas",
      value: { logicalSize: renderLogicalSize, canvas: canvas.handle },
    },
    canvas,
  };
}

/**
 * Attaches `target` on a device of the backend under test, which the session
 * keeps until it ends. A browser module carries one backend, whose device
 * the session creates.
 */
function attachOwn(
  map: MapHandle,
  target: RenderTarget,
  options: RenderSessionAttachOptions,
): RenderSessionHandle {
  if (browser) {
    return map.attach(target, undefined, options);
  }
  const device = GpuDeviceHandle.create({ backend: renderBackend() });
  try {
    return map.attach(target, device, options);
  } finally {
    device.release();
  }
}

/**
 * Attaches an offscreen session, or in a browser a canvas, that a core worker
 * drives, with `options` in place of the defaults, and releases the session
 * in a teardown.
 */
export function attachSession(
  map: MapHandle,
  options: Partial<RenderSessionAttachOptions>,
): RenderSessionHandle {
  const session = attachOwn(map, ownTarget().target, {
    driver: RenderDriverKind.coreWorker,
    ...options,
  });
  teardown(() => releaseSession(session));
  return session;
}

/**
 * Attaches a borrowed Vulkan texture that a core worker drives, on a device
 * that wraps the test graphics' device with `queueLock`, which the session
 * alone holds, and releases the session in a teardown.
 */
export function attachVulkanSession(
  map: MapHandle,
  queueLock: QueueLock,
): RenderSessionHandle {
  if (backend() !== "vulkan") {
    throw new Error(`a ${backend()} session has no Vulkan textures`);
  }
  const context = openGraphics();
  const device = wrapDevice(context, queueLock);
  const session = attachBorrowedTexture(context, device, map, {
    driver: RenderDriverKind.coreWorker,
  });
  // The session keeps the device, and the device the queue lock, until the
  // session ends.
  device.release();
  teardown(() => releaseSession(session));
  return session;
}

/**
 * Starts attaching a borrowed texture, or in a browser a canvas, that this
 * thread drives, and releases the session in a teardown. The attach finishes
 * only as this thread services the session.
 */
export function attachCallerSession(map: MapHandle): RenderSessionHandle {
  if (browser) {
    const session = map.attach(ownTarget().target, undefined, {
      driver: RenderDriverKind.callerGraphicsThread,
    });
    teardown(() => releaseSession(session, true));
    return session;
  }
  const context = openGraphics();
  if (drawsWithOpenGl() && !addon.makeCurrent(context.graphics)) {
    throw new Error("the test graphics context could not be made current");
  }
  const session = attachBorrowedTexture(context, wrapDevice(context), map, {
    driver: RenderDriverKind.callerGraphicsThread,
  });
  teardown(() => releaseSession(session, true));
  return session;
}

/**
 * Native code that a host brings, from the test graphics addon: the function
 * pointers and the context of a native registration. Each call that native
 * code makes through them reaches this realm's event loop as an event.
 */
export class NativeProbe implements ProbeAddresses {
  readonly wake: bigint;
  readonly lock: bigint;
  readonly unlock: bigint;
  readonly release: bigint;
  readonly userData: bigint;
  /** The names of the calls that have arrived, in order. */
  readonly events: string[] = [];
  readonly #waiters = new Set<{ event: string; resolve: () => void }>();

  constructor() {
    const addresses = addon.nativeProbe((event) => {
      this.events.push(event);
      for (const waiter of this.#waiters) {
        if (waiter.event === event) {
          this.#waiters.delete(waiter);
          waiter.resolve();
        }
      }
    });
    this.wake = addresses.wake;
    this.lock = addresses.lock;
    this.unlock = addresses.unlock;
    this.release = addresses.release;
    this.userData = addresses.userData;
  }

  /** Resolves once native code has made a call named `event`. */
  arrival(event: string): Promise<void> {
    if (this.events.includes(event)) {
      return Promise.resolve();
    }
    return callbackArrival(
      new Promise<void>((resolve) => this.#waiters.add({ event, resolve })),
      `the native ${event}`,
    );
  }
}

/**
 * Abandons a session, then disposes it. A session that the case detached,
 * abandoned, or disposed already has nothing left to abandon, and a second
 * dispose does nothing.
 *
 * A session that this thread services keeps its graphics objects after the
 * abandon until its next service, so a `serviced` session is serviced once
 * more before the teardowns that destroy the device it renders on.
 */
function releaseSession(session: RenderSessionHandle, serviced = false): void {
  try {
    session.abandon();
  } catch {
    // Native code reports what the session has already given up.
  }
  if (serviced) {
    try {
      session.serviceDriverWork();
    } catch {
      // The service of an abandoned session reports that it ended.
    }
  }
  session.dispose();
}

/**
 * Demands a frame with the default flags, which render only when the map has
 * an update newer than the session's last frame.
 */
export function demandFrame(session: RenderSessionHandle, token: bigint) {
  session.demandFrame({ token });
}

function takeResult(
  session: RenderSessionHandle,
  token: bigint,
): RenderFrameResult | undefined {
  for (
    let results = session.drainFrameResults();
    results.length > 0;
    results = session.drainFrameResults()
  ) {
    const result = results.find((item) => item.token === token);
    if (result !== undefined) {
      return result;
    }
  }
  return undefined;
}

/**
 * How many frame demands a case makes before it fails for want of a frame. A
 * demand waits for the map's next update, so only a target that is not ready
 * ends one without a frame.
 */
const frameAttempts = 8;

/**
 * A session that renders on demand, and the wake it waits on. A core worker
 * drives a worker session. This thread drives a caller session and services it
 * between waits; the JavaScript thread is a fixed operating-system thread, so
 * the session's graphics-thread identity and the current context hold for
 * every service call.
 */
export class TestSession {
  readonly session: RenderSessionHandle;
  readonly wake: AwaitableWake;
  readonly #serviced: boolean;
  #token = 0n;
  /** A browser canvas's center pixel, as the last frame left it. */
  #pixel: number[] | undefined;

  private constructor(
    session: RenderSessionHandle,
    wake: AwaitableWake,
    serviced: boolean,
  ) {
    this.session = session;
    this.wake = wake;
    this.#serviced = serviced;
  }

  /**
   * Attaches a session on a device of its own, which `driver` drives:
   * offscreen, or in a browser a canvas, whose center pixel each frame wake
   * reads in the task that presented the frame. A teardown releases it.
   */
  static async #own(
    map: MapHandle,
    driver: RenderDriverKind,
  ): Promise<TestSession> {
    const wake = new AwaitableWake();
    const { target, canvas } = ownTarget();
    let attached: TestSession | undefined;
    const serviced = driver === RenderDriverKind.callerGraphicsThread;
    const session = attachOwn(map, target, {
      driver,
      frameWake: {
        callback: () => {
          if (canvas !== undefined && attached !== undefined) {
            attached.#pixel = canvas.centerPixel();
          }
          wake.wake();
        },
      },
      driverWorkWake: serviced ? { callback: wake.wake } : undefined,
    });
    teardown(() => releaseSession(session, serviced));
    attached = new TestSession(session, wake, serviced);
    await attached.settle(session.ready, "the attachment");
    return attached;
  }

  /**
   * Attaches an offscreen worker session on `map`, or in a browser a canvas,
   * released by a teardown.
   */
  static offscreen(map: MapHandle): Promise<TestSession> {
    return TestSession.#own(map, RenderDriverKind.coreWorker);
  }

  /**
   * Attaches a session that this thread services: a texture of the test
   * graphics, or in a browser a canvas. A teardown releases it.
   */
  static serviced(map: MapHandle): Promise<TestSession> {
    return browser
      ? TestSession.#own(map, RenderDriverKind.callerGraphicsThread)
      : TestSession.borrowed(map, "caller");
  }

  /**
   * The RGBA bytes of the center pixel of the last frame: a readback, or in
   * a browser what the canvas showed.
   */
  async centerPixel(): Promise<number[]> {
    if (browser) {
      if (this.#pixel === undefined) {
        throw new Error("the canvas presented no frame");
      }
      return this.#pixel;
    }
    const image = await this.settle(this.session.readTexture(), "the readback");
    if (image.info.width !== renderSize || image.info.height !== renderSize) {
      throw new Error(
        `the readback is ${image.info.width}x${image.info.height}, not ${renderSize}x${renderSize}`,
      );
    }
    return centerPixel(image);
  }

  /**
   * Attaches a texture of the test graphics on `map`, released by a teardown,
   * for a host that has `borrowedTextures`. OpenGL textures take only a
   * caller session, whatever `driver` asks for.
   */
  static async borrowed(
    map: MapHandle,
    driver: "worker" | "caller" = "worker",
  ): Promise<TestSession> {
    const context = openGraphics();
    const serviced = driver === "caller" || drawsWithOpenGl();
    if (drawsWithOpenGl() && !addon.makeCurrent(context.graphics)) {
      throw new Error("the test graphics context could not be made current");
    }
    const wake = new AwaitableWake();
    const session = attachBorrowedTexture(context, wrapDevice(context), map, {
      driver: serviced
        ? RenderDriverKind.callerGraphicsThread
        : RenderDriverKind.coreWorker,
      frameWake: { callback: wake.wake },
      driverWorkWake: serviced ? { callback: wake.wake } : undefined,
    });
    teardown(() => releaseSession(session, serviced));
    const attached = new TestSession(session, wake, serviced);
    await attached.settle(session.ready, "the attachment");
    return attached;
  }

  /** Waits until `promise` settles, servicing the session if this thread drives it. */
  async settle<T>(promise: Promise<T>, what: string): Promise<T> {
    if (!this.#serviced) {
      return bounded(promise, what);
    }
    // A completion arrives through the realm's queue rather than the
    // session's wake, so settling also withdraws the wait for a wake.
    const settled = new AbortController();
    let outcome: PromiseSettledResult<T> | undefined;
    promise.then(
      (value) => {
        outcome = { status: "fulfilled", value };
        settled.abort();
      },
      (reason: unknown) => {
        outcome = { status: "rejected", reason };
        settled.abort();
      },
    );
    const result = await this.#until(() => outcome, what, settled.signal);
    if (result.status === "rejected") {
      throw result.reason;
    }
    return result.value;
  }

  /**
   * Demands frames until one renders, and returns its result. A demand made
   * when the map has nothing newer than the last frame waits for its next
   * update, such as the one that a source publishes as it loads.
   */
  async renderFrame(): Promise<RenderFrameResult> {
    for (let attempt = 0; attempt < frameAttempts; attempt++) {
      const token = ++this.#token;
      this.session.demandFrame({
        token,
        flags: FrameDemandFlag.waitForUpdate,
      });
      const result = await this.#until(
        () => takeResult(this.session, token),
        `frame ${token}`,
      );
      if (this.#serviced) {
        // Service renders on this thread, so let the callbacks that the frame
        // raised reach the event loop, as they do while a core worker renders.
        await turn();
      }
      if (result.disposition === RenderResult.rendered) {
        return result;
      }
    }
    throw new Error(`${frameAttempts} frame demands rendered nothing`);
  }

  async #until<T>(
    settled: () => T | undefined,
    what: string,
    signal?: AbortSignal,
  ): Promise<T> {
    for (;;) {
      if (this.#serviced) {
        this.session.serviceDriverWork();
      }
      const value = settled();
      if (value !== undefined) {
        return value;
      }
      await nextWake(this.wake, what, { signal }).catch((error: unknown) => {
        if (!signal?.aborted) {
          throw error;
        }
      });
    }
  }
}

/** The RGBA bytes of the center pixel of a tightly sized readback. */
function centerPixel(image: {
  readonly data: Uint8Array;
  readonly info: { readonly width: number; readonly stride: number };
}): number[] {
  const center =
    Math.floor(image.info.width / 2) * image.info.stride +
    Math.floor(image.info.width / 2) * 4;
  return [...image.data.subarray(center, center + 4)];
}
