// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Gets the IOSurface of an exported ring's acquired frame.
 *
 * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the surface. Once you release
 * or discard the frame, the session may render into the surface again. For an API that takes a
 * `CVPixelBuffer`, wrap the surface with `CVPixelBufferCreateWithIOSurface`. A reference that you
 * take with `CFRetain`, or a pixel buffer that wraps the surface, keeps the surface's memory, but
 * not its contents.
 *
 * See `mln_acquired_frame_get_io_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when the session's ring shares its textures in another way.
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withIoSurface(block: (IoSurfaceFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(32, 40, 28), w(8, 8, 4))
      check(C.mln_acquired_frame_get_io_surface(handle, out, diagnostic))
      block(readIoSurfaceFrame(out, scope))
    }
  }

/**
 * Gets the Metal texture of an acquired frame.
 *
 * It gets a borrowed ring's texture, or an exported ring's on a Metal device: the texture of the
 * IOSurface that `AcquiredFrameHandle.withIoSurface()` gets, and the frame's depth texture with
 * `RenderSessionAttachOptions.keepDepth`. Wait for `AcquiredFrameHandle.withProducerSync()` before
 * you use them.
 *
 * See `mln_acquired_frame_get_metal_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withMetalTexture(block: (MetalTextureFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(64, 72, 52), w(8, 8, 4))
      check(C.mln_acquired_frame_get_metal_texture(handle, out, diagnostic))
      block(readMetalTextureFrame(out, scope))
    }
  }

/**
 * Wraps your Metal device and command queue, so that sessions render on them.
 *
 * See `mln_gpu_device_wrap_metal` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.METAL`.
 */
public fun GpuDeviceHandle.Companion.wrapMetal(descriptor: MetalDeviceDescriptor): GpuDeviceHandle =
  nativeCall(null, null) {
    val out = allocate(8, w(8, 8, 4))
    check(C.mln_gpu_device_wrap_metal(writeMetalDeviceDescriptor(descriptor), out, diagnostic))
    adopt(out, GeneratedOwnerDisposal::gpuDevice) { GpuDeviceHandle.wrap(it) }
  }
