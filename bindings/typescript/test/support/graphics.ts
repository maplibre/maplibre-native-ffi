// Graphics objects and textures from tests/graphics, through the test graphics
// addon, the render sessions that the suite attaches offscreen or with them,
// and the native code that fills a native registration.

import {
  AwaitableWake,
  FrameDemandFlag,
  type LogicalExtent,
  type LogicalSize,
  type MapHandle,
  OpenGlClientApi,
  type OpenGlContextDescriptorData,
  OpenGlContextOwnership,
  OpenGlContextProviderFlag,
  openGlSupportedContextProviderMask,
  RenderBackendFlag,
  RenderDriverKind,
  type RenderFrameResult,
  RenderResult,
  type RenderSessionAttachOptions,
  type RenderSessionHandle,
  supportedRenderBackendMask,
} from "../../src/index.ts";
import {
  bounded,
  callbackArrival,
  nextWake,
  teardown,
  turn,
} from "./harness.ts";
import { capabilities } from "./host-capabilities.ts";

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
  readonly webGpuDevice: bigint;
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
 * descriptor names. Only the fields of the build's backend are set.
 */
interface TestTexture {
  readonly texture: bigint;
  readonly vulkanImage: bigint;
  readonly vulkanImageView: bigint;
  readonly metalTexture: bigint;
  readonly webGpuTexture: bigint;
  readonly webGpuTextureView: bigint;
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
 * The backend that this build renders through, by its test graphics number.
 * The browser's test graphics create a WebGL context or import a WebGPU
 * device from the page for the browser's two.
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

function buildBackend(): Backend {
  const mask = supportedRenderBackendMask();
  if ((mask & RenderBackendFlag.metal) !== 0) {
    return "metal";
  }
  if ((mask & RenderBackendFlag.vulkan) !== 0) {
    return "vulkan";
  }
  if ((mask & RenderBackendFlag.webGpu) !== 0) {
    return "webgpu";
  }
  const providers = openGlSupportedContextProviderMask();
  if (
    (mask & RenderBackendFlag.openGl) !== 0 &&
    (providers & OpenGlContextProviderFlag.wgl) !== 0
  ) {
    return "wgl";
  }
  if (
    (mask & RenderBackendFlag.openGl) !== 0 &&
    (providers & OpenGlContextProviderFlag.webGl) !== 0
  ) {
    return "webgl";
  }
  if (
    (mask & RenderBackendFlag.openGl) !== 0 &&
    (providers & OpenGlContextProviderFlag.egl) !== 0
  ) {
    return "egl";
  }
  throw new Error(
    `no test graphics backend serves render backend mask ${mask} with OpenGL context provider mask ${providers}`,
  );
}

export const backend = buildBackend();

/**
 * Browser texture targets take only the caller-graphics-thread driver, so this
 * thread services every borrowed session there.
 */
const browser = backend === "webgl" || backend === "webgpu";

export const renderSize = 32;
export const renderLogicalSize = {
  width: renderSize,
  height: renderSize,
} satisfies LogicalSize;
export const renderExtent: LogicalExtent = {
  ...renderLogicalSize,
  scaleFactor: 1,
};

/** Creates the build's graphics object, which a teardown destroys. */
function openGraphics(): GraphicsContext {
  const context = addon.create(backends[backend]);
  teardown(() => addon.destroy(context.graphics));
  return context;
}

/**
 * Attaches a texture that the test graphics create, which a teardown destroys,
 * so release the session in a teardown registered after this call. An OpenGL
 * session shares the graphics object's context, which must be current on the
 * calling thread.
 */
function attachBorrowedTexture(
  context: GraphicsContext,
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
  if (backend === "webgpu") {
    // TODO(part 2 of the move off Emscripten): the browser fixtures still
    // describe Emscripten's WebGPU handles, where the descriptor now takes
    // `mln_web_object` handles and a `GPUTextureFormat` name.
    return map.attachWebGpuBorrowedTexture(
      {
        ...ring,
        context: { device: Number(context.webGpuDevice) },
        textures: [{ texture: Number(texture.webGpuTexture) }],
        format: "rgba8unorm",
      },
      options,
    );
  }
  if (backend === "metal") {
    return map.attachMetalBorrowedTexture(
      {
        ...ring,
        textures: [{ texture: texture.metalTexture }],
      },
      options,
    );
  }
  if (backend === "vulkan") {
    return map.attachVulkanBorrowedTexture(
      {
        ...ring,
        context: {
          instance: context.vulkanInstance,
          physicalDevice: context.vulkanPhysicalDevice,
          device: context.vulkanDevice,
          graphicsQueue: context.vulkanQueue,
          graphicsQueueFamilyIndex: context.vulkanQueueFamilyIndex,
          getInstanceProcAddr: context.vulkanGetInstanceProcAddr,
          getDeviceProcAddr: context.vulkanGetDeviceProcAddr,
        },
        textures: [
          { image: texture.vulkanImage, imageView: texture.vulkanImageView },
        ],
        format: texture.format,
        finalLayout: texture.vulkanFinalLayout,
      },
      options,
    );
  }
  const data: OpenGlContextDescriptorData =
    backend === "webgl"
      ? {
          kind: "webGl",
          value: { context: context.webGlContext },
        }
      : backend === "wgl"
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
              clientApi: OpenGlClientApi.gles,
              getProcAddress: context.getProcAddress,
            },
          };
  return map.attachOpenGlBorrowedTexture(
    {
      ...ring,
      context: { ownership: OpenGlContextOwnership.shared, data },
      textures: [{ texture: texture.openGlTexture }],
      target: texture.openGlTarget,
    },
    options,
  );
}

