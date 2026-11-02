// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A GPU device that the library creates to render maps into textures.
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
   * Render sessions on the device keep working until they detach.
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
     * Creates a GPU device for rendering maps into textures.
     *
     * Render sessions on several maps can share one device.
     *
     * See `mln_graphics_device_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
     *
     * Errors:
     *
     * - `Status.UNSUPPORTED` when this build's render backend can't create a device.
     * - `Status.NATIVE_ERROR` when `adapter_index` names no GPU, or the platform can't create the
     *   device.
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
