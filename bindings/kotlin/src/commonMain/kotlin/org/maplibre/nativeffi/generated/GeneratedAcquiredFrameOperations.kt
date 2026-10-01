// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedAcquiredFrameOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  public fun dispose(): Unit =
    nativeClose(this, binding, "mln_acquired_frame_dispose") {
      check(C.mln_acquired_frame_dispose(handle, diagnostic))
    }

  public fun <T> withGetMetalTexture(block: (MetalOwnedTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_metal_texture") {
      borrowView(
        { C.mln_adapter_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_adapter_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(w(56, 64), 8)
        check(C.mln_acquired_frame_get_metal_texture(handle, out, diagnostic))
        block(readMetalOwnedTextureFrame(out, scope))
      }
    }

  public fun <T> withGetOpenglTexture(block: (OpenglOwnedTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_opengl_texture") {
      borrowView(
        { C.mln_adapter_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_adapter_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(64, 8)
        check(C.mln_acquired_frame_get_opengl_texture(handle, out, diagnostic))
        block(readOpenglOwnedTextureFrame(out, scope))
      }
    }

  public fun <T> withGetProducerSync(block: (GpuSync) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_producer_sync") {
      borrowView(
        { C.mln_adapter_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_adapter_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(24, 8)
        check(C.mln_acquired_frame_get_producer_sync(handle, out, diagnostic))
        block(readGpuSync(out, scope))
      }
    }

  public fun getResult(): RenderFrameResult =
    nativeCall(this, binding, "mln_acquired_frame_get_result") {
      val out = sized(48, 8)
      check(C.mln_acquired_frame_get_result(handle, out, diagnostic))
      readRenderFrameResult(out)
    }

  public fun <T> withGetVulkanTexture(block: (VulkanOwnedTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_vulkan_texture") {
      borrowView(
        { C.mln_adapter_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_adapter_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(72, 8)
        check(C.mln_acquired_frame_get_vulkan_texture(handle, out, diagnostic))
        block(readVulkanOwnedTextureFrame(out, scope))
      }
    }

  public fun <T> withGetWebgpuTexture(block: (WebgpuOwnedTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_webgpu_texture") {
      borrowView(
        { C.mln_adapter_acquired_frame_view_begin(handle, it, diagnostic) },
        { C.mln_adapter_acquired_frame_view_end(it) },
      ) { scope ->
        val out = sized(w(56, 72), 8)
        check(C.mln_acquired_frame_get_webgpu_texture(handle, out, diagnostic))
        block(readWebgpuOwnedTextureFrame(out, scope))
      }
    }

  public fun release(consumerCompletion: GpuSync = GeneratedApi.gpuSyncDefault()): Unit =
    nativeClose(this, binding, "mln_acquired_frame_release") {
      val holder = allocate(8).also { writeI64(it, handle) }
      check(C.mln_acquired_frame_release(holder, writeGpuSync(consumerCompletion), diagnostic))
    }
}
