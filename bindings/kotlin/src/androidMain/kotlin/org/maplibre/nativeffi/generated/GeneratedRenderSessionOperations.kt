// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.bytedeco.javacpp.*
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status as BindingStatus
import org.maplibre.nativeffi.runtime.CommandCompletion

public actual abstract class GeneratedRenderSessionOperations internal actual constructor() {
  internal val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingRenderSessionHandle(): Long

  internal abstract fun bindingCloseRenderSession(call: (Long) -> Int)

  public actual fun metalBorrowedTextureSetTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_metal_borrowed_texture_set_target",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_metal_borrowed_texture_set_target(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_metal_surface_set_target",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_metal_surface_set_target(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_opengl_borrowed_texture_set_target",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_opengl_borrowed_texture_set_target(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_opengl_surface_set_target",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_opengl_surface_set_target(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_abandon",
      )
      return PointerScope().use { arena ->
        val output = MaplibreNativeC.mln_render_abandon_result()
        output.size(output.sizeof())
        BindingStatus.check(
          MaplibreNativeC.mln_render_session_abandon(bindingRenderSessionHandle(), output)
        )
        GeneratedValues.readRenderAbandonResult(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun acquireFrame(): org.maplibre.nativeffi.generated.AcquiredFrameHandle {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_acquire_frame",
      )
      return PointerScope().use { arena ->
        val output = LongPointer(1L).put(0L)
        BindingStatus.check(
          MaplibreNativeC.mln_render_session_acquire_frame(bindingRenderSessionHandle(), output)
        )
        adoptOwned(
          output.get(),
          { GeneratedOwnerDisposal.acquiredFrame(it) },
          {
            AcquiredFrameHandle(
              it,
              this@GeneratedRenderSessionOperations
                as org.maplibre.nativeffi.generated.RenderSessionHandle,
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_barrier",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_render_session_barrier(bindingRenderSessionHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun clearData(): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_clear_data",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_render_session_clear_data(bindingRenderSessionHandle(), completion)
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
        PointerScope().use { arena -> MaplibreNativeC.mln_render_session_destroy(owner) }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun detach(): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_detach",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_render_session_detach(bindingRenderSessionHandle(), completion)
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
        PointerScope().use { arena -> MaplibreNativeC.mln_render_session_dispose(owner) }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun drainFrameResults(): org.maplibre.nativeffi.generated.RenderFrameBatchHandle {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_drain_frame_results",
      )
      return PointerScope().use { arena ->
        val output = LongPointer(1L).put(0L)
        BindingStatus.check(
          MaplibreNativeC.mln_render_session_drain_frame_results(
            bindingRenderSessionHandle(),
            output,
          )
        )
        adoptOwned(
          output.get(),
          { GeneratedOwnerDisposal.renderFrameBatch(it) },
          { RenderFrameBatchHandle(it) },
        )
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun dumpDebugLogs(): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_dump_debug_logs",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_render_session_dump_debug_logs(
            bindingRenderSessionHandle(),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getCapabilities(): RenderSessionCapabilities {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_get_capabilities",
      )
      return PointerScope().use { arena ->
        val output = MaplibreNativeC.mln_render_session_capabilities()
        output.size(output.sizeof())
        BindingStatus.check(
          MaplibreNativeC.mln_render_session_get_capabilities(bindingRenderSessionHandle(), output)
        )
        GeneratedValues.readRenderSessionCapabilities(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getSnapshot(): RenderSessionSnapshot {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_get_snapshot",
      )
      return PointerScope().use { arena ->
        val output = MaplibreNativeC.mln_render_session_snapshot()
        output.size(output.sizeof())
        BindingStatus.check(
          MaplibreNativeC.mln_render_session_get_snapshot(bindingRenderSessionHandle(), output)
        )
        GeneratedValues.readRenderSessionSnapshot(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun projectionCreate(): org.maplibre.nativeffi.generated.MapProjectionHandle {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_projection_create",
      )
      return PointerScope().use { arena ->
        val output = LongPointer(1L).put(0L)
        BindingStatus.check(
          MaplibreNativeC.mln_render_session_projection_create(bindingRenderSessionHandle(), output)
        )
        adoptOwned(
          output.get(),
          { GeneratedOwnerDisposal.mapProjection(it) },
          { MapProjectionHandle(it) },
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_query_feature_extensions",
      )
      return CompletionBridge.submit(
        { result -> GeneratedValues.readBytes(MaplibreNativeC.mln_buffer_view(result.value())) },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_render_session_query_feature_extensions(
              bindingRenderSessionHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.byteView(arena, feature),
              GeneratedValues.stringView(arena, extension),
              GeneratedValues.stringView(arena, extensionField),
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_query_rendered_features",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readQueriedFeatureArray(
            MaplibreNativeC.mln_queried_feature(result.value()),
            result.value_count(),
          )
        },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_render_session_query_rendered_features(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_query_source_features",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readQueriedFeatureArray(
            MaplibreNativeC.mln_queried_feature(result.value()),
            result.value_count(),
          )
        },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_render_session_query_source_features(
              bindingRenderSessionHandle(),
              GeneratedValues.stringView(arena, sourceId),
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_reduce_memory_use",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_render_session_reduce_memory_use(
            bindingRenderSessionHandle(),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun requestFrame(demand: FrameDemand): Unit {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_request_frame",
      )
      return PointerScope().use { arena ->
        BindingStatus.check(
          MaplibreNativeC.mln_render_session_request_frame(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_resize",
      )
      return CompletionBridge.command { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_render_session_resize(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_render_session_service_driver_work",
      )
      return PointerScope().use { arena ->
        val output = SizeTPointer(1L)
        BindingStatus.check(
          MaplibreNativeC.mln_render_session_service_driver_work(
            bindingRenderSessionHandle(),
            maxWork.toLong(),
            output,
          )
        )
        output.get().toULong()
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun textureReadPremultipliedRgba8(): Deferred<TextureReadbackResult> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_texture_read_premultiplied_rgba8",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readTextureReadbackResult(
            MaplibreNativeC.mln_texture_readback_result(result.value())
          )
        },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_texture_read_premultiplied_rgba8(
              bindingRenderSessionHandle(),
              completion,
            )
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_vulkan_borrowed_texture_set_target",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_vulkan_borrowed_texture_set_target(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_vulkan_surface_set_target",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_vulkan_surface_set_target(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_webgpu_borrowed_texture_set_target",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_webgpu_borrowed_texture_set_target(
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
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderSessionHandle().toLong(),
        "mln_webgpu_surface_set_target",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_webgpu_surface_set_target(
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
