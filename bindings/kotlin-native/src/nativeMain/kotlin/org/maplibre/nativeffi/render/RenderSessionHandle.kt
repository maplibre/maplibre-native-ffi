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
import org.maplibre.nativeffi.internal.c.mln_opengl_borrowed_texture_attach
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_acquire_frame
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_attach
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_release_frame
import org.maplibre.nativeffi.internal.c.mln_opengl_surface_attach
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
  private val activeFrame = ActiveFrameState()

  public fun resize(width: Int, height: Int, scaleFactor: Double) {
    activeFrame.ensureInactive("resize")
    require(width >= 0) { "width must be non-negative" }
    require(height >= 0) { "height must be non-negative" }
    Status.check(
      mln_render_session_resize(state.requireLive(), width.toUInt(), height.toUInt(), scaleFactor)
    )
  }

  public fun renderUpdate() {
    activeFrame.ensureInactive("render")
    Status.check(mln_render_session_render_update(state.requireLive()))
  }

  public fun detach() {
    activeFrame.ensureInactive("detach")
    Status.check(mln_render_session_detach(state.requireLive()))
  }

  public fun reduceMemoryUse() {
    activeFrame.ensureInactive("reduce memory use")
    Status.check(mln_render_session_reduce_memory_use(state.requireLive()))
  }

  public fun clearData() {
    activeFrame.ensureInactive("clear data")
    Status.check(mln_render_session_clear_data(state.requireLive()))
  }

  public fun dumpDebugLogs() {
    activeFrame.ensureInactive("dump debug logs")
    Status.check(mln_render_session_dump_debug_logs(state.requireLive()))
  }

  public fun setFeatureState(selector: FeatureStateSelector, value: JsonValue) {
    activeFrame.ensureInactive("set feature state")
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
    activeFrame.ensureInactive("get feature state")
    val outState = alloc<CPointerVarOf<CPointer<mln_json_snapshot>>>()
    outState.value = null
    Status.check(
      mln_render_session_get_feature_state(
        state.requireLive(),
        QueryStructs.featureStateSelector(selector, this),
        outState.ptr,
      )
    )
    ValueStructs.jsonSnapshotHandle(outState.value) ?: JsonValue.ObjectValue(emptyList())
  }

  public fun removeFeatureState(selector: FeatureStateSelector) {
    activeFrame.ensureInactive("remove feature state")
    memScoped {
      Status.check(
        mln_render_session_remove_feature_state(
          state.requireLive(),
          QueryStructs.featureStateSelector(selector, this),
        )
      )
    }
  }

  public fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature> = memScoped {
    activeFrame.ensureInactive("query rendered features")
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

  public fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature> = memScoped {
    activeFrame.ensureInactive("query source features")
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
    arguments: JsonValue?,
  ): FeatureExtensionResult = memScoped {
    activeFrame.ensureInactive("query feature extension")
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
    activeFrame.ensureInactive("read texture data")
    val outInfo = mln_texture_image_info_default().getPointer(this)
    val status = mln_texture_read_premultiplied_rgba8(state.requireLive(), null, 0UL, outInfo)
    val info = RenderStructs.textureImageInfo(outInfo.pointed)
    if (status == 0 || (status == -1 && info.byteLength > 0L)) {
      info
    } else {
      Status.check(status)
      error("unreachable")
    }
  }

  public fun readPremultipliedRgba8(buffer: NativeBuffer): TextureImageInfo = memScoped {
    activeFrame.ensureInactive("read texture data")
    val outInfo = mln_texture_image_info_default().getPointer(this)
    buffer.borrow { pointer, capacity ->
      Status.check(
        mln_texture_read_premultiplied_rgba8(
          state.requireLive(),
          pointer?.reinterpret<UByteVar>(),
          capacity.toULong(),
          outInfo,
        )
      )
    }
    RenderStructs.textureImageInfo(outInfo.pointed)
  }

  public fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle {
    val frame = nativeHeap.alloc<mln_metal_owned_texture_frame>()
    var acquired = false
    var borrowStarted = false
    try {
      activeFrame.beginAcquire()
      borrowStarted = true
      frame.size = sizeOf<mln_metal_owned_texture_frame>().toUInt()
      Status.check(mln_metal_owned_texture_acquire_frame(state.requireLive(), frame.ptr))
      acquired = true
      val scope = FrameScope()
      return MetalOwnedTextureFrameHandle(
        this,
        frame.ptr,
        scope,
        metalOwnedTextureFrame(frame, scope),
      )
    } catch (error: Throwable) {
      FrameAcquirePolicy.cleanupAfterWrapperFailure(
        acquired,
        releaseNative = { releaseMetalFrame(frame.ptr) },
        closeLocal = {
          if (borrowStarted) activeFrame.endBorrow()
          nativeHeap.free(frame.rawPtr)
        },
        failure = error,
      )
    }
  }

  public fun acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrameHandle {
    val frame = nativeHeap.alloc<mln_vulkan_owned_texture_frame>()
    var acquired = false
    var borrowStarted = false
    try {
      activeFrame.beginAcquire()
      borrowStarted = true
      frame.size = sizeOf<mln_vulkan_owned_texture_frame>().toUInt()
      Status.check(mln_vulkan_owned_texture_acquire_frame(state.requireLive(), frame.ptr))
      acquired = true
      val scope = FrameScope()
      return VulkanOwnedTextureFrameHandle(
        this,
        frame.ptr,
        scope,
        vulkanOwnedTextureFrame(frame, scope),
      )
    } catch (error: Throwable) {
      FrameAcquirePolicy.cleanupAfterWrapperFailure(
        acquired,
        releaseNative = { releaseVulkanFrame(frame.ptr) },
        closeLocal = {
          if (borrowStarted) activeFrame.endBorrow()
          nativeHeap.free(frame.rawPtr)
        },
        failure = error,
      )
    }
  }

  public fun acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrameHandle {
    val frame = nativeHeap.alloc<mln_opengl_owned_texture_frame>()
    var acquired = false
    var borrowStarted = false
    try {
      activeFrame.beginAcquire()
      borrowStarted = true
      frame.size = sizeOf<mln_opengl_owned_texture_frame>().toUInt()
      Status.check(mln_opengl_owned_texture_acquire_frame(state.requireLive(), frame.ptr))
      acquired = true
      val scope = FrameScope()
      return OpenGLOwnedTextureFrameHandle(
        this,
        frame.ptr,
        scope,
        openglOwnedTextureFrame(frame, scope),
      )
    } catch (error: Throwable) {
      FrameAcquirePolicy.cleanupAfterWrapperFailure(
        acquired,
        releaseNative = { releaseOpenGLFrame(frame.ptr) },
        closeLocal = {
          if (borrowStarted) activeFrame.endBorrow()
          nativeHeap.free(frame.rawPtr)
        },
        failure = error,
      )
    }
  }

  override fun close() {
    activeFrame.ensureInactive("destroy")
    state.closeOnce(::mln_render_session_destroy)
  }

  public val isClosed: Boolean
    get() = state.isReleased()

  public fun map(): MapHandle = map

  internal fun nativeHandle(): CPointer<mln_render_session> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()

  internal fun releaseMetalFrame(frame: CPointer<mln_metal_owned_texture_frame>) {
    Status.check(mln_metal_owned_texture_release_frame(state.requireLive(), frame))
  }

  internal fun releaseVulkanFrame(frame: CPointer<mln_vulkan_owned_texture_frame>) {
    Status.check(mln_vulkan_owned_texture_release_frame(state.requireLive(), frame))
  }

  internal fun releaseOpenGLFrame(frame: CPointer<mln_opengl_owned_texture_frame>) {
    Status.check(mln_opengl_owned_texture_release_frame(state.requireLive(), frame))
  }

  internal fun finishFrameBorrow() {
    activeFrame.endBorrow()
  }

  private fun metalOwnedTextureFrame(
    value: mln_metal_owned_texture_frame,
    scope: FrameScope,
  ): MetalOwnedTextureFrame =
    MetalOwnedTextureFrame(
      scope,
      uint64BitsToLong(value.generation),
      checkedInt(value.width, "Metal frame width"),
      checkedInt(value.height, "Metal frame height"),
      value.scale_factor,
      uint64BitsToLong(value.frame_id),
      scopedPointer(value.texture, scope),
      scopedPointer(value.device, scope),
      uint64BitsToLong(value.pixel_format),
    )

  private fun vulkanOwnedTextureFrame(
    value: mln_vulkan_owned_texture_frame,
    scope: FrameScope,
  ): VulkanOwnedTextureFrame =
    VulkanOwnedTextureFrame(
      scope,
      uint64BitsToLong(value.generation),
      checkedInt(value.width, "Vulkan frame width"),
      checkedInt(value.height, "Vulkan frame height"),
      value.scale_factor,
      uint64BitsToLong(value.frame_id),
      scopedPointer(value.image, scope),
      scopedPointer(value.image_view, scope),
      scopedPointer(value.device, scope),
      value.format.toInt(),
      value.layout.toInt(),
    )

  private fun openglOwnedTextureFrame(
    value: mln_opengl_owned_texture_frame,
    scope: FrameScope,
  ): OpenGLOwnedTextureFrame =
    OpenGLOwnedTextureFrame(
      scope,
      uint64BitsToLong(value.generation),
      checkedInt(value.width, "OpenGL frame width"),
      checkedInt(value.height, "OpenGL frame height"),
      value.scale_factor,
      uint64BitsToLong(value.frame_id),
      value.texture.toInt(),
      value.target.toInt(),
      value.internal_format.toInt(),
      value.format.toInt(),
      value.type.toInt(),
    )

  private fun checkedInt(value: UInt, name: String): Int {
    require(value <= Int.MAX_VALUE.toUInt()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  private fun uint64BitsToLong(value: ULong): Long = value.toLong()

  private fun scopedPointer(
    pointer: kotlinx.cinterop.COpaquePointer?,
    scope: FrameScope,
  ): NativePointer =
    pointer?.rawValue?.toLong()?.let { NativePointer.scoped(it, scope) } ?: NativePointer.NULL

  internal companion object {
    internal fun attachMetalOwnedTexture(
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

    internal fun attachMetalBorrowedTexture(
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

    internal fun attachVulkanOwnedTexture(
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

    internal fun attachVulkanBorrowedTexture(
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

    internal fun attachOpenGLOwnedTexture(
      map: MapHandle,
      descriptor: OpenGLOwnedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<CPointerVarOf<CPointer<mln_render_session>>>()
      outSession.value = null
      Status.check(
        mln_opengl_owned_texture_attach(
          map.nativeHandle(),
          RenderStructs.openglOwnedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(map, requireNotNull(outSession.value))
    }

    internal fun attachOpenGLBorrowedTexture(
      map: MapHandle,
      descriptor: OpenGLBorrowedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<CPointerVarOf<CPointer<mln_render_session>>>()
      outSession.value = null
      Status.check(
        mln_opengl_borrowed_texture_attach(
          map.nativeHandle(),
          RenderStructs.openglBorrowedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(map, requireNotNull(outSession.value))
    }

    internal fun attachMetalSurface(
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

    internal fun attachVulkanSurface(
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

    internal fun attachOpenGLSurface(
      map: MapHandle,
      descriptor: OpenGLSurfaceDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<CPointerVarOf<CPointer<mln_render_session>>>()
      outSession.value = null
      Status.check(
        mln_opengl_surface_attach(
          map.nativeHandle(),
          RenderStructs.openglSurfaceDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(map, requireNotNull(outSession.value))
    }
  }
}
