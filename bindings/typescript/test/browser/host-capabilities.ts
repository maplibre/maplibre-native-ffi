// How the suite reaches each host capability in a browser page, which the
// browser bundle uses in place of test/support/host-capabilities.ts.

import {
  RenderBackendFlag,
  supportedRenderBackendMask,
} from "../../src/browser.ts";
import { native } from "../../src/internal/addon.ts";
import { WebObject } from "../../src/internal/web_objects.ts";
import type {
  Capabilities,
  Origin,
  OriginRequest,
  TestCanvas,
} from "../support/host-capabilities.ts";

// The suite checks against the Node types, which declare no page globals.
declare const location: { readonly origin: string };
declare const OffscreenCanvas: new (
  width: number,
  height: number,
) => {
  getContext(type: "webgl2", attributes: object): WebGl2 | null;
  getContext(type: "2d", attributes: object): Canvas2d | null;
};

/** What the fixtures call of an `OffscreenCanvasRenderingContext2D`. */
interface Canvas2d {
  drawImage(image: object, x: number, y: number): void;
  getImageData(
    x: number,
    y: number,
    width: number,
    height: number,
  ): { readonly data: Uint8ClampedArray };
}

/** What the fixtures call of a `WebGL2RenderingContext`. */
interface WebGl2 {
  readonly TEXTURE_2D: number;
  readonly RGBA8: number;
  createTexture(): object;
  bindTexture(target: number, texture: object | null): void;
  texStorage2D(
    target: number,
    levels: number,
    format: number,
    width: number,
    height: number,
  ): void;
  deleteTexture(texture: object): void;
}

/** A request to the page's server, which serves each origin under its path. */
async function control(
  method: string,
  path: string,
  body?: unknown,
): Promise<unknown> {
  const response = await fetch(`/__origins${path}`, {
    method,
    headers: { "content-type": "application/json" },
    ...(body === undefined ? {} : { body: JSON.stringify(body) }),
  });
  if (!response.ok) {
    throw new Error(`${method} /__origins${path}: HTTP ${response.status}`);
  }
  const text = await response.text();
  return text === "" ? undefined : (JSON.parse(text) as unknown);
}

/**
 * An origin that scripts/run-browser-test.mjs serves under the page's own
 * origin, which the module's fetches reach without a cross-origin request.
 */
async function httpOrigin(
  routes: Readonly<Record<string, string>>,
): Promise<Origin> {
  const { id } = (await control("POST", "", { routes })) as { id: number };
  return {
    url: `${location.origin}/__origins/${id}`,
    requests: async () =>
      (await control("GET", `/${id}/requests`)) as OriginRequest[],
    close: async () => {
      await control("DELETE", `/${id}`);
    },
  };
}

interface TestExports {
  nativeProbe(listener: (event: string) => void): unknown;
}

interface GpuTexture {
  destroy(): void;
}

// GPUTextureUsage.RENDER_ATTACHMENT, TEXTURE_BINDING, and COPY_SRC.
const textureUsage = 0x10 | 0x04 | 0x01;

/** The suite's native probe, which the test module adds to the addon's exports. */
function testExports(): TestExports {
  return (native as unknown as { testGraphics: TestExports }).testGraphics;
}

// Requesting a device is asynchronous, so the page requests one up front,
// which every WebGPU session of the run shares. A WebGL run leaves it unused.
const gpu = (navigator as { gpu?: { requestAdapter(): Promise<unknown> } }).gpu;
const adapter = (await gpu?.requestAdapter()) as
  | { requestDevice(): Promise<object> }
  | null
  | undefined;
const device = ((await adapter?.requestDevice()) ?? null) as {
  createTexture(descriptor: object): GpuTexture;
} | null;

/** The page's graphics objects, by key: a WebGL 2 context or the device. */
const graphics = new Map<
  bigint,
  { readonly lent: WebObject; readonly context?: WebGl2 }
>();
let nextGraphics = 1n;

/** The page's textures, by key, and how each one is destroyed. */
const textures = new Map<bigint, () => void>();
let nextTexture = 1n;

