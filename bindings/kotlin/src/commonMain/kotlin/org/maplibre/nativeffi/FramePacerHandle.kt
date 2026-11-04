// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Demands render sessions' frames in step with a display's refreshes.
 *
 * See `mln_frame_pacer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class FramePacerHandle
internal constructor(
  handle: Long,
) : AutoCloseable {
  internal val binding = HandleState("FramePacerHandle", handle, GeneratedOwnerDisposal::framePacer)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after release. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /** Calls [release], for the standard library's `use`. */
  public override fun close() {
    release()
  }

  /**
   * Releases a frame pacer.
   *
   * Render sessions that use the pacer stay paced until you release them or set another pacer.
   *
   * See `mln_frame_pacer_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun release(): Unit = nativeClose(this, binding) { C.mln_frame_pacer_release(handle) }

  override fun toString(): String =
    "FramePacerHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
