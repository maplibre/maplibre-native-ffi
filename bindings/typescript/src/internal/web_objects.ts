// The JavaScript objects that a page lends the browser module, such as an
// OffscreenCanvas, a WebGL2RenderingContext, or a GPUDevice, as the
// `mln_web_object` handles that descriptors take.

import { native } from "./addon.ts";
import { ADOPT, type AdoptKey, checkKey } from "./owners.ts";

/** The browser module's table of JavaScript objects. */
export interface WebObjectTable {
  readonly insert: (value: unknown) => number;
  readonly get: (handle: number) => unknown;
}

let table: WebObjectTable | undefined;

export function installWebObjects(loaded: WebObjectTable): void {
  table = loaded;
}

function loaded(): WebObjectTable {
  if (table === undefined) {
    throw new Error(
      "the MapLibre Native browser module is not loaded; call loadModule() first",
    );
  }
  return table;
}

/**
 * A JavaScript object that you lend the browser module, as the handle that a
 * descriptor's `mln_web_object` field takes.
 *
 * A call that takes a handle keeps its own reference to the object, so you may
 * close the handle right after the call. A handle is valid only in the module
 * that made it.
 *
 * See `web.h` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/web_8h.html).
 */
export class WebObject {
  readonly #handle: number;
  #closed = false;

  /**
   * Created by {@link WebObject.of}, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: number) {
    checkKey(key);
    this.#handle = handle;
  }

  /** Lends `value` to the module. */
  static of(value: object): WebObject {
    return new WebObject(ADOPT, loaded().insert(value));
  }

  /**
   * The object that `handle` names, such as the `GPUTexture` of an acquired
   * frame, or undefined for a handle that names none.
   */
  static get(handle: number): unknown {
    return loaded().get(handle >>> 0);
  }

  /** The handle, for a descriptor. Throws after close. */
  get handle(): number {
    if (this.#closed) {
      throw new Error("WebObject is closed");
    }
    return this.#handle;
  }

  /** Releases the module's reference to the object. A second call does nothing. */
  close(): void {
    if (this.#closed) {
      return;
    }
    this.#closed = true;
    native.mln_web_object_release(this.#handle);
  }

  /**
   * Calls `close()`, for `using`. Hosts without `Symbol.dispose` leave it
   * undefined.
   */
  declare [Symbol.dispose]: () => void;
}

const dispose = (Symbol as { dispose?: symbol }).dispose;
if (dispose !== undefined) {
  Object.defineProperty(WebObject.prototype, dispose, {
    configurable: true,
    writable: true,
    value(this: WebObject) {
      this.close();
    },
  });
}
