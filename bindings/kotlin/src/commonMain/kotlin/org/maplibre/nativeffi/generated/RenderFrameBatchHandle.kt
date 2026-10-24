// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * An owned batch of frame results from one drain.
 *
 * See `mln_render_frame_batch` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class RenderFrameBatchHandle
internal constructor(
  handle: Long,
) : GeneratedRenderFrameBatchOperations(), AutoCloseable {
  internal override val binding =
    ReadHandleState("RenderFrameBatchHandle", handle, GeneratedOwnerDisposal::renderFrameBatch)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * close, so events drained later still match this handle.
   */
  public val id: ULong
    get() = binding.handle.toULong()

  public override fun close() {
    release()
  }

  override fun toString(): String =
    "RenderFrameBatchHandle(id=0x${binding.handle.toString(16)}, closed=${binding.isReleased})"
}
