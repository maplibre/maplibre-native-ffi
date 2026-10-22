// Graphics objects from tests/graphics, through the test graphics addon, and
// the render sessions that the suite attaches with them.

import {
  AwaitableWake,
  type LogicalExtent,
  type MapHandle,
  OpenglClientApi,
  OpenglContextOwnership,
  OpenglContextProviderFlag,
  openglSupportedContextProviderMask,
  RenderBackendFlag,
  RenderDriverKind,
  type RenderFrameResult,
  RenderResult,
  type RenderSessionAttachment,
  type RenderSessionAttachOptions,
  type RenderSessionHandle,
  supportedRenderBackendMask,
} from "../../src/index.ts";
import { bounded, nextWake, teardown, turn } from "./harness.ts";
import { capabilities } from "./host-capabilities.ts";

interface GraphicsContext {
  readonly graphics: bigint;
  readonly metalDevice: bigint;
  readonly eglDisplay: bigint;
  readonly eglConfig: bigint;
  readonly eglContext: bigint;
  readonly getProcAddress: bigint;
  readonly vulkanInstance: bigint;
  readonly vulkanPhysicalDevice: bigint;
  readonly vulkanDevice: bigint;
  readonly vulkanQueue: bigint;
  readonly vulkanQueueFamilyIndex: number;
  readonly vulkanGetInstanceProcAddr: bigint;
  readonly vulkanGetDeviceProcAddr: bigint;
}

interface GraphicsAddon {
  create(backend: number): GraphicsContext;
  makeCurrent(graphics: bigint): boolean;
  destroy(graphics: bigint): void;
}

const addon = capabilities.testGraphics() as GraphicsAddon;

/** The backend that this build renders through, by its test graphics number. */
const backends = { metal: 1, vulkan: 2, egl: 3 } as const;
export type Backend = keyof typeof backends;

function buildBackend(): Backend {
  const mask = supportedRenderBackendMask();
  if ((mask & RenderBackendFlag.metal) !== 0) {
    return "metal";
  }
  if ((mask & RenderBackendFlag.vulkan) !== 0) {
    return "vulkan";
  }
  const providers = openglSupportedContextProviderMask();
  if (
    (mask & RenderBackendFlag.opengl) !== 0 &&
    (providers & OpenglContextProviderFlag.egl) !== 0
  ) {
    return "egl";
  }
  throw new Error(
    `no test graphics backend serves render backend mask ${mask} with OpenGL context provider mask ${providers}`,
  );
}

export const backend = buildBackend();

export const renderSize = 32;
export const renderExtent: LogicalExtent = {
  width: renderSize,
  height: renderSize,
  scaleFactor: 1,
};

/** Creates the build's graphics object, which a teardown destroys. */
function openGraphics(): GraphicsContext {
  const context = addon.create(backends[backend]);
  teardown(() => addon.destroy(context.graphics));
  return context;
}

/**
 * Attaches an owned texture. On EGL, a core-worker session creates its own
 * dedicated context from the display and config, and a caller-driven session
 * shares the graphics object's context, which must be current on the calling
 * thread.
 */
function attachOwnedTexture(
  context: GraphicsContext,
  map: MapHandle,
  options: RenderSessionAttachOptions,
): RenderSessionAttachment {
  const extent = renderExtent;
  if (backend === "metal") {
    return map.attachMetalOwnedTexture(
      {
        extent,
        context: { device: context.metalDevice },
        // MTLPixelFormatBGRA8Unorm.
        pixelFormat: 80n,
      },
      options,
    );
  }
  if (backend === "vulkan") {
    return map.attachVulkanOwnedTexture(
      {
        extent,
        context: {
          instance: context.vulkanInstance,
          physicalDevice: context.vulkanPhysicalDevice,
          device: context.vulkanDevice,
          graphicsQueue: context.vulkanQueue,
          graphicsQueueFamilyIndex: context.vulkanQueueFamilyIndex,
          getInstanceProcAddr: context.vulkanGetInstanceProcAddr,
          getDeviceProcAddr: context.vulkanGetDeviceProcAddr,
        },
      },
      options,
    );
  }
  const dedicated = options.driver === RenderDriverKind.coreWorker;
  return map.attachOpenglOwnedTexture(
    {
      extent,
      context: {
        ownership: dedicated
          ? OpenglContextOwnership.dedicated
          : OpenglContextOwnership.shared,
        data: {
          kind: "egl",
          value: {
            display: context.eglDisplay,
            config: context.eglConfig,
            shareContext: dedicated ? undefined : context.eglContext,
            clientApi: OpenglClientApi.gles,
            getProcAddress: context.getProcAddress,
          },
        },
      },
    },
    options,
  );
}

/**
 * Abandons a session, then closes it. A session that the case detached,
 * abandoned, or closed already has nothing left to abandon, and a second close
 * does nothing.
 */
function releaseSession(session: RenderSessionHandle): void {
  try {
    session.abandon();
  } catch {
    // Native code reports what the session has already given up.
  }
  session.close();
}

/** Demands a frame, which renders whether or not the map changed. */
export function requestFrame(session: RenderSessionHandle, token: bigint) {
  session.requestFrame({ token });
}

/** The frame result for `token` among the session's drained results. */
function takeResult(
  session: RenderSessionHandle,
  token: bigint,
): RenderFrameResult | undefined {
  for (
    let batch = session.drainFrameResults();
    batch;
    batch = session.drainFrameResults()
  ) {
    try {
      const result = batch.get().results.find((item) => item.token === token);
      if (result !== undefined) {
        return result;
      }
    } finally {
      batch.close();
    }
  }
  return undefined;
}

/** How many frame demands a case makes before it fails for want of a frame. */
const frameAttempts = 8;

/**
 * An owned-texture session that renders on demand, and the wake it waits on.
 * A core worker drives a worker session. This thread drives a caller session
 * and services it between waits; the JavaScript thread is a fixed
 * operating-system thread, so the session's graphics-thread identity and the
 * current context hold for every service call.
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

  /** Attaches a session on `map`, released by a teardown. */
  static async attach(
    map: MapHandle,
    driver: "worker" | "caller" = "worker",
  ): Promise<TestSession> {
    const context = openGraphics();
    const serviced = driver === "caller";
    if (serviced && backend === "egl" && !addon.makeCurrent(context.graphics)) {
      throw new Error("the test graphics context could not be made current");
    }
    const wake = new AwaitableWake();
    const { session, completed } = attachOwnedTexture(context, map, {
      driver: serviced
        ? RenderDriverKind.callerGraphicsThread
        : RenderDriverKind.coreWorker,
      requestedTextureRingDepth: 1,
      frameWake: wake.wake,
      driverWorkWake: serviced ? wake.wake : undefined,
    });
    teardown(() => releaseSession(session));
    const attached = new TestSession(session, wake, serviced);
    await attached.settle(completed, "the attachment");
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

  /** Demands frames until one renders, and returns its result. */
  async renderFrame(): Promise<RenderFrameResult> {
    for (let attempt = 0; attempt < frameAttempts; attempt++) {
      const token = ++this.#token;
      requestFrame(this.session, token);
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

  /** Waits on the wake until `settled` returns a value. */
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