/**
 * The page's graphics objects, in the shape of the desktop graphics addon: a
 * WebGL 2 context over an OffscreenCanvas of its own, or the run's WebGPU
 * device, and RGBA textures on either, each lent to the module as a
 * WebObject. Sessions render into textures, so the canvas only carries the
 * context. WebGL has no current context: the module draws with the context
 * object that it holds.
 */
const testGraphics = {
  create(backend: number): object {
    const key = nextGraphics++;
    if (backend === 6) {
      if (device === null) {
        throw new Error("the browser provided no WebGPU device");
      }
      const lent = WebObject.of(device);
      graphics.set(key, { lent });
      return { graphics: key, webGpuDevice: lent.handle };
    }
    const context = new OffscreenCanvas(64, 64).getContext("webgl2", {
      depth: true,
      stencil: true,
      antialias: false,
      preserveDrawingBuffer: false,
    });
    if (context === null) {
      throw new Error("the browser refused a WebGL 2 context");
    }
    const lent = WebObject.of(context);
    graphics.set(key, { lent, context });
    return { graphics: key, webGlContext: lent.handle };
  },
  makeCurrent(_key: bigint): boolean {
    return true;
  },
  destroy(key: bigint): void {
    graphics.get(key)?.lent.close();
    graphics.delete(key);
  },
  textureCreate(key: bigint, width: number, height: number): object {
    const texture = nextTexture++;
    const { context } = graphics.get(key)!;
    if (context !== undefined) {
      const created = context.createTexture();
      context.bindTexture(context.TEXTURE_2D, created);
      context.texStorage2D(context.TEXTURE_2D, 1, context.RGBA8, width, height);
      context.bindTexture(context.TEXTURE_2D, null);
      const lent = WebObject.of(created);
      textures.set(texture, () => {
        lent.close();
        context.deleteTexture(created);
      });
      return {
        texture,
        width,
        height,
        format: context.RGBA8,
        openGlTexture: lent.handle,
        openGlTarget: context.TEXTURE_2D,
      };
    }
    const created = device!.createTexture({
      size: { width, height },
      format: "rgba8unorm",
      usage: textureUsage,
    });
    const lent = WebObject.of(created);
    textures.set(texture, () => {
      lent.close();
      created.destroy();
    });
    return { texture, width, height, webGpuTexture: lent.handle };
  },
  textureDestroy(texture: bigint): void {
    textures.get(texture)?.();
    textures.delete(texture);
  },
  nativeProbe(listener: (event: string) => void): unknown {
    return testExports().nativeProbe(listener);
  },
};

/**
 * A canvas that a session presents to, and a 2D canvas that its center pixel
 * is copied into to read.
 */
function webCanvas(size: number): TestCanvas {
  const canvas = new OffscreenCanvas(size, size);
  const scratch = new OffscreenCanvas(size, size).getContext("2d", {
    willReadFrequently: true,
  })!;
  const lent = WebObject.of(canvas);
  return {
    handle: lent.handle,
    centerPixel: () => {
      scratch.drawImage(canvas, 0, 0);
      const center = Math.floor(size / 2);
      return [...scratch.getImageData(center, center, 1, 1).data];
    },
    close: () => lent.close(),
  };
}

export const capabilities: Capabilities = {
  host: "browser",
  // A page starts no process and cannot end its own. Each Web Worker would
  // load a module of its own, with its own memory, which the suite does not
  // spawn.
  // mln-rs's browser build renders into canvases only, and webCanvas lends
  // the sessions theirs.
  unavailable: [
    "childProcess",
    "processExit",
    "realmSpawn",
    "borrowedTextures",
  ],
  httpOrigin,
  forcedGc: (globalThis as { gc?: () => void }).gc,
  bufferTransfer: (buffer, length) => void buffer.transfer(length),
  disposers: true,
  beforeLoad: () =>
    (globalThis as { mlnTsBeforeLoad?: unknown }).mlnTsBeforeLoad,
  testGraphics: () => testGraphics,
  // A browser module carries one backend, which the suite tests.
  testBackend: () =>
    (supportedRenderBackendMask() & RenderBackendFlag.webGpu) !== 0
      ? "webgpu"
      : "opengl",
  webCanvas,
  // Nothing holds a page open, so no timer needs to let go.
  unrefTimer: () => {},
};
