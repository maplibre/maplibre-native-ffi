package org.maplibre.nativeffi.render

import cnames.structs.mln_feature_extension_result
import cnames.structs.mln_feature_query_result
import cnames.structs.mln_json_snapshot
import cnames.structs.mln_render_session
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toLong
import kotlinx.cinterop.value
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_attach
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_acquire_frame
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_attach
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_release_frame
import org.maplibre.nativeffi.internal.c.mln_metal_surface_attach
import org.maplibre.nativeffi.internal.c.mln_render_session_clear_data
import org.maplibre.nativeffi.internal.c.mln_render_session_destroy
import org.maplibre.nativeffi.internal.c.mln_render_session_detach
import org.maplibre.nativeffi.internal.c.mln_render_session_dump_debug_logs
import org.maplibre.nativeffi.internal.c.mln_render_session_get_feature_state
import org.maplibre.nativeffi.internal.c.mln_render_session_query_feature_extensions
import org.maplibre.nativeffi.internal.c.mln_render_session_query_rendered_features
import org.maplibre.nativeffi.internal.c.mln_render_session_query_source_features
import org.maplibre.nativeffi.internal.c.mln_render_session_reduce_memory_use
import org.maplibre.nativeffi.internal.c.mln_render_session_remove_feature_state
import org.maplibre.nativeffi.internal.c.mln_render_session_render_update
import org.maplibre.nativeffi.internal.c.mln_render_session_resize
import org.maplibre.nativeffi.internal.c.mln_render_session_set_feature_state
import org.maplibre.nativeffi.internal.c.mln_texture_image_info_default
import org.maplibre.nativeffi.internal.c.mln_texture_read_premultiplied_rgba8
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_attach
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_acquire_frame
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_attach
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_release_frame
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_attach
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.QueryStructs
import org.maplibre.nativeffi.internal.struct.RenderStructs
import org.maplibre.nativeffi.internal.struct.ValueStructs
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Owned native render session handle. Close it on the map owner thread. */
@OptIn(ExperimentalForeignApi::class)
public class RenderSessionHandle
private constructor(private val map: MapHandle, handle: CPointer<mln_render_session>) :
  AutoCloseable {
  private val state = HandleState("RenderSessionHandle", handle, map)

  public fun resize(width: UInt, height: UInt, scaleFactor: Double) {
    Status.check(mln_render_session_resize(state.requireLive(), width, height, scaleFactor))
  }

  public fun renderUpdate() {
    Status.check(mln_render_session_render_update(state.requireLive()))
  }

  public fun detach() {
    Status.check(mln_render_session_detach(state.requireLive()))
  }

  public fun reduceMemoryUse() {
    Status.check(mln_render_session_reduce_memory_use(state.requireLive()))
  }

  public fun clearData() {
    Status.check(mln_render_session_clear_data(state.requireLive()))
  }

  public fun dumpDebugLogs() {
    Status.check(mln_render_session_dump_debug_logs(state.requireLive()))
  }

  public fun setFeatureState(selector: FeatureStateSelector, value: JsonValue) {
    memScoped {
      Status.check(
        mln_render_session_set_feature_state(
          state.requireLive(),
          QueryStructs.featureStateSelector(selector, this),
          ValueStructs.jsonValue(value, this),
        )
      )
    }
  }

  public fun getFeatureState(selector: FeatureStateSelector): JsonValue = memScoped {
    val outState = alloc<CPointerVarOf<CPointer<mln_json_snapshot>>>()
    outState.value = null
    Status.check(
      mln_render_session_get_feature_state(
        state.requireLive(),
        QueryStructs.featureStateSelector(selector, this),
        outState.ptr,
      )
    )
    ValueStructs.jsonSnapshotHandle(outState.value) ?: JsonValue.obj(emptyList())
  }

  public fun removeFeatureState(selector: FeatureStateSelector) {
    memScoped {
      Status.check(
        mln_render_session_remove_feature_state(
          state.requireLive(),
          QueryStructs.featureStateSelector(selector, this),
        )
      )
    }
  }

  public fun queryRenderedFeatures(geometry: RenderedQueryGeometry): List<QueriedFeature> =
    queryRenderedFeatures(geometry, null)

  public fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature> = memScoped {
    val outResult = alloc<CPointerVarOf<CPointer<mln_feature_query_result>>>()
    outResult.value = null
    Status.check(
      mln_render_session_query_rendered_features(
        state.requireLive(),
        QueryStructs.renderedQueryGeometry(geometry, this),
        QueryStructs.renderedFeatureQueryOptions(options, this),
        outResult.ptr,
      )
    )
    QueryStructs.featureQueryResult(requireNotNull(outResult.value))
  }

  public fun querySourceFeatures(sourceId: String): List<QueriedFeature> =
    querySourceFeatures(sourceId, null)

  public fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature> = memScoped {
    val outResult = alloc<CPointerVarOf<CPointer<mln_feature_query_result>>>()
    outResult.value = null
    Status.check(
      mln_render_session_query_source_features(
        state.requireLive(),
        CoreStructs.stringView(sourceId, this),
        QueryStructs.sourceFeatureQueryOptions(options, this),
        outResult.ptr,
      )
    )
    QueryStructs.featureQueryResult(requireNotNull(outResult.value))
  }

  public fun queryFeatureExtension(
    sourceId: String,
    feature: Feature,
    extension: String,
    extensionField: String,
  ): FeatureExtensionResult =
    queryFeatureExtension(sourceId, feature, extension, extensionField, null)

  public fun queryFeatureExtension(
    sourceId: String,
    feature: Feature,
    extension: String,
    extensionField: String,
    arguments: JsonValue?,
  ): FeatureExtensionResult = memScoped {
    val outResult = alloc<CPointerVarOf<CPointer<mln_feature_extension_result>>>()
    outResult.value = null
    Status.check(
      mln_render_session_query_feature_extensions(
        state.requireLive(),
        CoreStructs.stringView(sourceId, this),
        ValueStructs.feature(feature, this),
        CoreStructs.stringView(extension, this),
        CoreStructs.stringView(extensionField, this),
        arguments?.let { ValueStructs.jsonValue(it, this) },
        outResult.ptr,
      )
    )
    QueryStructs.featureExtensionResult(requireNotNull(outResult.value))
  }

  public fun textureImageInfo(): TextureImageInfo = memScoped {
    val outInfo = mln_texture_image_info_default().getPointer(this)
    val status = mln_texture_read_premultiplied_rgba8(state.requireLive(), null, 0UL, outInfo)
    val info = RenderStructs.textureImageInfo(outInfo.pointed)
    if (status == 0 || (status == -1 && info.byteLength > 0UL)) {
      info
    } else {
      Status.check(status)
      error("unreachable")
    }
  }

  public fun readPremultipliedRgba8(buffer: NativeBuffer): TextureImageInfo = memScoped {
    val capacity = buffer.byteLength()
    val outInfo = mln_texture_image_info_default().getPointer(this)
    Status.check(
      mln_texture_read_premultiplied_rgba8(
        state.requireLive(),
        buffer.pointer()?.reinterpret<UByteVar>(),
        capacity,
        outInfo,
      )
    )
    RenderStructs.textureImageInfo(outInfo.pointed)
  }

  public fun readPremultipliedRgba8(): PremultipliedRgba8Image {
    val info = textureImageInfo()
    NativeBuffer.allocate(info.byteLength).use { buffer ->
      val readInfo = readPremultipliedRgba8(buffer)
      return PremultipliedRgba8Image(
        readInfo.width,
        readInfo.height,
        readInfo.stride,
        buffer.toByteArray(),
      )
    }
  }

  public fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle {
    val frame = nativeHeap.alloc<mln_metal_owned_texture_frame>()
    frame.size = sizeOf<mln_metal_owned_texture_frame>().toUInt()
    try {
      Status.check(mln_metal_owned_texture_acquire_frame(state.requireLive(), frame.ptr))
      val scope = FrameScope()
      return MetalOwnedTextureFrameHandle(
        this,
        frame.ptr,
        scope,
        metalOwnedTextureFrame(frame, scope),
      )
    } catch (error: Throwable) {
      nativeHeap.free(frame.rawPtr)
      throw error
    }
  }

  public fun acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrameHandle {
    val frame = nativeHeap.alloc<mln_vulkan_owned_texture_frame>()
    frame.size = sizeOf<mln_vulkan_owned_texture_frame>().toUInt()
    try {
      Status.check(mln_vulkan_owned_texture_acquire_frame(state.requireLive(), frame.ptr))
      val scope = FrameScope()
      return VulkanOwnedTextureFrameHandle(
        this,
        frame.ptr,
        scope,
        vulkanOwnedTextureFrame(frame, scope),
      )
    } catch (error: Throwable) {
      nativeHeap.free(frame.rawPtr)
      throw error
    }
  }

  override fun close() {
    state.closeOnce(::mln_render_session_destroy)
  }

  public fun isClosed(): Boolean = state.isReleased()

  public fun map(): MapHandle = map

  internal fun nativeHandle(): CPointer<mln_render_session> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()

  internal fun releaseMetalFrame(frame: CPointer<mln_metal_owned_texture_frame>) {
    Status.check(mln_metal_owned_texture_release_frame(state.requireLive(), frame))
  }

  internal fun releaseVulkanFrame(frame: CPointer<mln_vulkan_owned_texture_frame>) {
    Status.check(mln_vulkan_owned_texture_release_frame(state.requireLive(), frame))
  }

  private fun metalOwnedTextureFrame(
    value: mln_metal_owned_texture_frame,
    scope: FrameScope,
  ): MetalOwnedTextureFrame =
    MetalOwnedTextureFrame(
      scope,
      value.generation,
      value.width,
      value.height,
      value.scale_factor,
      value.frame_id,
      pointer(value.texture),
      pointer(value.device),
      value.pixel_format,
    )

  private fun vulkanOwnedTextureFrame(
    value: mln_vulkan_owned_texture_frame,
    scope: FrameScope,
  ): VulkanOwnedTextureFrame =
    VulkanOwnedTextureFrame(
      scope,
      value.generation,
      value.width,
      value.height,
      value.scale_factor,
      value.frame_id,
      pointer(value.image),
      pointer(value.image_view),
      pointer(value.device),
      value.format,
      value.layout,
    )

  private fun pointer(pointer: kotlinx.cinterop.COpaquePointer?): NativePointer =
    pointer?.rawValue?.toLong()?.toULong()?.let(NativePointer::ofAddress) ?: NativePointer.NULL

  public companion object {
    public fun attachMetalOwnedTexture(
      map: MapHandle,
      descriptor: MetalOwnedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<CPointerVarOf<CPointer<mln_render_session>>>()
      outSession.value = null
      Status.check(
        mln_metal_owned_texture_attach(
          map.nativeHandle(),
          RenderStructs.metalOwnedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(map, requireNotNull(outSession.value))
    }

    public fun attachMetalBorrowedTexture(
      map: MapHandle,
      descriptor: MetalBorrowedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<CPointerVarOf<CPointer<mln_render_session>>>()
      outSession.value = null
      Status.check(
        mln_metal_borrowed_texture_attach(
          map.nativeHandle(),
          RenderStructs.metalBorrowedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(map, requireNotNull(outSession.value))
    }

    public fun attachVulkanOwnedTexture(
      map: MapHandle,
      descriptor: VulkanOwnedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<CPointerVarOf<CPointer<mln_render_session>>>()
      outSession.value = null
      Status.check(
        mln_vulkan_owned_texture_attach(
          map.nativeHandle(),
          RenderStructs.vulkanOwnedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(map, requireNotNull(outSession.value))
    }

    public fun attachVulkanBorrowedTexture(
      map: MapHandle,
      descriptor: VulkanBorrowedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<CPointerVarOf<CPointer<mln_render_session>>>()
      outSession.value = null
      Status.check(
        mln_vulkan_borrowed_texture_attach(
          map.nativeHandle(),
          RenderStructs.vulkanBorrowedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(map, requireNotNull(outSession.value))
    }

    public fun attachMetalSurface(
      map: MapHandle,
      descriptor: MetalSurfaceDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<CPointerVarOf<CPointer<mln_render_session>>>()
      outSession.value = null
      Status.check(
        mln_metal_surface_attach(
          map.nativeHandle(),
          RenderStructs.metalSurfaceDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(map, requireNotNull(outSession.value))
    }

    public fun attachVulkanSurface(
      map: MapHandle,
      descriptor: VulkanSurfaceDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<CPointerVarOf<CPointer<mln_render_session>>>()
      outSession.value = null
      Status.check(
        mln_vulkan_surface_attach(
          map.nativeHandle(),
          RenderStructs.vulkanSurfaceDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(map, requireNotNull(outSession.value))
    }
  }
}
