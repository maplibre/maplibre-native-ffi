// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import java.lang.foreign.ValueLayout
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

public actual abstract class GeneratedAcquiredFrameOperations internal actual constructor() {
  internal abstract fun bindingAcquiredFrameHandle(): Long

  internal abstract fun bindingCloseAcquiredFrame(call: (Long) -> Unit)

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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_acquired_frame_dispose(owner, diagnostic)
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetMetalTexture(block: (MetalOwnedTextureFrame) -> T): T {
    try {
      return Arena.ofConfined().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_metal_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.allocate(ValueLayout.ADDRESS)
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = mln_metal_owned_texture_frame.allocate(arena)
          mln_metal_owned_texture_frame.size(output, mln_metal_owned_texture_frame.sizeof().toInt())
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_acquired_frame_get_metal_texture(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readMetalOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          MapLibreNativeC.mln_adapter_acquired_frame_view_end(token.get(ValueLayout.ADDRESS, 0))
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetOpenglTexture(block: (OpenglOwnedTextureFrame) -> T): T {
    try {
      return Arena.ofConfined().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_opengl_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.allocate(ValueLayout.ADDRESS)
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = mln_opengl_owned_texture_frame.allocate(arena)
          mln_opengl_owned_texture_frame.size(
            output,
            mln_opengl_owned_texture_frame.sizeof().toInt(),
          )
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_acquired_frame_get_opengl_texture(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readOpenglOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          MapLibreNativeC.mln_adapter_acquired_frame_view_end(token.get(ValueLayout.ADDRESS, 0))
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetProducerSync(block: (GpuSync) -> T): T {
    try {
      return Arena.ofConfined().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_producer_sync",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.allocate(ValueLayout.ADDRESS)
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = mln_gpu_sync.allocate(arena)
          mln_gpu_sync.size(output, mln_gpu_sync.sizeof().toInt())
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_acquired_frame_get_producer_sync(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readGpuSync(output, scope))
        } finally {
          scope.close()
          MapLibreNativeC.mln_adapter_acquired_frame_view_end(token.get(ValueLayout.ADDRESS, 0))
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getResult(): RenderFrameResult {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingAcquiredFrameHandle().toLong(),
        "mln_acquired_frame_get_result",
      )
      return Arena.ofConfined().use { arena ->
        val output = mln_render_frame_result.allocate(arena)
        mln_render_frame_result.size(output, mln_render_frame_result.sizeof().toInt())
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_acquired_frame_get_result(
            bindingAcquiredFrameHandle(),
            output,
            diagnostic,
          )
        }
        GeneratedValues.readRenderFrameResult(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetVulkanTexture(block: (VulkanOwnedTextureFrame) -> T): T {
    try {
      return Arena.ofConfined().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_vulkan_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.allocate(ValueLayout.ADDRESS)
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = mln_vulkan_owned_texture_frame.allocate(arena)
          mln_vulkan_owned_texture_frame.size(
            output,
            mln_vulkan_owned_texture_frame.sizeof().toInt(),
          )
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_acquired_frame_get_vulkan_texture(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readVulkanOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          MapLibreNativeC.mln_adapter_acquired_frame_view_end(token.get(ValueLayout.ADDRESS, 0))
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetWebgpuTexture(block: (WebgpuOwnedTextureFrame) -> T): T {
    try {
      return Arena.ofConfined().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_webgpu_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = arena.allocate(ValueLayout.ADDRESS)
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = mln_webgpu_owned_texture_frame.allocate(arena)
          mln_webgpu_owned_texture_frame.size(
            output,
            mln_webgpu_owned_texture_frame.sizeof().toInt(),
          )
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_acquired_frame_get_webgpu_texture(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readWebgpuOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          MapLibreNativeC.mln_adapter_acquired_frame_view_end(token.get(ValueLayout.ADDRESS, 0))
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
        Arena.ofConfined().use { arena ->
          val holder =
            arena.allocate(ValueLayout.JAVA_LONG).also { it.set(ValueLayout.JAVA_LONG, 0, owner) }
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_acquired_frame_release(
              holder,
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
