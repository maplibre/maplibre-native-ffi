// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*
import org.maplibre.nativeffi.runtime.CommandCompletion

public abstract class GeneratedRenderSessionOperations internal constructor() {
  internal abstract val binding: HandleStateCore
  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  public fun metalBorrowedTextureSetTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_metal_borrowed_texture_set_target") {
      check(
        C.mln_metal_borrowed_texture_set_target(
          handle,
          writeMetalBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  public fun metalSurfaceSetTarget(descriptor: MetalSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding, "mln_metal_surface_set_target") {
      check(
        C.mln_metal_surface_set_target(
          handle,
          writeMetalSurfaceDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  public fun openglBorrowedTextureSetTarget(
    descriptor: OpenglBorrowedTextureDescriptor
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_opengl_borrowed_texture_set_target") {
      check(
        C.mln_opengl_borrowed_texture_set_target(
          handle,
          writeOpenglBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  public fun openglSurfaceSetTarget(descriptor: OpenglSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding, "mln_opengl_surface_set_target") {
      check(
        C.mln_opengl_surface_set_target(
          handle,
          writeOpenglSurfaceDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  public fun abandon(): RenderAbandonResult =
    nativeCall(this, binding, "mln_render_session_abandon") {
      val out = sized(16, 4)
      check(C.mln_render_session_abandon(handle, out, diagnostic))
      readRenderAbandonResult(out)
    }

  public fun acquireFrame(): AcquiredFrameHandle =
    nativeCall(this, binding, "mln_render_session_acquire_frame") {
      val out = allocate(8)
      check(C.mln_render_session_acquire_frame(handle, out, diagnostic))
      adopt(out, GeneratedOwnerDisposal::acquiredFrame) {
        AcquiredFrameHandle(it, this@GeneratedRenderSessionOperations as RenderSessionHandle)
      }
    }

  public fun barrier(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_barrier") {
      check(C.mln_render_session_barrier(handle, completion, diagnostic))
    }

  public fun clearData(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_clear_data") {
      check(C.mln_render_session_clear_data(handle, completion, diagnostic))
    }

  public fun destroy(): Unit =
    nativeClose(this, binding, "mln_render_session_destroy") {
      check(C.mln_render_session_destroy(handle, diagnostic))
    }

  public fun detach(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_detach") {
      check(C.mln_render_session_detach(handle, completion, diagnostic))
    }

  public fun dispose(): Unit =
    nativeClose(this, binding, "mln_render_session_dispose") {
      check(C.mln_render_session_dispose(handle, diagnostic))
    }

  public fun drainFrameResults(): RenderFrameBatchHandle =
    nativeCall(this, binding, "mln_render_session_drain_frame_results") {
      val out = allocate(8)
      check(C.mln_render_session_drain_frame_results(handle, out, diagnostic))
      adopt(out, GeneratedOwnerDisposal::renderFrameBatch) { RenderFrameBatchHandle(it) }
    }

  public fun dumpDebugLogs(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_dump_debug_logs") {
      check(C.mln_render_session_dump_debug_logs(handle, completion, diagnostic))
    }

  public fun getCapabilities(): RenderSessionCapabilities =
    nativeCall(this, binding, "mln_render_session_get_capabilities") {
      val out = sized(16, 4)
      check(C.mln_render_session_get_capabilities(handle, out, diagnostic))
      readRenderSessionCapabilities(out)
    }

  public fun getSnapshot(): RenderSessionSnapshot =
    nativeCall(this, binding, "mln_render_session_get_snapshot") {
      val out = sized(104, 8)
      check(C.mln_render_session_get_snapshot(handle, out, diagnostic))
      readRenderSessionSnapshot(out)
    }

  public fun projectionCreate(): MapProjectionHandle =
    nativeCall(this, binding, "mln_render_session_projection_create") {
      val out = allocate(8)
      check(C.mln_render_session_projection_create(handle, out, diagnostic))
      adopt(out, GeneratedOwnerDisposal::mapProjection) { MapProjectionHandle(it) }
    }

  public fun queryFeatureExtensions(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray? = null,
  ): Deferred<ByteArray> =
    nativeSubmit(
      this,
      binding,
      "mln_render_session_query_feature_extensions",
      { result -> readView(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_render_session_query_feature_extensions(
          handle,
          view(sourceId),
          view(feature),
          view(extension),
          view(extensionField),
          arguments?.let { view(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions? = null,
  ): Deferred<List<QueriedFeature>> =
    nativeSubmit(
      this,
      binding,
      "mln_render_session_query_rendered_features",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          w(40, 72).toLong(),
        ) {
          readQueriedFeature(it)
        }
      },
    ) {
      check(
        C.mln_render_session_query_rendered_features(
          handle,
          writeRenderedQueryGeometry(geometry),
          options?.let { writeRenderedFeatureQueryOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions? = null,
  ): Deferred<List<QueriedFeature>> =
    nativeSubmit(
      this,
      binding,
      "mln_render_session_query_source_features",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          w(40, 72).toLong(),
        ) {
          readQueriedFeature(it)
        }
      },
    ) {
      check(
        C.mln_render_session_query_source_features(
          handle,
          view(sourceId),
          options?.let { writeSourceFeatureQueryOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun reduceMemoryUse(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_render_session_reduce_memory_use") {
      check(C.mln_render_session_reduce_memory_use(handle, completion, diagnostic))
    }

  public fun requestFrame(demand: FrameDemand): Unit =
    nativeCall(this, binding, "mln_render_session_request_frame") {
      check(C.mln_render_session_request_frame(handle, writeFrameDemand(demand), diagnostic))
    }

  public fun resize(extent: RenderTargetExtent): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_render_session_resize") {
      check(
        C.mln_render_session_resize(handle, writeRenderTargetExtent(extent), completion, diagnostic)
      )
    }

  public fun serviceDriverWork(maxWork: ULong): ULong =
    nativeCall(this, binding, "mln_render_session_service_driver_work") {
      val out = allocate(8)
      check(C.mln_render_session_service_driver_work(handle, maxWork.toLong(), out, diagnostic))
      readSize(out)
    }

  public fun textureReadPremultipliedRgba8(): Deferred<TextureReadbackResult> =
    nativeSubmit(
      this,
      binding,
      "mln_texture_read_premultiplied_rgba8",
      { result -> readTextureReadbackResult(CompletionBridge.value(result)) },
    ) {
      check(C.mln_texture_read_premultiplied_rgba8(handle, completion, diagnostic))
    }

  public fun vulkanBorrowedTextureSetTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_vulkan_borrowed_texture_set_target") {
      check(
        C.mln_vulkan_borrowed_texture_set_target(
          handle,
          writeVulkanBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  public fun vulkanSurfaceSetTarget(descriptor: VulkanSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding, "mln_vulkan_surface_set_target") {
      check(
        C.mln_vulkan_surface_set_target(
          handle,
          writeVulkanSurfaceDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  public fun webgpuBorrowedTextureSetTarget(
    descriptor: WebgpuBorrowedTextureDescriptor
  ): Deferred<Unit> =
    nativeUnit(this, binding, "mln_webgpu_borrowed_texture_set_target") {
      check(
        C.mln_webgpu_borrowed_texture_set_target(
          handle,
          writeWebgpuBorrowedTextureDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }

  public fun webgpuSurfaceSetTarget(descriptor: WebgpuSurfaceDescriptor): Deferred<Unit> =
    nativeUnit(this, binding, "mln_webgpu_surface_set_target") {
      check(
        C.mln_webgpu_surface_set_target(
          handle,
          writeWebgpuSurfaceDescriptor(descriptor),
          completion,
          diagnostic,
        )
      )
    }
}
