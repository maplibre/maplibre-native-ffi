// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedAcquiredFrameOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  /**
   * Consumes an acquired frame and quarantines its slot of the texture ring.
   *
   * See `mln_acquired_frame_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding, "mln_acquired_frame_dispose") {
      check(C.mln_acquired_frame_dispose(handle, diagnostic))
    }

  /**
   * Copies Metal-native metadata from an acquired frame.
   *
   * See `mln_acquired_frame_get_metal_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun <T> withMetalTexture(block: (MetalTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_metal_texture", Access.READ) {
      borrowView(
        { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(w(64, 72), 8)
        check(C.mln_acquired_frame_get_metal_texture(handle, out, diagnostic))
        block(readMetalTextureFrame(out, scope))
      }
    }

  /**
   * Copies OpenGL-native metadata from an acquired frame.
   *
   * See `mln_acquired_frame_get_opengl_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun <T> withOpenglTexture(block: (OpenglTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_opengl_texture", Access.READ) {
      borrowView(
        { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(64, 8)
        check(C.mln_acquired_frame_get_opengl_texture(handle, out, diagnostic))
        block(readOpenglTextureFrame(out, scope))
      }
    }

  /**
   * Copies the producer synchronization for an acquired texture frame.
   *
   * See `mln_acquired_frame_get_producer_sync` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun <T> withProducerSync(block: (GpuSync) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_producer_sync", Access.READ) {
      borrowView(
        { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(24, 8)
        check(C.mln_acquired_frame_get_producer_sync(handle, out, diagnostic))
        block(readGpuSync(out, scope))
      }
    }

  /**
   * Copies common metadata for an acquired frame.
   *
   * See `mln_acquired_frame_get_result` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun getResult(): RenderFrameResult =
    nativeCall(this, binding, "mln_acquired_frame_get_result") {
      val out = sized(48, 8)
      check(C.mln_acquired_frame_get_result(handle, out, diagnostic))
      readRenderFrameResult(out)
    }

  /**
   * Copies Vulkan-native metadata from an acquired frame.
   *
   * See `mln_acquired_frame_get_vulkan_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun <T> withVulkanTexture(block: (VulkanTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_vulkan_texture", Access.READ) {
      borrowView(
        { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(80, 8)
        check(C.mln_acquired_frame_get_vulkan_texture(handle, out, diagnostic))
        block(readVulkanTextureFrame(out, scope))
      }
    }

  /**
   * Copies WebGPU-native metadata from an acquired frame.
   *
   * See `mln_acquired_frame_get_webgpu_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun <T> withWebgpuTexture(block: (WebgpuTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_webgpu_texture", Access.READ) {
      borrowView(
        { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(w(64, 80), 8)
        check(C.mln_acquired_frame_get_webgpu_texture(handle, out, diagnostic))
        block(readWebgpuTextureFrame(out, scope))
      }
    }

  /**
   * Releases an acquired frame after optional consumer GPU work.
   *
   * See `mln_acquired_frame_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun release(consumerCompletion: GpuSync = GeneratedApi.gpuSyncDefault()): Unit =
    nativeClose(this, binding, "mln_acquired_frame_release") {
      val holder = allocate(8, 8).also { writeI64(it, handle) }
      check(C.mln_acquired_frame_release(holder, writeGpuSync(consumerCompletion), diagnostic))
    }
}
