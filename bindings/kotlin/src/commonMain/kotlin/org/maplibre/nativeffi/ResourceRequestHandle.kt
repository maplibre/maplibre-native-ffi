// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A resource request that a resource provider handles.
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
  public override fun close(): Unit =
    nativeClose(this, binding) { C.mln_resource_request_release(handle) }

  /**
   * Registers a callback that runs when MapLibre cancels a C API resource provider request. The
   * callback runs on the thread that discards the request, never a host thread: the runtime's
   * native scheduler thread when a committed map or runtime command discards it, such as a style
   * change or map destruction, and a MapLibre worker thread otherwise.
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
