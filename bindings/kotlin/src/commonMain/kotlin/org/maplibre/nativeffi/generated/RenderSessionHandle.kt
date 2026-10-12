// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * A render session, which renders one map to one render target.
 *
 * See `mln_render_session` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class RenderSessionHandle
internal constructor(
  handle: Long,
  parent: MapHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::renderSession,
) : GeneratedRenderSessionOperations(), AutoCloseable {
  internal override val binding =
    HandleStateCore("RenderSessionHandle", handle, parent, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()

  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * close, so events drained later still match this handle.
   */
  public val id: ULong
    get() = binding.issued().toULong()

  public override fun close() {
    destroy()
  }
}
