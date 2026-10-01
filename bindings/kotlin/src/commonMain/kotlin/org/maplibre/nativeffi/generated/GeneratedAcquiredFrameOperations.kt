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
      val token = allocate(8)
      check(C.mln_adapter_acquired_frame_view_begin(handle, token, diagnostic))
      val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
      try {
        val out = allocate(w(56, 64), 8).also { writeU32(it, w(56, 64).toUInt()) }
        check(C.mln_acquired_frame_get_metal_texture(handle, out, diagnostic))
        block(readMetalOwnedTextureFrame(out, scope))
      } finally {
        scope.close()
        C.mln_adapter_acquired_frame_view_end(readAddress(token))
      }
    }

  public fun <T> withGetOpenglTexture(block: (OpenglOwnedTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_opengl_texture") {
      val token = allocate(8)
      check(C.mln_adapter_acquired_frame_view_begin(handle, token, diagnostic))
      val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
      try {
        val out = allocate(64, 8).also { writeU32(it, 64.toUInt()) }
        check(C.mln_acquired_frame_get_opengl_texture(handle, out, diagnostic))
        block(readOpenglOwnedTextureFrame(out, scope))
      } finally {
        scope.close()
        C.mln_adapter_acquired_frame_view_end(readAddress(token))
      }
    }

  public fun <T> withGetProducerSync(block: (GpuSync) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_producer_sync") {
      val token = allocate(8)
      check(C.mln_adapter_acquired_frame_view_begin(handle, token, diagnostic))
      val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
      try {
        val out = allocate(24, 8).also { writeU32(it, 24.toUInt()) }
        check(C.mln_acquired_frame_get_producer_sync(handle, out, diagnostic))
        block(readGpuSync(out, scope))
      } finally {
        scope.close()
        C.mln_adapter_acquired_frame_view_end(readAddress(token))
      }
    }

  public fun getResult(): RenderFrameResult =
    nativeCall(this, binding, "mln_acquired_frame_get_result") {
      val out = allocate(48, 8).also { writeU32(it, 48.toUInt()) }
      check(C.mln_acquired_frame_get_result(handle, out, diagnostic))
      readRenderFrameResult(out)
    }

  public fun <T> withGetVulkanTexture(block: (VulkanOwnedTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_vulkan_texture") {
      val token = allocate(8)
      check(C.mln_adapter_acquired_frame_view_begin(handle, token, diagnostic))
      val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
      try {
        val out = allocate(72, 8).also { writeU32(it, 72.toUInt()) }
        check(C.mln_acquired_frame_get_vulkan_texture(handle, out, diagnostic))
        block(readVulkanOwnedTextureFrame(out, scope))
      } finally {
        scope.close()
        C.mln_adapter_acquired_frame_view_end(readAddress(token))
      }
    }

  public fun <T> withGetWebgpuTexture(block: (WebgpuOwnedTextureFrame) -> T): T =
    nativeCall(this, binding, "mln_acquired_frame_get_webgpu_texture") {
      val token = allocate(8)
      check(C.mln_adapter_acquired_frame_view_begin(handle, token, diagnostic))
      val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
      try {
        val out = allocate(w(56, 72), 8).also { writeU32(it, w(56, 72).toUInt()) }
        check(C.mln_acquired_frame_get_webgpu_texture(handle, out, diagnostic))
        block(readWebgpuOwnedTextureFrame(out, scope))
      } finally {
        scope.close()
        C.mln_adapter_acquired_frame_view_end(readAddress(token))
      }
    }

  public fun release(consumerCompletion: GpuSync = GeneratedApi.gpuSyncDefault()): Unit =
    nativeClose(this, binding, "mln_acquired_frame_release") {
      val holder = allocate(8).also { writeI64(it, handle) }
      check(C.mln_acquired_frame_release(holder, writeGpuSync(consumerCompletion), diagnostic))
    }
}
