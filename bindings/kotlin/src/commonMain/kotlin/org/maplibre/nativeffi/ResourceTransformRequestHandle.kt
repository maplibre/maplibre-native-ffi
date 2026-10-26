// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A resource URL transform that waits for its host's answer.
 *
 * See `mln_resource_transform_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class ResourceTransformRequestHandle
internal constructor(
  handle: Long,
) : AutoCloseable {
  internal val binding =
    HandleState(
      "ResourceTransformRequestHandle",
      handle,
      GeneratedOwnerDisposal::resourceTransformRequest,
    )
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after close. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /**
   * Answers a resource transform request with a replacement URL and consumes its handle.
   *
   * See `mln_resource_transform_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun complete(url: String): Unit =
    nativeAnswer(this, binding) {
      check(C.mln_resource_transform_request_complete(handle, view(url), diagnostic))
    }

  /**
   * Releases a resource transform request without answering it.
   *
   * See `mln_resource_transform_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public override fun close(): Unit =
    nativeClose(this, binding) { C.mln_resource_transform_request_release(handle) }

  override fun toString(): String =
    "ResourceTransformRequestHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
