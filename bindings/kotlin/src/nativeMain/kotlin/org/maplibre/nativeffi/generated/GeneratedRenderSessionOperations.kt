// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.lifecycle.OwnerAdoption
import org.maplibre.nativeffi.internal.status.Status as BindingStatus
import org.maplibre.nativeffi.runtime.CommandCompletion
import platform.posix.size_t
import platform.posix.size_tVar

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedRenderSessionOperations internal actual constructor() {
  internal val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingRenderSessionHandle(): ULong

  internal abstract fun bindingCloseRenderSession(call: (ULong) -> Int)

  internal abstract fun invalidateBindingViews()

  public actual fun metalBorrowedTextureSetTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_metal_borrowed_texture_set_target",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_metal_borrowed_texture_set_target(
            bindingRenderSessionHandle(),
            GeneratedValues.writeMetalBorrowedTextureDescriptor(arena, descriptor),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun metalSurfaceSetTarget(descriptor: MetalSurfaceDescriptor): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_metal_surface_set_target",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_metal_surface_set_target(
            bindingRenderSessionHandle(),
            GeneratedValues.writeMetalSurfaceDescriptor(arena, descriptor),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun openglBorrowedTextureSetTarget(
    descriptor: OpenglBorrowedTextureDescriptor
  ): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_opengl_borrowed_texture_set_target",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_opengl_borrowed_texture_set_target(
            bindingRenderSessionHandle(),
            GeneratedValues.writeOpenglBorrowedTextureDescriptor(arena, descriptor),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun openglSurfaceSetTarget(descriptor: OpenglSurfaceDescriptor): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_opengl_surface_set_target",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_opengl_surface_set_target(
            bindingRenderSessionHandle(),
            GeneratedValues.writeOpenglSurfaceDescriptor(arena, descriptor),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun abandon(): RenderAbandonResult {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_abandon",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<mln_render_abandon_result>()
        output.size = sizeOf<mln_render_abandon_result>().toUInt()
        BindingStatus.check(mln_render_session_abandon(bindingRenderSessionHandle(), output.ptr))
        invalidateBindingViews()
        GeneratedValues.readRenderAbandonResult(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun acquireFrame(): org.maplibre.nativeffi.render.AcquiredFrameHandle {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_acquire_frame",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<ULongVar>().also { it.value = 0uL }
        BindingStatus.check(
          mln_render_session_acquire_frame(bindingRenderSessionHandle(), output.ptr)
        )
        adoptOwned(
          output.value,
          { GeneratedOwnerDisposal.acquiredFrame(it.toLong()) },
          {
            OwnerAdoption.acquiredFrame(
              it,
              this@GeneratedRenderSessionOperations
                as org.maplibre.nativeffi.render.RenderSessionHandle,
            )
          },
        )
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun barrier(): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_barrier",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_render_session_barrier(bindingRenderSessionHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun clearData(): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_clear_data",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_render_session_clear_data(bindingRenderSessionHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun destroy(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_render_session_destroy"
      )
      return bindingCloseRenderSession { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_render_session_destroy",
        )
        memScoped {
          val arena = this
          mln_render_session_destroy(owner)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun detach(): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_detach",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_render_session_detach(bindingRenderSessionHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun dispose(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_render_session_dispose"
      )
      return bindingCloseRenderSession { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_render_session_dispose",
        )
        memScoped {
          val arena = this
          mln_render_session_dispose(owner)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun drainFrameResults(): org.maplibre.nativeffi.generated.RenderFrameBatchHandle {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_drain_frame_results",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<ULongVar>().also { it.value = 0uL }
        BindingStatus.check(
          mln_render_session_drain_frame_results(bindingRenderSessionHandle(), output.ptr)
        )
        adoptOwned(
          output.value,
          { GeneratedOwnerDisposal.renderFrameBatch(it.toLong()) },
          { RenderFrameBatchHandle(it) },
        )
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun dumpDebugLogs(): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_dump_debug_logs",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_render_session_dump_debug_logs(bindingRenderSessionHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getCapabilities(): RenderSessionCapabilities {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_get_capabilities",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<mln_render_session_capabilities>()
        output.size = sizeOf<mln_render_session_capabilities>().toUInt()
        BindingStatus.check(
          mln_render_session_get_capabilities(bindingRenderSessionHandle(), output.ptr)
        )
        GeneratedValues.readRenderSessionCapabilities(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getSnapshot(): RenderSessionSnapshot {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_get_snapshot",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<mln_render_session_snapshot>()
        output.size = sizeOf<mln_render_session_snapshot>().toUInt()
        BindingStatus.check(
          mln_render_session_get_snapshot(bindingRenderSessionHandle(), output.ptr)
        )
        GeneratedValues.readRenderSessionSnapshot(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun projectionCreate(): org.maplibre.nativeffi.map.MapProjectionHandle {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_projection_create",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<ULongVar>().also { it.value = 0uL }
        BindingStatus.check(
          mln_render_session_projection_create(bindingRenderSessionHandle(), output.ptr)
        )
        adoptOwned(
          output.value,
          { GeneratedOwnerDisposal.mapProjection(it.toLong()) },
          { OwnerAdoption.mapProjection(it) },
        )
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun queryFeatureExtensions(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): Deferred<ByteArray> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_query_feature_extensions",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readBytes(result.pointed.value!!.reinterpret<mln_buffer_view>().pointed)
        },
        { completion ->
          memScoped {
            val arena = this
            mln_render_session_query_feature_extensions(
              bindingRenderSessionHandle(),
              GeneratedValues.stringView(arena, sourceId).pointed.readValue(),
              GeneratedValues.byteView(arena, feature).pointed.readValue(),
              GeneratedValues.stringView(arena, extension).pointed.readValue(),
              GeneratedValues.stringView(arena, extensionField).pointed.readValue(),
              if (arguments == null) null else GeneratedValues.byteView(arena, arguments!!),
              completion,
            )
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_query_rendered_features",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readQueriedFeatureArray(
            result.pointed.value?.reinterpret<mln_queried_feature>(),
            result.pointed.value_count.toULong(),
          )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_render_session_query_rendered_features(
              bindingRenderSessionHandle(),
              GeneratedValues.writeRenderedQueryGeometry(arena, geometry),
              if (options == null) null
              else GeneratedValues.writeRenderedFeatureQueryOptions(arena, options!!),
              completion,
            )
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_query_source_features",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readQueriedFeatureArray(
            result.pointed.value?.reinterpret<mln_queried_feature>(),
            result.pointed.value_count.toULong(),
          )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_render_session_query_source_features(
              bindingRenderSessionHandle(),
              GeneratedValues.stringView(arena, sourceId).pointed.readValue(),
              if (options == null) null
              else GeneratedValues.writeSourceFeatureQueryOptions(arena, options!!),
              completion,
            )
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun reduceMemoryUse(): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_reduce_memory_use",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_render_session_reduce_memory_use(bindingRenderSessionHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun requestFrame(demand: FrameDemand): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_request_frame",
      )
      return memScoped {
        val arena = this
        BindingStatus.check(
          mln_render_session_request_frame(
            bindingRenderSessionHandle(),
            GeneratedValues.writeFrameDemand(arena, demand),
          )
        )
        Unit
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun resize(extent: RenderTargetExtent): Deferred<CommandCompletion> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_resize",
      )
      return CompletionBridge.command { completion ->
        memScoped {
          val arena = this
          mln_render_session_resize(
            bindingRenderSessionHandle(),
            GeneratedValues.writeRenderTargetExtent(arena, extent),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun serviceDriverWork(maxWork: ULong): ULong {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_service_driver_work",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<size_tVar>()
        BindingStatus.check(
          mln_render_session_service_driver_work(
            bindingRenderSessionHandle(),
            maxWork.convert<size_t>(),
            output.ptr,
          )
        )
        output.value.toULong()
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun textureReadPremultipliedRgba8(): Deferred<TextureReadbackResult> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_texture_read_premultiplied_rgba8",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readTextureReadbackResult(
            result.pointed.value!!.reinterpret<mln_texture_readback_result>().pointed
          )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_texture_read_premultiplied_rgba8(bindingRenderSessionHandle(), completion)
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun vulkanBorrowedTextureSetTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_vulkan_borrowed_texture_set_target",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_vulkan_borrowed_texture_set_target(
            bindingRenderSessionHandle(),
            GeneratedValues.writeVulkanBorrowedTextureDescriptor(arena, descriptor),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun vulkanSurfaceSetTarget(descriptor: VulkanSurfaceDescriptor): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_vulkan_surface_set_target",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_vulkan_surface_set_target(
            bindingRenderSessionHandle(),
            GeneratedValues.writeVulkanSurfaceDescriptor(arena, descriptor),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun webgpuBorrowedTextureSetTarget(
    descriptor: WebgpuBorrowedTextureDescriptor
  ): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_webgpu_borrowed_texture_set_target",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_webgpu_borrowed_texture_set_target(
            bindingRenderSessionHandle(),
            GeneratedValues.writeWebgpuBorrowedTextureDescriptor(arena, descriptor),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun webgpuSurfaceSetTarget(descriptor: WebgpuSurfaceDescriptor): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_webgpu_surface_set_target",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_webgpu_surface_set_target(
            bindingRenderSessionHandle(),
            GeneratedValues.writeWebgpuSurfaceDescriptor(arena, descriptor),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
