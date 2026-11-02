// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A resource load that a resource provider answers.
 *
 * See `mln_resource_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class ResourceRequestHandle
internal constructor(
  handle: Long,
) : AutoCloseable {
  internal val binding =
    HandleState("ResourceRequestHandle", handle, GeneratedOwnerDisposal::resourceRequest)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after close. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Answers a resource request.
   *
   * See `mln_resource_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_STATE` when the library cancelled the request.
   */
  public fun complete(response: ResourceResponse): Unit =
    nativeAnswer(this, binding) {
      check(C.mln_resource_request_complete(handle, writeResourceResponse(response), diagnostic))
    }

  /**
   * Reads whether the library cancelled a resource request.
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
   * Hands a resource request back to the library to load as usual.
   *
   * See `mln_resource_request_pass_through` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_STATE` when the library cancelled the request.
   */
  public fun passThrough(): Unit =
    nativeAnswer(this, binding) { check(C.mln_resource_request_pass_through(handle, diagnostic)) }

  /**
   * Releases a resource request.
   *
   * See `mln_resource_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public override fun close(): Unit =
    nativeClose(this, binding) { C.mln_resource_request_release(handle) }

  /**
   * Calls a callback when the library cancels a resource request.
   *
   * The callback never runs after the request is answered. A request takes one cancel callback.
   * When the library has already cancelled the request, the call reports that instead and the
   * callback never runs.
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

  override fun toString(): String =
    "ResourceRequestHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
