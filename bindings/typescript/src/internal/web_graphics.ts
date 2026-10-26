// Imports a page's graphics objects into the browser module.
//
// Render target descriptors name WebGPU objects and WebGL contexts by handles
// in the module's own tables, and only the module can create those handles.
// The module of each backend links the graphics library of
// bindings/typescript/native and exports it as a property of its own.

import { Status } from "../generated_values.ts";
import { MaplibreError } from "./errors.ts";
import { ADOPT, type AdoptKey, checkKey } from "./owners.ts";

// The libraries' functions are arrow functions, which close over the module.
interface WebGpuLibrary {
  readonly importDevice: (device: object) => number;
  readonly importSurface: (context: object) => number;
  readonly importTexture: (texture: object) => number;
  readonly importTextureView: (view: object) => number;
  readonly releaseDevice: (handle: number) => void;
  readonly releaseSurface: (handle: number) => void;
  readonly releaseTexture: (handle: number) => void;
  readonly releaseTextureView: (handle: number) => void;
  readonly object: (handle: number) => object | null;
  readonly textureFormat: (name: string) => number;
}

interface WebGlLibrary {
  readonly registerContext: (context: object) => number;
  readonly unregisterContext: (handle: number) => void;
}

/** The graphics library that a module of each backend exports. */
export interface WebGraphicsLibraries {
  readonly mlnTsWebGpu?: WebGpuLibrary;
  readonly mlnTsWebGl?: WebGlLibrary;
}

let libraries: WebGraphicsLibraries | undefined;

export function installWebGraphics(module: WebGraphicsLibraries): void {
  libraries = module;
}

function loaded(): WebGraphicsLibraries {
  if (libraries === undefined) {
    throw new Error(
      "the MapLibre Native browser module is not loaded; call loadModule() first",
    );
  }
  return libraries;
}

function webGpu(): WebGpuLibrary {
  const library = loaded().mlnTsWebGpu;
  if (library === undefined) {
    throw new MaplibreError(
      Status.unsupported,
      "this browser module carries no WebGPU backend",
    );
  }
  return library;
}

function importWebGpu(
  imported: number,
  name: string,
  type: string,
  release: (handle: number) => void,
): ImportedWebGpuObject {
  if (imported === 0) {
    throw new TypeError(`${name} is not a ${type} of this realm`);
  }
  return new ImportedWebGpuObject(ADOPT, imported, release);
}

/**
 * Imports a `GPUDevice` for a `WebGpuContextDescriptor`, whose instance and
 * queue stay unset.
 *
 * Throws a `TypeError` when `device` is not a `GPUDevice`.
 */
export function importWebGpuDevice(device: object): ImportedWebGpuObject {
  const library = webGpu();
  return importWebGpu(
    library.importDevice(device),
    "device",
    "GPUDevice",
    library.releaseDevice,
  );
}

/**
 * Imports the `GPUCanvasContext` of a canvas for a `WebGpuSurfaceDescriptor`.
 *
 * Throws a `TypeError` when `canvasContext` is not a `GPUCanvasContext`.
 */
export function importWebGpuSurface(
  canvasContext: object,
): ImportedWebGpuObject {
  const library = webGpu();
  return importWebGpu(
    library.importSurface(canvasContext),
    "canvasContext",
    "GPUCanvasContext",
    library.releaseSurface,
  );
}

/**
 * Imports a `GPUTexture`, such as one of a borrowed texture ring.
 *
 * Throws a `TypeError` when `texture` is not a `GPUTexture`.
 */
export function importWebGpuTexture(texture: object): ImportedWebGpuObject {
  const library = webGpu();
  return importWebGpu(
    library.importTexture(texture),
    "texture",
    "GPUTexture",
    library.releaseTexture,
  );
}

/**
 * Imports a `GPUTextureView`, such as one of a borrowed texture ring.
 *
 * Throws a `TypeError` when `view` is not a `GPUTextureView`.
 */
export function importWebGpuTextureView(view: object): ImportedWebGpuObject {
  const library = webGpu();
  return importWebGpu(
    library.importTextureView(view),
    "view",
    "GPUTextureView",
    library.releaseTextureView,
  );
}

/**
 * Returns the `WGPUTextureFormat` value of a `GPUTextureFormat` name, such as
 * `navigator.gpu.getPreferredCanvasFormat()`.
 *
 * Throws a `RangeError` for an unknown name.
 */
