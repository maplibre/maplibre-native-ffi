// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedAcquiredFrameOperations internal actual constructor() {
  internal abstract fun bindingAcquiredFrameHandle(): ULong

  internal abstract fun bindingCloseAcquiredFrame(call: (ULong) -> Unit)

  public actual fun dispose(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_acquired_frame_dispose"
      )
      return bindingCloseAcquiredFrame { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_acquired_frame_dispose",
        )
        memScoped {
          val arena = this
          NativeDiagnostics.check { diagnostic -> mln_acquired_frame_dispose(owner, diagnostic) }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetMetalTexture(block: (MetalOwnedTextureFrame) -> T): T {
    try {
      return memScoped {
        val arena = this
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_metal_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.alloc<COpaquePointerVar>()
        NativeDiagnostics.check { diagnostic ->
          mln_adapter_acquired_frame_view_begin(bindingAcquiredFrameHandle(), token.ptr, diagnostic)
        }
        try {
          val output = arena.alloc<mln_metal_owned_texture_frame>()
          output.size = sizeOf<mln_metal_owned_texture_frame>().toUInt()
          NativeDiagnostics.check { diagnostic ->
            mln_acquired_frame_get_metal_texture(
              bindingAcquiredFrameHandle(),
              output.ptr,
              diagnostic,
            )
          }
          block(GeneratedValues.readMetalOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          mln_adapter_acquired_frame_view_end(token.value)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetOpenglTexture(block: (OpenglOwnedTextureFrame) -> T): T {
    try {
      return memScoped {
        val arena = this
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_opengl_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.alloc<COpaquePointerVar>()
        NativeDiagnostics.check { diagnostic ->
          mln_adapter_acquired_frame_view_begin(bindingAcquiredFrameHandle(), token.ptr, diagnostic)
        }
        try {
          val output = arena.alloc<mln_opengl_owned_texture_frame>()
          output.size = sizeOf<mln_opengl_owned_texture_frame>().toUInt()
          NativeDiagnostics.check { diagnostic ->
            mln_acquired_frame_get_opengl_texture(
              bindingAcquiredFrameHandle(),
              output.ptr,
              diagnostic,
            )
          }
          block(GeneratedValues.readOpenglOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          mln_adapter_acquired_frame_view_end(token.value)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetProducerSync(block: (GpuSync) -> T): T {
    try {
      return memScoped {
        val arena = this
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_producer_sync",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.alloc<COpaquePointerVar>()
        NativeDiagnostics.check { diagnostic ->
          mln_adapter_acquired_frame_view_begin(bindingAcquiredFrameHandle(), token.ptr, diagnostic)
        }
        try {
          val output = arena.alloc<mln_gpu_sync>()
          output.size = sizeOf<mln_gpu_sync>().toUInt()
          NativeDiagnostics.check { diagnostic ->
            mln_acquired_frame_get_producer_sync(
              bindingAcquiredFrameHandle(),
              output.ptr,
              diagnostic,
            )
          }
          block(GeneratedValues.readGpuSync(output, scope))
        } finally {
          scope.close()
          mln_adapter_acquired_frame_view_end(token.value)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getResult(): RenderFrameResult {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingAcquiredFrameHandle().toLong(),
        "mln_acquired_frame_get_result",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<mln_render_frame_result>()
        output.size = sizeOf<mln_render_frame_result>().toUInt()
        NativeDiagnostics.check { diagnostic ->
          mln_acquired_frame_get_result(bindingAcquiredFrameHandle(), output.ptr, diagnostic)
        }
        GeneratedValues.readRenderFrameResult(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetVulkanTexture(block: (VulkanOwnedTextureFrame) -> T): T {
    try {
      return memScoped {
        val arena = this
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_vulkan_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.alloc<COpaquePointerVar>()
        NativeDiagnostics.check { diagnostic ->
          mln_adapter_acquired_frame_view_begin(bindingAcquiredFrameHandle(), token.ptr, diagnostic)
        }
        try {
          val output = arena.alloc<mln_vulkan_owned_texture_frame>()
          output.size = sizeOf<mln_vulkan_owned_texture_frame>().toUInt()
          NativeDiagnostics.check { diagnostic ->
            mln_acquired_frame_get_vulkan_texture(
              bindingAcquiredFrameHandle(),
              output.ptr,
              diagnostic,
            )
          }
          block(GeneratedValues.readVulkanOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          mln_adapter_acquired_frame_view_end(token.value)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetWebgpuTexture(block: (WebgpuOwnedTextureFrame) -> T): T {
    try {
      return memScoped {
        val arena = this
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_webgpu_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.alloc<COpaquePointerVar>()
        NativeDiagnostics.check { diagnostic ->
          mln_adapter_acquired_frame_view_begin(bindingAcquiredFrameHandle(), token.ptr, diagnostic)
        }
        try {
          val output = arena.alloc<mln_webgpu_owned_texture_frame>()
          output.size = sizeOf<mln_webgpu_owned_texture_frame>().toUInt()
          NativeDiagnostics.check { diagnostic ->
            mln_acquired_frame_get_webgpu_texture(
              bindingAcquiredFrameHandle(),
              output.ptr,
              diagnostic,
            )
          }
          block(GeneratedValues.readWebgpuOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          mln_adapter_acquired_frame_view_end(token.value)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun release(consumerCompletion: GpuSync): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_acquired_frame_release"
      )
      return bindingCloseAcquiredFrame { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_acquired_frame_release",
        )
        memScoped {
          val arena = this
          val holder = arena.alloc<ULongVar>().also { it.value = owner }
          NativeDiagnostics.check { diagnostic ->
            mln_acquired_frame_release(
              holder.ptr,
              GeneratedValues.writeGpuSync(arena, consumerCompletion),
              diagnostic,
            )
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
