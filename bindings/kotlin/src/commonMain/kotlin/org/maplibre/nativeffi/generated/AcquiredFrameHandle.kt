// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * A rendered frame that a render session lends until its release.
 *
 * See `mln_acquired_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class AcquiredFrameHandle
internal constructor(
  handle: Long,
  @Suppress("unused") private val parent: RenderSessionHandle,
) : GeneratedAcquiredFrameOperations() {
  internal override val binding =
    HandleState("AcquiredFrameHandle", handle, GeneratedOwnerDisposal::acquiredFrame)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after close. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  override fun toString(): String =
    "AcquiredFrameHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
