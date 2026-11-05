// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.JvmSynthetic
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A GPU device that the library creates to render maps offscreen.
 *
 * See `mln_offscreen_device` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class OffscreenDeviceHandle
private constructor(
  handle: Long,
) : AutoCloseable {
  @get:JvmSynthetic
  internal val binding =
    HandleState("OffscreenDeviceHandle", handle, GeneratedOwnerDisposal::offscreenDevice)
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
   * Releases an offscreen device.
   *
   * Render sessions on the device keep working until they detach.
   *
   * See `mln_offscreen_device_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
   */
  public fun release(): Unit = nativeClose(this, binding) { C.mln_offscreen_device_release(handle) }

  override fun toString(): String =
    "OffscreenDeviceHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {
    /**
     * Creates a GPU device for rendering maps offscreen.
     *
     * `MapHandle.attachOffscreen()` creates a device of its own when it gets none. Create one to
     * share it among the sessions on many maps, or to pick a GPU.
     *
     * In a browser WebGPU build, attach without a device instead.
     *
     * See `mln_offscreen_device_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
     *
     * Errors:
     *
     * - `Status.UNSUPPORTED` in a browser WebGPU build, or when the device can't render with
     *   `client_api`.
     * - `Status.NATIVE_ERROR` when `adapter_index` names no GPU, or the platform can't create the
     *   device.
     */
    public fun create(options: OffscreenDeviceOptions? = null): OffscreenDeviceHandle =
      nativeCall(null, null) {
        val out = allocate(8, w(8, 8, 4))
        check(
          C.mln_offscreen_device_create(
            options?.let { writeOffscreenDeviceOptions(it) } ?: 0L,
            out,
            diagnostic,
          ),
        )
        adopt(out, GeneratedOwnerDisposal::offscreenDevice) { OffscreenDeviceHandle.wrap(it) }
      }

    /** Creates the owner of `handle`. */
    @JvmSynthetic
    internal fun wrap(handle: Long): OffscreenDeviceHandle = OffscreenDeviceHandle(handle)
  }
}
