// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.loader.CompletionBridge
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status as BindingStatus
import org.maplibre.nativeffi.runtime.CommandCompletion

public actual abstract class GeneratedRenderSessionOperations internal actual constructor() {
  internal actual val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_metal_borrowed_texture_set_target(
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_metal_surface_set_target(
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_opengl_borrowed_texture_set_target(
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_opengl_surface_set_target(
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
      return Arena.ofConfined().use { arena ->
        val output = mln_render_abandon_result.allocate(arena)
        mln_render_abandon_result.size(output, mln_render_abandon_result.sizeof().toInt())
        BindingStatus.check(
          MapLibreNativeC.mln_render_session_abandon(bindingRenderSessionHandle(), output)
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
      return Arena.ofConfined().use { arena ->
        val output = arena.allocate(ValueLayout.JAVA_LONG)
        BindingStatus.check(
          MapLibreNativeC.mln_render_session_acquire_frame(bindingRenderSessionHandle(), output)
        )
        adoptOwned(
          output.get(ValueLayout.JAVA_LONG, 0),
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_render_session_barrier(bindingRenderSessionHandle(), completion)
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_render_session_clear_data(bindingRenderSessionHandle(), completion)
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
        Arena.ofConfined().use { arena -> MapLibreNativeC.mln_render_session_destroy(owner) }
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_render_session_detach(bindingRenderSessionHandle(), completion)
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
        Arena.ofConfined().use { arena -> MapLibreNativeC.mln_render_session_dispose(owner) }
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
      return Arena.ofConfined().use { arena ->
        val output = arena.allocate(ValueLayout.JAVA_LONG)
        BindingStatus.check(
          MapLibreNativeC.mln_render_session_drain_frame_results(
            bindingRenderSessionHandle(),
            output,
          )
        )
        adoptOwned(
          output.get(ValueLayout.JAVA_LONG, 0),
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_render_session_dump_debug_logs(
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
      return Arena.ofConfined().use { arena ->
        val output = mln_render_session_capabilities.allocate(arena)
        mln_render_session_capabilities.size(
          output,
          mln_render_session_capabilities.sizeof().toInt(),
        )
        BindingStatus.check(
          MapLibreNativeC.mln_render_session_get_capabilities(bindingRenderSessionHandle(), output)
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
      return Arena.ofConfined().use { arena ->
        val output = mln_render_session_snapshot.allocate(arena)
        mln_render_session_snapshot.size(output, mln_render_session_snapshot.sizeof().toInt())
        BindingStatus.check(
          MapLibreNativeC.mln_render_session_get_snapshot(bindingRenderSessionHandle(), output)
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
      return Arena.ofConfined().use { arena ->
        val output = arena.allocate(ValueLayout.JAVA_LONG)
        BindingStatus.check(
          MapLibreNativeC.mln_render_session_projection_create(bindingRenderSessionHandle(), output)
        )
        adoptOwned(
          output.get(ValueLayout.JAVA_LONG, 0),
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
        { result ->
          GeneratedValues.readBytes(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            MapLibreNativeC.mln_render_session_query_feature_extensions(
              bindingRenderSessionHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.byteView(arena, feature),
              GeneratedValues.stringView(arena, extension),
              GeneratedValues.stringView(arena, extensionField),
              if (arguments == null) MemorySegment.NULL
              else GeneratedValues.byteView(arena, arguments!!),
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
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            MapLibreNativeC.mln_render_session_query_rendered_features(
              bindingRenderSessionHandle(),
              GeneratedValues.writeRenderedQueryGeometry(arena, geometry),
              if (options == null) MemorySegment.NULL
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
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            MapLibreNativeC.mln_render_session_query_source_features(
              bindingRenderSessionHandle(),
              GeneratedValues.stringView(arena, sourceId),
              if (options == null) MemorySegment.NULL
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_render_session_reduce_memory_use(
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
      return Arena.ofConfined().use { arena ->
        BindingStatus.check(
          MapLibreNativeC.mln_render_session_request_frame(
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_render_session_resize(
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
      return Arena.ofConfined().use { arena ->
        val output = arena.allocate(ValueLayout.JAVA_LONG)
        BindingStatus.check(
          MapLibreNativeC.mln_render_session_service_driver_work(
            bindingRenderSessionHandle(),
            maxWork.toLong(),
            output,
          )
        )
        output.get(ValueLayout.JAVA_LONG, 0).toULong()
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
            NativeAccess.completionValue(result, mln_texture_readback_result.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            MapLibreNativeC.mln_texture_read_premultiplied_rgba8(
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_vulkan_borrowed_texture_set_target(
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_vulkan_surface_set_target(
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_webgpu_borrowed_texture_set_target(
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_webgpu_surface_set_target(
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
