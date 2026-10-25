// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedResourceRequestOperations internal constructor() {
  internal abstract val binding: HandleState
  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Completes a C API resource provider request and consumes its handle.
   *
   * See `mln_resource_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun complete(response: ResourceResponse): Unit =
    nativeAnswer(this, binding) {
      check(C.mln_resource_request_complete(handle, writeResourceResponse(response), diagnostic))
    }

  /**
   * Reports whether MapLibre has cancelled a C API resource provider request.
   *
   * See `mln_resource_request_is_cancelled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun isCancelled(): Boolean =
    nativeCall(this, binding) {
      val out = allocate(1, 1)
      check(C.mln_resource_request_is_cancelled(handle, out, diagnostic))
      readBool(out)
    }

  /**
   * Hands an outstanding provider request to the built-in network loader and consumes its handle.
   *
   * See `mln_resource_request_pass_through` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun passThrough(): Unit =
    nativeAnswer(this, binding) { check(C.mln_resource_request_pass_through(handle, diagnostic)) }

  /**
   * Releases the provider's resource request handle without answering it.
   *
   * See `mln_resource_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun release(): Unit = nativeClose(this, binding) { C.mln_resource_request_release(handle) }

  /**
   * Registers a callback that runs when MapLibre cancels a C API resource provider request. A
   * request that is already cancelled registers nothing and reports true through out_cancelled, and
   * the caller handles that cancellation itself.
   *
   * See `mln_resource_request_set_cancel_callback` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setCancelCallback(handler: ResourceRequestCancelHandler): Boolean =
    nativeCall(this, binding) {
      val out = allocate(1)
      check(
        C.mln_resource_request_set_cancel_callback(
          handle,
          writeResourceRequestCancelHandler(handler),
          out,
          diagnostic,
        ),
      )
      val outCancelled = readBool(out)
      if (!outCancelled) accept(bindingCallbacks)
      outCancelled
    }
}
