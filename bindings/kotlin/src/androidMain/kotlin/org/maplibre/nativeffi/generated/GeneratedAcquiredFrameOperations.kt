// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.bytedeco.javacpp.*
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
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
        PointerScope().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_acquired_frame_dispose(owner, diagnostic)
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetMetalTexture(block: (MetalOwnedTextureFrame) -> T): T {
    try {
      return PointerScope().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_metal_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = PointerPointer<Pointer>(1L)
        NativeDiagnostics.check { diagnostic ->
          MaplibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = MaplibreNativeC.mln_metal_owned_texture_frame()
          output.size(output.sizeof())
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_acquired_frame_get_metal_texture(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readMetalOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          MaplibreNativeC.mln_adapter_acquired_frame_view_end(token.get(Pointer::class.java, 0))
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetOpenglTexture(block: (OpenglOwnedTextureFrame) -> T): T {
    try {
      return PointerScope().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_opengl_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = PointerPointer<Pointer>(1L)
        NativeDiagnostics.check { diagnostic ->
          MaplibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = MaplibreNativeC.mln_opengl_owned_texture_frame()
          output.size(output.sizeof())
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_acquired_frame_get_opengl_texture(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readOpenglOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          MaplibreNativeC.mln_adapter_acquired_frame_view_end(token.get(Pointer::class.java, 0))
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetProducerSync(block: (GpuSync) -> T): T {
    try {
      return PointerScope().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_producer_sync",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = PointerPointer<Pointer>(1L)
        NativeDiagnostics.check { diagnostic ->
          MaplibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = MaplibreNativeC.mln_gpu_sync()
          output.size(output.sizeof())
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_acquired_frame_get_producer_sync(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readGpuSync(output, scope))
        } finally {
          scope.close()
          MaplibreNativeC.mln_adapter_acquired_frame_view_end(token.get(Pointer::class.java, 0))
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
      return PointerScope().use { arena ->
        val output = MaplibreNativeC.mln_render_frame_result()
        output.size(output.sizeof())
        NativeDiagnostics.check { diagnostic ->
          MaplibreNativeC.mln_acquired_frame_get_result(
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
      return PointerScope().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_vulkan_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = PointerPointer<Pointer>(1L)
        NativeDiagnostics.check { diagnostic ->
          MaplibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = MaplibreNativeC.mln_vulkan_owned_texture_frame()
          output.size(output.sizeof())
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_acquired_frame_get_vulkan_texture(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readVulkanOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          MaplibreNativeC.mln_adapter_acquired_frame_view_end(token.get(Pointer::class.java, 0))
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun <T> withGetWebgpuTexture(block: (WebgpuOwnedTextureFrame) -> T): T {
    try {
      return PointerScope().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingAcquiredFrameHandle().toLong(),
          "mln_acquired_frame_get_webgpu_texture",
        )
        val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()
        val token = PointerPointer<Pointer>(1L)
        NativeDiagnostics.check { diagnostic ->
          MaplibreNativeC.mln_adapter_acquired_frame_view_begin(
            bindingAcquiredFrameHandle(),
            token,
            diagnostic,
          )
        }
        try {
          val output = MaplibreNativeC.mln_webgpu_owned_texture_frame()
          output.size(output.sizeof())
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_acquired_frame_get_webgpu_texture(
              bindingAcquiredFrameHandle(),
              output,
              diagnostic,
            )
          }
          block(GeneratedValues.readWebgpuOwnedTextureFrame(output, scope))
        } finally {
          scope.close()
          MaplibreNativeC.mln_adapter_acquired_frame_view_end(token.get(Pointer::class.java, 0))
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
        PointerScope().use { arena ->
          val holder = LongPointer(1L).put(owner)
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_acquired_frame_release(
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
