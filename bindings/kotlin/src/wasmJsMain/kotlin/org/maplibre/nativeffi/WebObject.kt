@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary

/**
 * A JavaScript object that you lend the browser module, such as an `OffscreenCanvas`, a
 * `WebGL2RenderingContext`, or a `GPUDevice`, as the `mln_web_object` handle that descriptors take.
 *
 * A call that takes a handle keeps its own reference to the object, so you may close the handle
 * right after the call. A handle is valid only in the module that made it.
 *
 * See `web.h` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/web_8h.html).
 */
public class WebObject private constructor(private val value: UInt) : AutoCloseable {
  private var closed = false

  /** The handle, for a descriptor's `mln_web_object` field. */
  public val handle: UInt
    get() {
      check(!closed) { "WebObject is closed" }
      return value
    }

  /** Releases the module's reference to the object. Closing again does nothing. */
  override fun close() {
    if (closed) return
    closed = true
    C.mln_web_object_release(value.toInt())
  }

  public companion object {
    /** Lends [value] to the module. */
    public fun of(value: JsAny): WebObject {
      ensureNativeLibrary()
      return WebObject(mlnWebObjectInsert(value).toUInt())
    }

    /**
     * The object that [handle] names, such as the `GPUTexture` of an acquired frame, or null for a
     * handle that names none.
     */
    public fun get(handle: UInt): JsAny? {
      ensureNativeLibrary()
      return mlnWebObjectGet(handle.toInt())
    }
  }
}

private fun mlnWebObjectInsert(value: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnWebObjectInsert(value)")

private fun mlnWebObjectGet(handle: Int): JsAny? =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnWebObjectGet(handle >>> 0) ?? null")
