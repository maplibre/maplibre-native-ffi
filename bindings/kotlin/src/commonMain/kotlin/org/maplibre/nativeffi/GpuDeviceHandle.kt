// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlin.jvm.JvmName
import kotlin.jvm.JvmStatic
import org.maplibre.nativeffi.internal.JvmSynthetic
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A GPU, and the queue that render sessions on it share.
 *
 * See `mln_gpu_device` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class GpuDeviceHandle
private constructor(
  handle: Long,
) : AutoCloseable {
  @get:JvmSynthetic
  internal val binding = HandleState("GpuDeviceHandle", handle, GeneratedOwnerDisposal::gpuDevice)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after release. */
  @get:JvmName("getId")
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /** Calls [release], for the standard library's `use`. */
  public override fun close() {
    release()
  }

  @get:JvmSynthetic internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Reads what the device can do.
   *
   * Check `export_kinds` before you attach an exported ring, to choose between the ring and a way
   * of your own to show the map.
   *
   * See `mln_gpu_device_get_capabilities` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
   */
  public fun getCapabilities(): GpuDeviceCapabilities =
    nativeCall(this, binding) {
      val out = sized(28, 4)
      check(C.mln_gpu_device_get_capabilities(handle, out, diagnostic))
      readGpuDeviceCapabilities(out)
    }

  /**
   * Releases a GPU device.
   *
   * Render sessions on the device keep using it until they detach, or their release finishes.
   *
   * See `mln_gpu_device_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
   */
  public fun release(): Unit = nativeClose(this, binding) { C.mln_gpu_device_release(handle) }

  override fun toString(): String =
    "GpuDeviceHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {
    /**
     * Creates a GPU device that the library owns, for render sessions to share.
     *
     * The sessions on a device share its GPU and queue: one `VkDevice` and queue, one `MTLDevice`
     * and command queue, or one OpenGL share group. `MapHandle.attach()` creates a device of its
     * own when it gets none. Create one to share it among the sessions of many maps, or to pick the
     * backend or the GPU. A Vulkan device enables the presentation, timeline semaphore, external
     * memory, and external semaphore extensions that its GPU offers.
     *
     * See `mln_gpu_device_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
     *
     * Errors:
     *
     * - `Status.UNSUPPORTED` when this build or platform lacks `backend`, when the device can't
     *   render with `open_gl_client_api`, when options name a GPU that the backend can't select (an
     *   OpenGL device takes no `adapter_index`, `adapter_uuid`, or `adapter_luid`, and only a
     *   Vulkan device takes `adapter_uuid` or `adapter_luid`), when a device that doesn't render
     *   with Vulkan gets a `queue_lock`, or in a browser for every backend: WebGPU makes devices
     *   asynchronously, and a WebGL2 context needs a canvas. Attach without a device, which the
     *   session makes itself, or wrap your WebGL2 context.
     * - `Status.NOT_FOUND` when no GPU matches `adapter_index`, `adapter_uuid`, or `adapter_luid`.
     * - `Status.NATIVE_ERROR` when the platform can't create the device.
     */
    @JvmStatic
    public fun create(options: GpuDeviceOptions? = null): GpuDeviceHandle =
      nativeCall(null, null) {
        val out = allocate(8, w(8, 8, 4))
        check(
          C.mln_gpu_device_create(
            options?.let { writeGpuDeviceOptions(it) } ?: 0L,
            out,
            diagnostic,
          ),
        )
        adoptRegistered(
          out,
          GeneratedOwnerDisposal::gpuDevice,
          { GpuDeviceHandle.wrap(it) },
          { it.bindingCallbacks },
          { it.release() },
        )
      }

    /**
     * Makes an OpenGL device whose share group is your context's.
     *
     * Each session on the device makes a context of its own that shares objects with yours, so that
     * it can render into your textures and present to your surfaces. A device that the library
     * creates is a share group of its own.
     *
     * WebGL has no share groups: a WebGL device is your context. Its sessions present to a web
     * canvas target whose canvas is the context's own, and take turns in the context. The library
     * resets the context's state that it tracks at the start of each frame, and leaves it undefined
     * after.
     *
     * See `mln_gpu_device_wrap_open_gl` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
     *
     * Errors:
     *
     * - `Status.UNSUPPORTED` when `openGlSupportedContextProviderMask()` lacks the context's
     *   platform.
     * - `Status.NATIVE_ERROR` when the platform rejects the context.
     */
    @JvmStatic
    public fun wrapOpenGl(context: OpenGlContextDescriptor): GpuDeviceHandle =
      nativeCall(null, null) {
        val out = allocate(8, w(8, 8, 4))
        check(C.mln_gpu_device_wrap_open_gl(writeOpenGlContextDescriptor(context), out, diagnostic))
        adopt(out, GeneratedOwnerDisposal::gpuDevice) { GpuDeviceHandle.wrap(it) }
      }

    /** Creates the owner of `handle`. */
    @JvmSynthetic internal fun wrap(handle: Long): GpuDeviceHandle = GpuDeviceHandle(handle)
  }
}
