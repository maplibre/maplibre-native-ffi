// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Gets the WebGPU texture of an acquired frame.
 *
 * See `mln_acquired_frame_get_web_gpu_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withWebGpuTexture(block: (WebGpuTextureFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(56, 80, 52), w(8, 8, 4))
      check(C.mln_acquired_frame_get_web_gpu_texture(handle, out, diagnostic))
      block(readWebGpuTextureFrame(out, scope))
    }
  }

/**
 * Wraps your WebGPU device, so that sessions render on it, into your textures and your canvases.
 *
 * See `mln_gpu_device_wrap_web_gpu` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` in every build: the library's WebGPU (wgpu) can't wrap a `GPUDevice` yet
 *   (https://github.com/louwers/mln-rs/issues/263). Attach a web canvas target without a device,
 *   and the session asks the browser for one.
 */
public fun GpuDeviceHandle.Companion.wrapWebGpu(context: WebGpuContextDescriptor): GpuDeviceHandle =
  nativeCall(null, null) {
    val out = allocate(8, w(8, 8, 4))
    check(C.mln_gpu_device_wrap_web_gpu(writeWebGpuContextDescriptor(context), out, diagnostic))
    adopt(out, GeneratedOwnerDisposal::gpuDevice) { GpuDeviceHandle.wrap(it) }
  }

/**
 * Allocates `size` bytes of the instance's memory, aligned to `align`, a power of two, for the
 * descriptors and buffers that you pass the library.
 *
 * Returns null when `size` is 0, `align` isn't a power of two, or the memory can't grow. The
 * memory's `buffer` detaches when it grows, so make new views of it after each call.
 *
 * See `mln_web_alloc` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/web_8h.html).
 */
public fun webAlloc(size: ULong, align: ULong): NativePointer =
  nativeCall(null, null) {
    val raw = C.mln_web_alloc(size.toLong(), align.toLong())
    NativePointer.ofAddress(raw)
  }

/**
 * Frees what `webAlloc()` allocated, with the same `size` and `align`. Freeing null does nothing.
 *
 * See `mln_web_free` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/web_8h.html).
 */
public fun webFree(pointer: NativePointer, size: ULong, align: ULong): Unit =
  nativeCall(null, null) { C.mln_web_free(pointer.address, size.toLong(), align.toLong()) }

/**
 * Releases a handle that you made. Releasing 0, or a handle that names no object, does nothing.
 *
 * See `mln_web_object_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/web_8h.html).
 */
public fun webObjectRelease(`object`: UInt): Unit =
  nativeCall(null, null) { C.mln_web_object_release(`object`.toInt()) }
