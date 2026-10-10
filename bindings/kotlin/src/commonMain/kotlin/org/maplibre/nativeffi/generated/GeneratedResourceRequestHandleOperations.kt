// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedResourceRequestHandleOperations internal constructor() {
  internal abstract val binding: DecisionOwnerState
  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Reports whether MapLibre has cancelled a C API resource provider request.
   *
   * See `mln_resource_request_cancelled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun cancelled(): Boolean =
    nativeCall(this, binding, "mln_resource_request_cancelled", Access.READ) {
      val out = allocate(1)
      check(C.mln_resource_request_cancelled(handle, out, diagnostic))
      readBool(out)
    }

  /**
   * Completes a C API resource provider request.
   *
   * See `mln_resource_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun complete(response: ResourceResponse): Unit =
    nativeComplete(this, binding, "mln_resource_request_complete") {
      check(C.mln_resource_request_complete(handle, writeResourceResponse(response), diagnostic))
    }

  /**
   * Releases the provider's reference to a resource request handle.
   *
   * See `mln_resource_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun release(): Unit =
    nativeClose(this, binding, "mln_resource_request_release") {
      C.mln_resource_request_release(handle)
    }

  /**
   * Registers a callback that runs when MapLibre cancels a C API resource provider request.
   *
   * See `mln_resource_request_set_cancel_callback` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun setCancelCallback(callback: ResourceRequestCancelCallback): Boolean =
    nativeCall(this, binding, "mln_resource_request_set_cancel_callback", Access.READ) {
      val token =
        registrations.register(GeneratedResourceRequestCancelCallbackRegistration(callback), handle)
      val out = allocate(1)
      check(
        C.mln_resource_request_set_cancel_callback(
          handle,
          UpcallStubs.resourceRequestCancelCallback,
          token,
          UpcallStubs.releaseRoot,
          out,
          diagnostic,
        )
      )
      val outCancelled = readBool(out)
      if (!outCancelled) accept(bindingCallbacks)
      outCancelled
    }

  /**
   * Blocks until a resource request is released and its cancel callback registration has retired:
   * the callback, if it ran, and release_user_data have both returned. Completing a request does
   * not release its owner.
   *
   * See `mln_resource_request_wait_until_retired` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun waitUntilRetired(): Unit =
    nativeCall(this, binding, "mln_resource_request_wait_until_retired", Access.ISSUED) {
      check(C.mln_resource_request_wait_until_retired(handle, diagnostic))
    }
}