/**
 * Attaches an offscreen session that a core worker drives, with `options` in
 * place of the defaults, and releases the session in a teardown.
 */
export function attachSession(
  map: MapHandle,
  options: Partial<RenderSessionAttachOptions>,
): RenderSessionHandle {
  const session = map.attachOffscreen(
    { logicalSize: renderLogicalSize },
    { driver: RenderDriverKind.coreWorker, ...options },
  );
  teardown(() => releaseSession(session));
  return session;
}

/**
 * Attaches a borrowed Vulkan texture that a core worker drives, with `options`
 * in place of the defaults, and releases the session in a teardown.
 */
export function attachVulkanSession(
  map: MapHandle,
  options: Partial<RenderSessionAttachOptions>,
): RenderSessionHandle {
  if (backend !== "vulkan") {
    throw new Error(`a ${backend} build has no Vulkan textures`);
  }
  const session = attachBorrowedTexture(openGraphics(), map, {
    driver: RenderDriverKind.coreWorker,
    ...options,
  });
  teardown(() => releaseSession(session));
  return session;
}

/**
 * Starts attaching a borrowed texture that this thread drives, and releases
 * the session in a teardown. The attach finishes only as this thread services
 * the session.
 */
export function attachCallerSession(map: MapHandle): RenderSessionHandle {
  const context = openGraphics();
  const openGl = backend === "egl" || backend === "wgl" || backend === "webgl";
  if (openGl && !addon.makeCurrent(context.graphics)) {
    throw new Error("the test graphics context could not be made current");
  }
  const session = attachBorrowedTexture(context, map, {
    driver: RenderDriverKind.callerGraphicsThread,
  });
  teardown(() => releaseSession(session));
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
 */
function releaseSession(session: RenderSessionHandle): void {
  try {
    session.abandon();
  } catch {
    // Native code reports what the session has already given up.
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

  private constructor(
    session: RenderSessionHandle,
    wake: AwaitableWake,
    serviced: boolean,
  ) {
    this.session = session;
    this.wake = wake;
    this.#serviced = serviced;
  }

  /** Attaches an offscreen worker session on `map`, released by a teardown. */
  static async offscreen(map: MapHandle): Promise<TestSession> {
    const wake = new AwaitableWake();
    const session = attachSession(map, {
      frameWake: { callback: wake.wake },
    });
    const attached = new TestSession(session, wake, false);
    await attached.settle(session.ready, "the attachment");
    return attached;
  }

  /**
   * Attaches a texture of the test graphics on `map`, released by a teardown.
   * Browser and OpenGL textures take only a caller session, whatever `driver`
   * asks for.
   */
  static async borrowed(
    map: MapHandle,
    driver: "worker" | "caller" = "worker",
  ): Promise<TestSession> {
    const context = openGraphics();
    const openGl =
      backend === "egl" || backend === "wgl" || backend === "webgl";
    const serviced = driver === "caller" || browser || openGl;
    if (openGl && !addon.makeCurrent(context.graphics)) {
      throw new Error("the test graphics context could not be made current");
    }
    const wake = new AwaitableWake();
    const session = attachBorrowedTexture(context, map, {
      driver: serviced
        ? RenderDriverKind.callerGraphicsThread
        : RenderDriverKind.coreWorker,
      frameWake: { callback: wake.wake },
      driverWorkWake: serviced ? { callback: wake.wake } : undefined,
    });
    teardown(() => releaseSession(session));
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
export function centerPixel(image: {
  readonly data: Uint8Array;
  readonly info: { readonly width: number; readonly stride: number };
}): number[] {
  const center =
    Math.floor(image.info.width / 2) * image.info.stride +
    Math.floor(image.info.width / 2) * 4;
  return [...image.data.subarray(center, center + 4)];
}
