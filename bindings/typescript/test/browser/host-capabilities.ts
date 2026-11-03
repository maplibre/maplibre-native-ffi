// How the suite reaches each host capability in a browser page, which the
// browser bundle uses in place of test/support/host-capabilities.ts.

import { native } from "../../src/internal/addon.ts";
import {
  importWebGpuDevice,
  importWebGpuTexture,
  importWebGpuTextureView,
  registerWebGlContext,
  RegisteredWebGlContext,
  webGpuTextureFormat,
  type ImportedWebGpuObject,
} from "../../src/internal/web_graphics.ts";
import type {
  Capabilities,
  Origin,
  OriginRequest,
} from "../support/host-capabilities.ts";

// The suite checks against the Node types, which declare no page globals.
declare const location: { readonly origin: string };
declare const OffscreenCanvas: new (
  width: number,
  height: number,
) => { getContext(type: "webgl2", attributes: object): object | null };

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
  makeCurrent(context: number): boolean;
  webGlTextureCreate(width: number, height: number): number;
  webGlTextureDelete(texture: number): void;
}

interface GpuTexture {
  createView(): object;
  destroy(): void;
}

// GPUTextureUsage.RENDER_ATTACHMENT, TEXTURE_BINDING, and COPY_SRC.
const textureUsage = 0x10 | 0x04 | 0x01;

/** The test exports that test/browser/test_addon.c adds to the addon's. */
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

const graphics = new Map<
  bigint,
  RegisteredWebGlContext | ImportedWebGpuObject
>();
let nextGraphics = 1n;

/** The page's textures, by key, and how each one is destroyed. */
const textures = new Map<bigint, () => void>();
let nextTexture = 1n;

/**
 * The page's graphics objects, in the shape of the desktop graphics addon: a
 * WebGL 2 context over an OffscreenCanvas of its own, or an import of the
 * run's WebGPU device, and RGBA textures on either. Sessions render into
 * textures, so the canvas only carries the context.
 */
const testGraphics = {
  create(backend: number): object {
    const key = nextGraphics++;
    if (backend === 6) {
      if (device === null) {
        throw new Error("the browser provided no WebGPU device");
      }
      const imported = importWebGpuDevice(device);
      graphics.set(key, imported);
      return { graphics: key, webGpuDevice: imported.pointer };
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
    const registered = registerWebGlContext(context);
    graphics.set(key, registered);
    return { graphics: key, webGlContext: registered.context };
  },
  makeCurrent(key: bigint): boolean {
    const registered = graphics.get(key) as RegisteredWebGlContext;
    return testExports().makeCurrent(registered.context);
  },
  destroy(key: bigint): void {
    graphics.get(key)?.close();
    graphics.delete(key);
  },
  textureCreate(key: bigint, width: number, height: number): object {
    const texture = nextTexture++;
    const owner = graphics.get(key)!;
    if (owner instanceof RegisteredWebGlContext) {
      const context = owner.context;
      const exports = testExports();
      if (!exports.makeCurrent(context)) {
        throw new Error("the WebGL context could not be made current");
      }
      const name = exports.webGlTextureCreate(width, height);
      textures.set(texture, () => {
        if (exports.makeCurrent(context)) {
          exports.webGlTextureDelete(name);
        }
      });
      // GL_RGBA8 and GL_TEXTURE_2D.
      return {
        texture,
        width,
        height,
        format: 0x8058,
        openGlTexture: name,
        openGlTarget: 0x0de1,
      };
    }
    const created = device!.createTexture({
      size: { width, height },
      format: "rgba8unorm",
      usage: textureUsage,
    });
    const imported = importWebGpuTexture(created);
    const view = importWebGpuTextureView(created.createView());
    textures.set(texture, () => {
      view.close();
      imported.close();
      created.destroy();
    });
    return {
      texture,
      width,
      height,
      format: webGpuTextureFormat("rgba8unorm"),
      webGpuTexture: imported.pointer,
      webGpuTextureView: view.pointer,
    };
  },
  textureDestroy(texture: bigint): void {
    textures.get(texture)?.();
    textures.delete(texture);
  },
  nativeProbe(listener: (event: string) => void): unknown {
    return testExports().nativeProbe(listener);
  },
};

export const capabilities: Capabilities = {
  host: "browser",
  // A page starts no process and cannot end its own. Each Web Worker would
  // load a module of its own, with its own heap and threads, which the suite
  // does not spawn.
  unavailable: ["childProcess", "processExit", "realmSpawn"],
  httpOrigin,
  forcedGc: (globalThis as { gc?: () => void }).gc,
  bufferTransfer: (buffer, length) => void buffer.transfer(length),
  disposers: true,
  beforeLoad: () =>
    (globalThis as { mlnTsBeforeLoad?: unknown }).mlnTsBeforeLoad,
  testGraphics: () => testGraphics,
  // Nothing holds a page open, so no timer needs to let go.
  unrefTimer: () => {},
};