export function webGpuTextureFormat(format: string): number {
  const value = webGpu().textureFormat(format);
  if (value < 0) {
    throw new RangeError(`unknown WebGPU texture format: ${format}`);
  }
  return value;
}

/**
 * Returns the JavaScript object behind a WebGPU handle, such as an acquired
 * frame's texture, valid for as long as the handle is.
 *
 * Throws a `RangeError` when `pointer` names no WebGPU object.
 */
export function webGpuObject(pointer: bigint): object {
  const library = webGpu();
  const object =
    pointer > 0n && pointer <= 0xffffffffn
      ? library.object(Number(pointer))
      : null;
  if (object === null) {
    throw new RangeError("pointer names no WebGPU object of this module");
  }
  return object;
}

/**
 * Registers a `WebGL2RenderingContext` for a `WebGlContextDescriptor` of kind
 * `existing`. Create the context with a depth and a stencil buffer.
 *
 * Throws a `TypeError` when `context` is not a `WebGL2RenderingContext`.
 */
export function registerWebGlContext(context: object): RegisteredWebGlContext {
  const library = loaded().mlnTsWebGl;
  if (library === undefined) {
    throw new MaplibreError(
      Status.unsupported,
      "this browser module carries no WebGL backend",
    );
  }
  const handle = library.registerContext(context);
  if (handle === 0) {
    throw new TypeError(
      "context is not a WebGL2RenderingContext of this realm",
    );
  }
  return new RegisteredWebGlContext(ADOPT, handle, library);
}

function closedError(name: string): MaplibreError {
  return new MaplibreError(Status.invalidState, `${name} is closed`);
}

/**
 * A WebGPU object imported into the browser module. Closing it leaves the
 * JavaScript object usable.
 *
 * Close it only after every session that uses it has detached or been
 * destroyed. An import that is never closed keeps its object alive.
 */
export class ImportedWebGpuObject {
  readonly #handle: number;
  readonly #release: (handle: number) => void;
  #closed = false;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(
    key: AdoptKey,
    handle: number,
    release: (handle: number) => void,
  ) {
    checkKey(key);
    this.#handle = handle;
    this.#release = release;
  }

  /**
   * The handle for a render target descriptor. Throws a `MaplibreError` with
   * the invalid-state status after close.
   */
  get pointer(): bigint {
    if (this.#closed) {
      throw closedError("ImportedWebGpuObject");
    }
    return BigInt(this.#handle >>> 0);
  }

  /** Releases the import. A second call does nothing. */
  close(): void {
    if (this.#closed) {
      return;
    }
    this.#closed = true;
    this.#release(this.#handle);
  }

  /**
   * Calls `close()`, for `using`. Hosts without `Symbol.dispose` leave it
   * undefined.
   */
  declare [Symbol.dispose]: () => void;
}

/**
 * A WebGL 2 context registered with the browser module. Closing it leaves the
 * context usable.
 *
 * Close it only after every session that uses it has detached or been
 * destroyed. A registration that is never closed keeps its context alive.
 */
export class RegisteredWebGlContext {
  readonly #handle: number;
  readonly #library: WebGlLibrary;
  #closed = false;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: number, library: WebGlLibrary) {
    checkKey(key);
    this.#handle = handle;
    this.#library = library;
  }

  /**
   * The handle for a `WebGlContextDescriptor` of kind `existing`. Throws a
   * `MaplibreError` with the invalid-state status after close.
   */
  get context(): number {
    if (this.#closed) {
      throw closedError("RegisteredWebGlContext");
    }
    return this.#handle;
  }

  /**
   * Removes the registration. A descriptor that still names the handle fails
   * to make a context current. A second call does nothing.
   */
  close(): void {
    if (this.#closed) {
      return;
    }
    this.#closed = true;
    this.#library.unregisterContext(this.#handle);
  }

  /**
   * Calls `close()`, for `using`. Hosts without `Symbol.dispose` leave it
   * undefined.
   */
  declare [Symbol.dispose]: () => void;
}

// Each class closes for `using` where the host defines Symbol.dispose.
const dispose = (Symbol as { dispose?: symbol }).dispose;
if (dispose !== undefined) {
  for (const type of [ImportedWebGpuObject, RegisteredWebGlContext]) {
    Object.defineProperty(type.prototype, dispose, {
      configurable: true,
      writable: true,
      value(this: { close(): void }) {
        this.close();
      },
    });
  }
}
