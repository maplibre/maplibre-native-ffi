// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * A resource URL transform that waits for its host's answer.
 *
 * See `mln_resource_transform_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class ResourceTransformRequestHandle
internal constructor(
  handle: Long,
) : GeneratedResourceTransformRequestOperations(), AutoCloseable {
  internal override val binding =
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

  public override fun close() {
    release()
  }

  override fun toString(): String =
    "ResourceTransformRequestHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
