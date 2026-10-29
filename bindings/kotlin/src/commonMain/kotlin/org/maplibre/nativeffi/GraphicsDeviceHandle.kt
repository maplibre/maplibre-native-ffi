// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A GPU device that native code creates for texture render targets.
 *
 * See `mln_graphics_device` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class GraphicsDeviceHandle
internal constructor(
  handle: Long,
) : AutoCloseable {
  internal val binding =
    HandleState("GraphicsDeviceHandle", handle, GeneratedOwnerDisposal::graphicsDevice)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after close. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /**
   * Releases a graphics device.
   *
   * See `mln_graphics_device_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
   */
  public override fun close(): Unit =
    nativeClose(this, binding) { C.mln_graphics_device_release(handle) }

  override fun toString(): String =
    "GraphicsDeviceHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {
    /**
     * Creates a graphics device for this build's render backend.
     *
     * See `mln_graphics_device_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
     */
    public fun create(options: GraphicsDeviceOptions? = null): GraphicsDeviceHandle =
      nativeCall(null, null) {
        val out = allocate(8, w(8, 8, 4))
        check(
          C.mln_graphics_device_create(
            options?.let { writeGraphicsDeviceOptions(it) } ?: 0L,
            out,
            diagnostic,
          ),
        )
        adopt(out, GeneratedOwnerDisposal::graphicsDevice) { GraphicsDeviceHandle(it) }
      }
  }
}
