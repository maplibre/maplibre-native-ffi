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
  @Suppress("unused") private val parent: MapHandle,
) :
  GeneratedRenderSessionOperations(),
  org.maplibre.nativeffi.runtime.AsyncReleasable,
  AutoCloseable {
  internal override val binding =
    HandleState("RenderSessionHandle", handle, GeneratedOwnerDisposal::renderSession)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * close, so events drained later still match this handle.
   */
  public val id: ULong
    get() = binding.handle.toULong()

  public override val isClosed: Boolean
    get() = binding.isReleased

  public override fun close() {
    dispose()
  }

  override fun toString(): String =
    "RenderSessionHandle(id=0x${binding.handle.toString(16)}, closed=${binding.isReleased})"
}
