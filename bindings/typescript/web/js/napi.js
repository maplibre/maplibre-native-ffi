// The JavaScript half of the module's Node-API (src/napi.rs): what Rust can't
// make through wasm-bindgen alone. wasm-bindgen copies this file into the
// package's snippets/.

let invoke = null;
let finalize = null;
let drain = null;
// What the last call of a native function threw, which its wrapper rethrows
// once the call has returned through Rust.
let thrown = null;

const externals = new WeakMap();
const registry = new FinalizationRegistry(({ callback, data, hint }) =>
  finalize(callback, data, hint),
);

/**
 * Installs Rust's entry points: `invoke(callback, data, this, args)` runs a
 * native function, `finalize(callback, data, hint)` an external's finalizer,
 * and `drain(function)` a threadsafe function's queue.
 */
export function napiInstall(invokeNative, finalizeExternal, drainQueue) {
  invoke = invokeNative;
  finalize = finalizeExternal;
  drain = drainQueue;
}

/** A function that calls the native `callback` with `data`. */
export function napiFunction(name, callback, data) {
  const native = {
    [name](...args) {
      const result = invoke(callback, data, this, args);
      if (thrown !== null) {
        const { error } = thrown;
        thrown = null;
        throw error;
      }
      return result;
    },
  }[name];
  return native;
}

/** Makes the running native function's wrapper throw `error` as it returns. */
export function napiThrowOnReturn(error) {
  thrown = { error };
}

/**
 * An external: an object of its own that carries `data`, whose finalizer
 * `callback`, if not 0, runs after the collector reclaims it.
 */
export function napiExternal(data, callback, hint) {
  const external = Object.freeze(Object.create(null));
  externals.set(external, data);
  if (callback !== 0) {
    registry.register(external, { callback, data, hint });
  }
  return external;
}

/** The data of an external, or undefined for any other value. */
export function napiExternalData(value) {
  return externals.get(value);
}

export function napiIsExternal(value) {
  return externals.has(value);
}

export function napiWeakRef(value) {
  return new WeakRef(value);
}

/** The referent of a weak reference, or undefined once it's collected. */
export function napiDeref(reference) {
  return reference.deref();
}

/** Runs `drain(function)` in a microtask. */
export function napiSchedule(threadsafeFunction) {
  queueMicrotask(() => drain(threadsafeFunction));
}

/** A value as an object, with a primitive in its wrapper. */
export function napiToObject(value) {
  return Object(value);
}

/** Reports an error that nothing caught, as the page reports its own. */
export function napiReportError(error) {
  if (typeof globalThis.reportError === "function") {
    globalThis.reportError(error);
  } else {
    setTimeout(() => {
      throw error;
    });
  }
}

const typedArrays = [
  Int8Array,
  Uint8Array,
  Uint8ClampedArray,
  Int16Array,
  Uint16Array,
  Int32Array,
  Uint32Array,
  Float32Array,
  Float64Array,
  BigInt64Array,
  BigUint64Array,
];

export function napiIsTypedArray(value) {
  return ArrayBuffer.isView(value) && !(value instanceof DataView);
}

/** The `napi_typedarray_type` of a typed array. */
export function napiTypedArrayType(value) {
  return typedArrays.findIndex((type) => value instanceof type);
}

/** A typed array of `napi_typedarray_type` `type` over `buffer`. */
export function napiTypedArray(type, buffer, offset, length) {
  return new typedArrays[type](buffer, offset, length);
}

export function napiError(kind, message, code) {
  const error = new [Error, TypeError, RangeError][kind](message);
  if (code !== undefined) {
    error.code = code;
  }
  return error;
}

/** A bigint as a signed 64-bit integer, wrapped. */
export function napiAsI64(value) {
  return BigInt.asIntN(64, value);
}

/** A bigint as an unsigned 64-bit integer, wrapped. */
export function napiAsU64(value) {
  return BigInt.asUintN(64, value);
}

export function napiIsLosslessI64(value) {
  return BigInt.asIntN(64, value) === value;
}

export function napiIsLosslessU64(value) {
  return BigInt.asUintN(64, value) === value;
}
