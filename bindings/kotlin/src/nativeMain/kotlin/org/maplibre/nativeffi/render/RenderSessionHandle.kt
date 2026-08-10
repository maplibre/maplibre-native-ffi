package org.maplibre.nativeffi.render

import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.ULongVar
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
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_attach
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_set_target
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_acquire_frame
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_attach
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_release_frame
import org.maplibre.nativeffi.internal.c.mln_metal_surface_attach
import org.maplibre.nativeffi.internal.c.mln_metal_surface_set_target
import org.maplibre.nativeffi.internal.c.mln_opengl_borrowed_texture_attach
import org.maplibre.nativeffi.internal.c.mln_opengl_borrowed_texture_set_target
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_acquire_frame
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_attach
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_release_frame
import org.maplibre.nativeffi.internal.c.mln_opengl_surface_attach
import org.maplibre.nativeffi.internal.c.mln_opengl_surface_set_target
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
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_set_target
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_acquire_frame
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_attach
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_release_frame
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_attach
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_set_target
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.internal.lifecycle.asHandle
import org.maplibre.nativeffi.internal.lifecycle.ownedBufferHandle
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.lifecycle.renderSessionHandle
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ByteStructs
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.QueryStructs
import org.maplibre.nativeffi.internal.struct.RenderStructs
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Owned native render session handle. Close it on the thread that attached it. */
@OptIn(ExperimentalForeignApi::class)
public actual class RenderSessionHandle
private constructor(private val map: MapHandle, handle: NativeRenderSession) : AutoCloseable {
  private val mapRetention = map.retainChild("RenderSessionHandle")
  private val state = HandleState("RenderSessionHandle", handle, map)
  private val activeFrame = ActiveFrameState()

  public actual fun resize(width: Int, height: Int, scaleFactor: Double) {
    activeFrame.ensureInactive("resize")
    Status.requireArgument(width >= 0) { "width must be non-negative" }
    Status.requireArgument(height >= 0) { "height must be non-negative" }
    Status.check(
      mln_render_session_resize(
        state.requireLive().rawHandleValue,
        width.toUInt(),
        height.toUInt(),
        scaleFactor,
      )
    )
  }

  public actual fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor) {
    activeFrame.ensureInactive("set target")
    memScoped {
      Status.check(
        mln_metal_surface_set_target(
          state.requireLive().rawHandleValue,
          RenderStructs.metalSurfaceDescriptor(descriptor, this),
        )
      )
    }
  }

  public actual fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor) {
    activeFrame.ensureInactive("set target")
    memScoped {
      Status.check(
        mln_vulkan_surface_set_target(
          state.requireLive().rawHandleValue,
          RenderStructs.vulkanSurfaceDescriptor(descriptor, this),
        )
      )
    }
  }

  public actual fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor) {
    activeFrame.ensureInactive("set target")
    memScoped {
      Status.check(
        mln_opengl_surface_set_target(
          state.requireLive().rawHandleValue,
          RenderStructs.openglSurfaceDescriptor(descriptor, this),
        )
      )
    }
  }

  public actual fun setMetalBorrowedTextureTarget(descriptor: MetalBorrowedTextureDescriptor) {
    activeFrame.ensureInactive("set target")
    memScoped {
      Status.check(
        mln_metal_borrowed_texture_set_target(
          state.requireLive().rawHandleValue,
          RenderStructs.metalBorrowedTextureDescriptor(descriptor, this),
        )
      )
    }
  }

  public actual fun setVulkanBorrowedTextureTarget(descriptor: VulkanBorrowedTextureDescriptor) {
    activeFrame.ensureInactive("set target")
    memScoped {
      Status.check(
        mln_vulkan_borrowed_texture_set_target(
          state.requireLive().rawHandleValue,
          RenderStructs.vulkanBorrowedTextureDescriptor(descriptor, this),
        )
      )
    }
  }

  public actual fun setOpenGLBorrowedTextureTarget(descriptor: OpenGLBorrowedTextureDescriptor) {
    activeFrame.ensureInactive("set target")
    memScoped {
      Status.check(
        mln_opengl_borrowed_texture_set_target(
          state.requireLive().rawHandleValue,
          RenderStructs.openglBorrowedTextureDescriptor(descriptor, this),
        )
      )
    }
  }

  public actual fun renderUpdate(): Boolean = memScoped {
    activeFrame.ensureInactive("render")
    val outRendered = alloc<BooleanVar>()
    outRendered.value = false
    Status.check(
      mln_render_session_render_update(state.requireLive().rawHandleValue, outRendered.ptr)
    )
    outRendered.value
  }

  public actual fun detach() {
    activeFrame.ensureInactive("detach")
    Status.check(mln_render_session_detach(state.requireLive().rawHandleValue))
    mapRetention.close()
  }

  public actual fun reduceMemoryUse() {
    activeFrame.ensureInactive("reduce memory use")
    Status.check(mln_render_session_reduce_memory_use(state.requireLive().rawHandleValue))
  }

  public actual fun clearData() {
    activeFrame.ensureInactive("clear data")
    Status.check(mln_render_session_clear_data(state.requireLive().rawHandleValue))
  }

  public actual fun dumpDebugLogs() {
    activeFrame.ensureInactive("dump debug logs")
    Status.check(mln_render_session_dump_debug_logs(state.requireLive().rawHandleValue))
  }

  public actual fun setFeatureState(selector: FeatureStateSelector, value: ByteArray) {
    activeFrame.ensureInactive("set feature state")
    memScoped {
      Status.check(
        mln_render_session_set_feature_state(
          state.requireLive().rawHandleValue,
          QueryStructs.featureStateSelector(selector, this),
          ByteStructs.bufferView(value, this),
        )
      )
    }
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): ByteArray = memScoped {
    activeFrame.ensureInactive("get feature state")
    val outState = alloc<ULongVar>()
    outState.value = 0uL
    Status.check(
      mln_render_session_get_feature_state(
        state.requireLive().rawHandleValue,
        QueryStructs.featureStateSelector(selector, this),
        outState.ptr,
      )
    )
    ByteStructs.ownedBuffer(outState.value.asHandle("mln_buffer", ::ownedBufferHandle))
  }

  public actual fun removeFeatureState(selector: FeatureStateSelector) {
    activeFrame.ensureInactive("remove feature state")
    memScoped {
      Status.check(
        mln_render_session_remove_feature_state(
          state.requireLive().rawHandleValue,
          QueryStructs.featureStateSelector(selector, this),
        )
      )
    }
  }

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): ByteArray = memScoped {
    activeFrame.ensureInactive("query rendered features")
    val outResult = alloc<ULongVar>()
    outResult.value = 0uL
    Status.check(
      mln_render_session_query_rendered_features(
        state.requireLive().rawHandleValue,
        QueryStructs.renderedQueryGeometry(geometry, this),
        QueryStructs.renderedFeatureQueryOptions(options, this),
        outResult.ptr,
      )
    )
    ByteStructs.ownedBuffer(outResult.value.asHandle("mln_buffer", ::ownedBufferHandle))
  }

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): ByteArray = memScoped {
    activeFrame.ensureInactive("query source features")
    val outResult = alloc<ULongVar>()
    outResult.value = 0uL
    Status.check(
      mln_render_session_query_source_features(
        state.requireLive().rawHandleValue,
        CoreStructs.stringView(sourceId, this),
        QueryStructs.sourceFeatureQueryOptions(options, this),
        outResult.ptr,
      )
    )
    ByteStructs.ownedBuffer(outResult.value.asHandle("mln_buffer", ::ownedBufferHandle))
  }

  public actual fun queryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): ByteArray = memScoped {
    activeFrame.ensureInactive("query feature extension")
    val outResult = alloc<ULongVar>()
    outResult.value = 0uL
    Status.check(
      mln_render_session_query_feature_extensions(
        state.requireLive().rawHandleValue,
        CoreStructs.stringView(sourceId, this),
        ByteStructs.bufferView(feature, this),
        CoreStructs.stringView(extension, this),
        CoreStructs.stringView(extensionField, this),
        arguments?.let { ByteStructs.bufferViewPointer(it, this) },
        outResult.ptr,
      )
    )
    ByteStructs.ownedBuffer(outResult.value.asHandle("mln_buffer", ::ownedBufferHandle))
  }

  public actual fun textureImageInfo(): TextureImageInfo = memScoped {
    activeFrame.ensureInactive("read texture data")
    val outInfo = mln_texture_image_info_default().getPointer(this)
    val status =
      mln_texture_read_premultiplied_rgba8(state.requireLive().rawHandleValue, null, 0UL, outInfo)
    val info = RenderStructs.textureImageInfo(outInfo.pointed)
    if (status == 0 || (status == -1 && info.byteLength > 0L)) {
      info
    } else {
      Status.check(status)
      error("unreachable")
    }
  }

  public actual fun readPremultipliedRgba8(buffer: NativeBuffer): TextureImageInfo = memScoped {
    activeFrame.ensureInactive("read texture data")
    val outInfo = mln_texture_image_info_default().getPointer(this)
    buffer.borrow { pointer, capacity ->
      Status.check(
        mln_texture_read_premultiplied_rgba8(
          state.requireLive().rawHandleValue,
          pointer?.reinterpret<UByteVar>(),
          capacity.toULong(),
          outInfo,
        )
      )
    }
    val info = RenderStructs.textureImageInfo(outInfo.pointed)
    // Native reads an empty destination as a size probe rather than a copy, so the
    // capacity is rechecked here.
    buffer.ensureCapacity(info.byteLength.toULong())
    info
  }

  public actual fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle {
    val frame = nativeHeap.alloc<mln_metal_owned_texture_frame>()
    var acquired = false
    var borrowStarted = false
    try {
      activeFrame.beginAcquire()
      borrowStarted = true
      frame.size = sizeOf<mln_metal_owned_texture_frame>().toUInt()
      Status.check(
        mln_metal_owned_texture_acquire_frame(state.requireLive().rawHandleValue, frame.ptr)
      )
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

  public actual fun acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrameHandle {
    val frame = nativeHeap.alloc<mln_vulkan_owned_texture_frame>()
    var acquired = false
    var borrowStarted = false
    try {
      activeFrame.beginAcquire()
      borrowStarted = true
      frame.size = sizeOf<mln_vulkan_owned_texture_frame>().toUInt()
      Status.check(
        mln_vulkan_owned_texture_acquire_frame(state.requireLive().rawHandleValue, frame.ptr)
      )
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

  public actual fun acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrameHandle {
    val frame = nativeHeap.alloc<mln_opengl_owned_texture_frame>()
    var acquired = false
    var borrowStarted = false
    try {
      activeFrame.beginAcquire()
      borrowStarted = true
      frame.size = sizeOf<mln_opengl_owned_texture_frame>().toUInt()
      Status.check(
        mln_opengl_owned_texture_acquire_frame(state.requireLive().rawHandleValue, frame.ptr)
      )
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

  public actual override fun close() {
    activeFrame.ensureInactive("destroy")
    state.closeOnce({ handle -> mln_render_session_destroy(handle.rawHandleValue) }) {
      mapRetention.close()
    }
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()

  public actual fun map(): MapHandle = map

  internal fun nativeHandle(): NativeRenderSession = state.requireLive()

  internal fun nativeHandleId(): Long = state.handleId()

  internal fun releaseMetalFrame(frame: CPointer<mln_metal_owned_texture_frame>) {
    Status.check(mln_metal_owned_texture_release_frame(state.requireLive().rawHandleValue, frame))
  }

  internal fun releaseVulkanFrame(frame: CPointer<mln_vulkan_owned_texture_frame>) {
    Status.check(mln_vulkan_owned_texture_release_frame(state.requireLive().rawHandleValue, frame))
  }

  internal fun releaseOpenGLFrame(frame: CPointer<mln_opengl_owned_texture_frame>) {
    Status.check(mln_opengl_owned_texture_release_frame(state.requireLive().rawHandleValue, frame))
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
      val outSession = alloc<ULongVar>()
      outSession.value = 0uL
      Status.check(
        mln_metal_owned_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.metalOwnedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(
        map,
        outSession.value.asHandle("mln_render_session", ::renderSessionHandle),
      )
    }

    internal fun attachMetalBorrowedTexture(
      map: MapHandle,
      descriptor: MetalBorrowedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<ULongVar>()
      outSession.value = 0uL
      Status.check(
        mln_metal_borrowed_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.metalBorrowedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(
        map,
        outSession.value.asHandle("mln_render_session", ::renderSessionHandle),
      )
    }

    internal fun attachVulkanOwnedTexture(
      map: MapHandle,
      descriptor: VulkanOwnedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<ULongVar>()
      outSession.value = 0uL
      Status.check(
        mln_vulkan_owned_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.vulkanOwnedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(
        map,
        outSession.value.asHandle("mln_render_session", ::renderSessionHandle),
      )
    }

    internal fun attachVulkanBorrowedTexture(
      map: MapHandle,
      descriptor: VulkanBorrowedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<ULongVar>()
      outSession.value = 0uL
      Status.check(
        mln_vulkan_borrowed_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.vulkanBorrowedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(
        map,
        outSession.value.asHandle("mln_render_session", ::renderSessionHandle),
      )
    }

    internal fun attachOpenGLOwnedTexture(
      map: MapHandle,
      descriptor: OpenGLOwnedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<ULongVar>()
      outSession.value = 0uL
      Status.check(
        mln_opengl_owned_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.openglOwnedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(
        map,
        outSession.value.asHandle("mln_render_session", ::renderSessionHandle),
      )
    }

    internal fun attachOpenGLBorrowedTexture(
      map: MapHandle,
      descriptor: OpenGLBorrowedTextureDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<ULongVar>()
      outSession.value = 0uL
      Status.check(
        mln_opengl_borrowed_texture_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.openglBorrowedTextureDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(
        map,
        outSession.value.asHandle("mln_render_session", ::renderSessionHandle),
      )
    }

    internal fun attachMetalSurface(
      map: MapHandle,
      descriptor: MetalSurfaceDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<ULongVar>()
      outSession.value = 0uL
      Status.check(
        mln_metal_surface_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.metalSurfaceDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(
        map,
        outSession.value.asHandle("mln_render_session", ::renderSessionHandle),
      )
    }

    internal fun attachVulkanSurface(
      map: MapHandle,
      descriptor: VulkanSurfaceDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<ULongVar>()
      outSession.value = 0uL
      Status.check(
        mln_vulkan_surface_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.vulkanSurfaceDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(
        map,
        outSession.value.asHandle("mln_render_session", ::renderSessionHandle),
      )
    }

    internal fun attachOpenGLSurface(
      map: MapHandle,
      descriptor: OpenGLSurfaceDescriptor,
    ): RenderSessionHandle = memScoped {
      val outSession = alloc<ULongVar>()
      outSession.value = 0uL
      Status.check(
        mln_opengl_surface_attach(
          map.nativeHandle().rawHandleValue,
          RenderStructs.openglSurfaceDescriptor(descriptor, this),
          outSession.ptr,
        )
      )
      RenderSessionHandle(
        map,
        outSession.value.asHandle("mln_render_session", ::renderSessionHandle),
      )
    }
  }
}
