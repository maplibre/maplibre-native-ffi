package org.maplibre.nativeffi.render

import java.nio.charset.StandardCharsets
import org.bytedeco.javacpp.BoolPointer
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.LongPointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.javacpp.ownedBuffer
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Owned Android JNI render session handle. Close it on the thread that attached it. */
public actual class RenderSessionHandle
private constructor(private val map: MapHandle, private val handleId: Long) : AutoCloseable {
  private val mapRetention = map.retainChild("RenderSessionHandle")
  private val core = HandleStateCore("RenderSessionHandle", handleId, map)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private val activeFrame = ActiveFrameState()

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun map(): MapHandle = map

  public actual fun resize(width: Int, height: Int, scaleFactor: Double) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("resize")
    Status.requireArgument(width >= 0) { "width must be non-negative" }
    Status.requireArgument(height >= 0) { "height must be non-negative" }
    Status.check(
      MaplibreNativeC.mln_render_session_resize(requireLiveHandle(), width, height, scaleFactor)
    )
  }

  public actual fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("set target")
    Status.check(
      MaplibreNativeC.mln_metal_surface_set_target(
        requireLiveHandle(),
        metalSurfaceDescriptor(descriptor),
      )
    )
  }

  public actual fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("set target")
    Status.check(
      MaplibreNativeC.mln_vulkan_surface_set_target(
        requireLiveHandle(),
        vulkanSurfaceDescriptor(descriptor),
      )
    )
  }

  public actual fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("set target")
    Status.check(
      MaplibreNativeC.mln_opengl_surface_set_target(
        requireLiveHandle(),
        openglSurfaceDescriptor(descriptor),
      )
    )
  }

  public actual fun setMetalBorrowedTextureTarget(descriptor: MetalBorrowedTextureDescriptor) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("set target")
    Status.check(
      MaplibreNativeC.mln_metal_borrowed_texture_set_target(
        requireLiveHandle(),
        metalBorrowedTextureDescriptor(descriptor),
      )
    )
  }

  public actual fun setVulkanBorrowedTextureTarget(descriptor: VulkanBorrowedTextureDescriptor) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("set target")
    Status.check(
      MaplibreNativeC.mln_vulkan_borrowed_texture_set_target(
        requireLiveHandle(),
        vulkanBorrowedTextureDescriptor(descriptor),
      )
    )
  }

  public actual fun setOpenGLBorrowedTextureTarget(descriptor: OpenGLBorrowedTextureDescriptor) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("set target")
    Status.check(
      MaplibreNativeC.mln_opengl_borrowed_texture_set_target(
        requireLiveHandle(),
        openglBorrowedTextureDescriptor(descriptor),
      )
    )
  }

  public actual fun renderUpdate(): Boolean {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("render")
    BoolPointer(1).use { outRendered ->
      outRendered.put(0, false)
      Status.check(
        MaplibreNativeC.mln_render_session_render_update(requireLiveHandle(), outRendered)
      )
      return outRendered.get()
    }
  }

  public actual fun detach() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("detach")
    Status.check(MaplibreNativeC.mln_render_session_detach(requireLiveHandle()))
    mapRetention.close()
  }

  public actual fun reduceMemoryUse() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("reduce memory use")
    Status.check(MaplibreNativeC.mln_render_session_reduce_memory_use(requireLiveHandle()))
  }

  public actual fun clearData() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("clear data")
    Status.check(MaplibreNativeC.mln_render_session_clear_data(requireLiveHandle()))
  }

  public actual fun dumpDebugLogs() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("dump debug logs")
    Status.check(MaplibreNativeC.mln_render_session_dump_debug_logs(requireLiveHandle()))
  }

  public actual fun setFeatureState(selector: FeatureStateSelector, value: ByteArray) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("set feature state")
    FeatureStateSelectorScope(selector).use { nativeSelector ->
      ByteArrayViewScope(value).use { nativeValue ->
        Status.check(
          MaplibreNativeC.mln_render_session_set_feature_state(
            requireLiveHandle(),
            nativeSelector.selector,
            nativeValue.view,
          )
        )
      }
    }
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): ByteArray {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("get feature state")
    FeatureStateSelectorScope(selector).use { nativeSelector ->
      LongPointer(1).use { outState ->
        outState.put(0, 0L)
        Status.check(
          MaplibreNativeC.mln_render_session_get_feature_state(
            requireLiveHandle(),
            nativeSelector.selector,
            outState,
          )
        )
        return ownedBuffer(outState.get())
      }
    }
  }

  public actual fun removeFeatureState(selector: FeatureStateSelector) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("remove feature state")
    FeatureStateSelectorScope(selector).use { nativeSelector ->
      Status.check(
        MaplibreNativeC.mln_render_session_remove_feature_state(
          requireLiveHandle(),
          nativeSelector.selector,
        )
      )
    }
  }

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): ByteArray {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("query rendered features")
    RenderedQueryGeometryScope(geometry).use { nativeGeometry ->
      RenderedFeatureQueryOptionsScope(options).use { nativeOptions ->
        LongPointer(1).use { outResult ->
          outResult.put(0, 0L)
          Status.check(
            MaplibreNativeC.mln_render_session_query_rendered_features(
              requireLiveHandle(),
              nativeGeometry.geometry,
              nativeOptions.options,
              outResult,
            )
          )
          return ownedBuffer(outResult.get())
        }
      }
    }
  }

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): ByteArray {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("query source features")
    StringViewScope(sourceId).use { nativeSourceId ->
      SourceFeatureQueryOptionsScope(options).use { nativeOptions ->
        LongPointer(1).use { outResult ->
          outResult.put(0, 0L)
          Status.check(
            MaplibreNativeC.mln_render_session_query_source_features(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeOptions.options,
              outResult,
            )
          )
          return ownedBuffer(outResult.get())
        }
      }
    }
  }

  public actual fun queryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): ByteArray {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("query feature extension")
    StringViewScope(sourceId).use { nativeSourceId ->
      ByteArrayViewScope(feature).use { nativeFeature ->
        StringViewScope(extension).use { nativeExtension ->
          StringViewScope(extensionField).use { nativeExtensionField ->
            ByteArrayViewScope(arguments ?: byteArrayOf()).use { nativeArguments ->
              LongPointer(1).use { outResult ->
                outResult.put(0, 0L)
                Status.check(
                  MaplibreNativeC.mln_render_session_query_feature_extensions(
                    requireLiveHandle(),
                    nativeSourceId.view,
                    nativeFeature.view,
                    nativeExtension.view,
                    nativeExtensionField.view,
                    if (arguments == null) null else nativeArguments.view,
                    outResult,
                  )
                )
                return ownedBuffer(outResult.get())
              }
            }
          }
        }
      }
    }
  }

  public actual fun textureImageInfo(): TextureImageInfo {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("read texture data")
    val outInfo = MaplibreNativeC.mln_texture_image_info_default()
    val status =
      MaplibreNativeC.mln_texture_read_premultiplied_rgba8(
        requireLiveHandle(),
        null as BytePointer?,
        0L,
        outInfo,
      )
    val info = textureImageInfo(outInfo)
    if (
      status == MaplibreStatus.OK.nativeCode ||
        (status == MaplibreStatus.INVALID_ARGUMENT.nativeCode && info.byteLength > 0)
    ) {
      return info
    }
    Status.check(status)
    error("unreachable")
  }

  public actual fun readPremultipliedRgba8(buffer: NativeBuffer): TextureImageInfo {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("read texture data")
    val outInfo = MaplibreNativeC.mln_texture_image_info_default()
    Status.check(
      MaplibreNativeC.mln_texture_read_premultiplied_rgba8(
        requireLiveHandle(),
        buffer.borrowBuffer(),
        buffer.byteLength(),
        outInfo,
      )
    )
    val info = textureImageInfo(outInfo)
    // Native reads an empty destination as a size probe rather than a copy, so the
    // capacity is rechecked here.
    buffer.ensureCapacity(info.byteLength)
    return info
  }

  public actual fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle {
    NativeAccess.ensureLoaded()
    activeFrame.beginAcquire()
    val nativeFrame = MaplibreNativeC.mln_metal_owned_texture_frame()
    nativeFrame.size(nativeFrame.sizeof())
    try {
      Status.check(
        MaplibreNativeC.mln_metal_owned_texture_acquire_frame(requireLiveHandle(), nativeFrame)
      )
      val scope = FrameScope()
      return MetalOwnedTextureFrameHandle(
        this,
        nativeFrame,
        scope,
        metalOwnedTextureFrame(nativeFrame, scope),
      )
    } catch (error: Throwable) {
      activeFrame.endBorrow()
      nativeFrame.close()
      throw error
    }
  }

  public actual fun acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrameHandle {
    NativeAccess.ensureLoaded()
    activeFrame.beginAcquire()
    val nativeFrame = MaplibreNativeC.mln_vulkan_owned_texture_frame()
    nativeFrame.size(nativeFrame.sizeof())
    try {
      Status.check(
        MaplibreNativeC.mln_vulkan_owned_texture_acquire_frame(requireLiveHandle(), nativeFrame)
      )
      val scope = FrameScope()
      return VulkanOwnedTextureFrameHandle(
        this,
        nativeFrame,
        scope,
        vulkanOwnedTextureFrame(nativeFrame, scope),
      )
    } catch (error: Throwable) {
      activeFrame.endBorrow()
      nativeFrame.close()
      throw error
    }
  }

  public actual fun acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrameHandle {
    NativeAccess.ensureLoaded()
    activeFrame.beginAcquire()
    val nativeFrame = MaplibreNativeC.mln_opengl_owned_texture_frame()
    nativeFrame.size(nativeFrame.sizeof())
    try {
      Status.check(
        MaplibreNativeC.mln_opengl_owned_texture_acquire_frame(requireLiveHandle(), nativeFrame)
      )
      val scope = FrameScope()
      return OpenGLOwnedTextureFrameHandle(
        this,
        nativeFrame,
        scope,
        openglOwnedTextureFrame(nativeFrame, scope),
      )
    } catch (error: Throwable) {
      activeFrame.endBorrow()
      nativeFrame.close()
      throw error
    }
  }

  public actual override fun close() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("destroy")
    core.closeOnce(
      destroy = { MaplibreNativeC.mln_render_session_destroy(handleId) },
      afterSuccess = { mapRetention.close() },
    )
  }

  private fun requireLiveHandle(): Long {
    core.requireLive()
    return handleId
  }

  internal fun releaseMetalFrame(frame: MaplibreNativeC.mln_metal_owned_texture_frame) {
    Status.check(MaplibreNativeC.mln_metal_owned_texture_release_frame(requireLiveHandle(), frame))
  }

  internal fun releaseVulkanFrame(frame: MaplibreNativeC.mln_vulkan_owned_texture_frame) {
    Status.check(MaplibreNativeC.mln_vulkan_owned_texture_release_frame(requireLiveHandle(), frame))
  }

  internal fun releaseOpenGLFrame(frame: MaplibreNativeC.mln_opengl_owned_texture_frame) {
    Status.check(MaplibreNativeC.mln_opengl_owned_texture_release_frame(requireLiveHandle(), frame))
  }

  internal fun finishFrameBorrow() {
    activeFrame.endBorrow()
  }

  public companion object {
    internal fun attachMetalOwnedTexture(
      map: MapHandle,
      descriptor: MetalOwnedTextureDescriptor,
    ): RenderSessionHandle =
      attach(map) { outSession ->
        MaplibreNativeC.mln_metal_owned_texture_attach(
          map.nativeHandleId(),
          metalOwnedTextureDescriptor(descriptor),
          outSession,
        )
      }

    internal fun attachMetalBorrowedTexture(
      map: MapHandle,
      descriptor: MetalBorrowedTextureDescriptor,
    ): RenderSessionHandle =
      attach(map) { outSession ->
        MaplibreNativeC.mln_metal_borrowed_texture_attach(
          map.nativeHandleId(),
          metalBorrowedTextureDescriptor(descriptor),
          outSession,
        )
      }

    internal fun attachVulkanOwnedTexture(
      map: MapHandle,
      descriptor: VulkanOwnedTextureDescriptor,
    ): RenderSessionHandle =
      attach(map) { outSession ->
        MaplibreNativeC.mln_vulkan_owned_texture_attach(
          map.nativeHandleId(),
          vulkanOwnedTextureDescriptor(descriptor),
          outSession,
        )
      }

    internal fun attachVulkanBorrowedTexture(
      map: MapHandle,
      descriptor: VulkanBorrowedTextureDescriptor,
    ): RenderSessionHandle =
      attach(map) { outSession ->
        MaplibreNativeC.mln_vulkan_borrowed_texture_attach(
          map.nativeHandleId(),
          vulkanBorrowedTextureDescriptor(descriptor),
          outSession,
        )
      }

    internal fun attachOpenGLOwnedTexture(
      map: MapHandle,
      descriptor: OpenGLOwnedTextureDescriptor,
    ): RenderSessionHandle =
      attach(map) { outSession ->
        MaplibreNativeC.mln_opengl_owned_texture_attach(
          map.nativeHandleId(),
          openglOwnedTextureDescriptor(descriptor),
          outSession,
        )
      }

    internal fun attachOpenGLBorrowedTexture(
      map: MapHandle,
      descriptor: OpenGLBorrowedTextureDescriptor,
    ): RenderSessionHandle =
      attach(map) { outSession ->
        MaplibreNativeC.mln_opengl_borrowed_texture_attach(
          map.nativeHandleId(),
          openglBorrowedTextureDescriptor(descriptor),
          outSession,
        )
      }

    internal fun attachMetalSurface(
      map: MapHandle,
      descriptor: MetalSurfaceDescriptor,
    ): RenderSessionHandle =
      attach(map) { outSession ->
        MaplibreNativeC.mln_metal_surface_attach(
          map.nativeHandleId(),
          metalSurfaceDescriptor(descriptor),
          outSession,
        )
      }

    internal fun attachVulkanSurface(
      map: MapHandle,
      descriptor: VulkanSurfaceDescriptor,
    ): RenderSessionHandle =
      attach(map) { outSession ->
        MaplibreNativeC.mln_vulkan_surface_attach(
          map.nativeHandleId(),
          vulkanSurfaceDescriptor(descriptor),
          outSession,
        )
      }

    internal fun attachOpenGLSurface(
      map: MapHandle,
      descriptor: OpenGLSurfaceDescriptor,
    ): RenderSessionHandle =
      attach(map) { outSession ->
        MaplibreNativeC.mln_opengl_surface_attach(
          map.nativeHandleId(),
          openglSurfaceDescriptor(descriptor),
          outSession,
        )
      }

    private fun attach(map: MapHandle, block: (LongPointer) -> Int): RenderSessionHandle {
      NativeAccess.ensureLoaded()
      LongPointer(1).use { outSession ->
        outSession.put(0, 0L)
        Status.check(block(outSession))
        val session = outSession.get()
        require(session != 0L) { "render session attach returned a null session" }
        return RenderSessionHandle(map, session)
      }
    }
  }
}

private fun metalOwnedTextureDescriptor(
  descriptor: MetalOwnedTextureDescriptor
): MaplibreNativeC.mln_metal_owned_texture_descriptor =
  MaplibreNativeC.mln_metal_owned_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    context().device(pointerOrNull(descriptor.context.device))
  }

private fun metalBorrowedTextureDescriptor(
  descriptor: MetalBorrowedTextureDescriptor
): MaplibreNativeC.mln_metal_borrowed_texture_descriptor =
  MaplibreNativeC.mln_metal_borrowed_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    physical_width(descriptor.physicalWidth)
    physical_height(descriptor.physicalHeight)
    texture(pointerOrNull(descriptor.texture))
  }

private fun metalSurfaceDescriptor(
  descriptor: MetalSurfaceDescriptor
): MaplibreNativeC.mln_metal_surface_descriptor =
  MaplibreNativeC.mln_metal_surface_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    context().device(pointerOrNull(descriptor.context.device))
    layer(pointerOrNull(descriptor.layer))
  }

private fun vulkanOwnedTextureDescriptor(
  descriptor: VulkanOwnedTextureDescriptor
): MaplibreNativeC.mln_vulkan_owned_texture_descriptor =
  MaplibreNativeC.mln_vulkan_owned_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    setVulkanContext(context(), descriptor.context)
  }

private fun vulkanBorrowedTextureDescriptor(
  descriptor: VulkanBorrowedTextureDescriptor
): MaplibreNativeC.mln_vulkan_borrowed_texture_descriptor =
  MaplibreNativeC.mln_vulkan_borrowed_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    physical_width(descriptor.physicalWidth)
    physical_height(descriptor.physicalHeight)
    setVulkanContext(context(), descriptor.context)
    image(pointerOrNull(descriptor.image))
    image_view(pointerOrNull(descriptor.imageView))
    format(descriptor.format)
    initial_layout(descriptor.initialLayout)
    descriptor.finalLayout?.let { final_layout(it) }
  }

private fun vulkanSurfaceDescriptor(
  descriptor: VulkanSurfaceDescriptor
): MaplibreNativeC.mln_vulkan_surface_descriptor =
  MaplibreNativeC.mln_vulkan_surface_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    setVulkanContext(context(), descriptor.context)
    surface(pointerOrNull(descriptor.surface))
  }

private fun openglOwnedTextureDescriptor(
  descriptor: OpenGLOwnedTextureDescriptor
): MaplibreNativeC.mln_opengl_owned_texture_descriptor =
  MaplibreNativeC.mln_opengl_owned_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    setOpenGLContext(context(), descriptor.context)
  }

private fun openglBorrowedTextureDescriptor(
  descriptor: OpenGLBorrowedTextureDescriptor
): MaplibreNativeC.mln_opengl_borrowed_texture_descriptor =
  MaplibreNativeC.mln_opengl_borrowed_texture_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    physical_width(descriptor.physicalWidth)
    physical_height(descriptor.physicalHeight)
    setOpenGLContext(context(), descriptor.context)
    texture(descriptor.texture)
    target(descriptor.target)
  }

private fun openglSurfaceDescriptor(
  descriptor: OpenGLSurfaceDescriptor
): MaplibreNativeC.mln_opengl_surface_descriptor =
  MaplibreNativeC.mln_opengl_surface_descriptor_default().apply {
    setExtent(extent(), descriptor.extent)
    setOpenGLContext(context(), descriptor.context)
    surface(pointerOrNull(descriptor.surface))
  }

private fun setExtent(out: MaplibreNativeC.mln_render_target_extent, extent: RenderTargetExtent) {
  out.width(extent.width)
  out.height(extent.height)
  out.scale_factor(extent.scaleFactor)
}

private fun setVulkanContext(
  out: MaplibreNativeC.mln_vulkan_context_descriptor,
  context: VulkanContextDescriptor,
) {
  out.instance(pointerOrNull(context.instance))
  out.physical_device(pointerOrNull(context.physicalDevice))
  out.device(pointerOrNull(context.device))
  out.graphics_queue(pointerOrNull(context.graphicsQueue))
  out.graphics_queue_family_index(context.graphicsQueueFamilyIndex)
  out.get_instance_proc_addr(pointerOrNull(context.getInstanceProcAddr))
  out.get_device_proc_addr(pointerOrNull(context.getDeviceProcAddr))
}

private fun setOpenGLContext(
  out: MaplibreNativeC.mln_opengl_context_descriptor,
  context: OpenGLContextDescriptor,
) {
  out.size(out.sizeof())
  when (context) {
    is WglContextDescriptor -> {
      out.platform(MaplibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_WGL)
      out.data_wgl().apply {
        size(sizeof())
        device_context(pointerOrNull(context.deviceContext))
        share_context(pointerOrNull(context.shareContext))
        get_proc_address(pointerOrNull(context.getProcAddress))
      }
    }
    is EglContextDescriptor -> {
      out.platform(MaplibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_EGL)
      out.data_egl().apply {
        size(sizeof())
        display(pointerOrNull(context.display))
        config(pointerOrNull(context.config))
        share_context(pointerOrNull(context.shareContext))
        get_proc_address(pointerOrNull(context.getProcAddress))
      }
    }
  }
}

private fun textureImageInfo(info: MaplibreNativeC.mln_texture_image_info): TextureImageInfo =
  TextureImageInfo(
    info.width(),
    info.height(),
    info.stride(),
    checkedSizeT(info.byte_length(), "texture image byte length"),
  )

private fun checkedSizeT(value: Long, name: String): Long {
  require(value >= 0L) { "$name exceeds Long.MAX_VALUE" }
  return value
}

private fun metalOwnedTextureFrame(
  frame: MaplibreNativeC.mln_metal_owned_texture_frame,
  scope: FrameScope,
): MetalOwnedTextureFrame =
  MetalOwnedTextureFrame(
    scope,
    frame.generation(),
    frame.width(),
    frame.height(),
    frame.scale_factor(),
    frame.frame_id(),
    NativePointer.scoped(address(frame.texture()), scope),
    NativePointer.scoped(address(frame.device()), scope),
    frame.pixel_format(),
  )

private fun vulkanOwnedTextureFrame(
  frame: MaplibreNativeC.mln_vulkan_owned_texture_frame,
  scope: FrameScope,
): VulkanOwnedTextureFrame =
  VulkanOwnedTextureFrame(
    scope,
    frame.generation(),
    frame.width(),
    frame.height(),
    frame.scale_factor(),
    frame.frame_id(),
    NativePointer.scoped(address(frame.image()), scope),
    NativePointer.scoped(address(frame.image_view()), scope),
    NativePointer.scoped(address(frame.device()), scope),
    frame.format(),
    frame.layout(),
  )

private fun openglOwnedTextureFrame(
  frame: MaplibreNativeC.mln_opengl_owned_texture_frame,
  scope: FrameScope,
): OpenGLOwnedTextureFrame =
  OpenGLOwnedTextureFrame(
    scope,
    frame.generation(),
    frame.width(),
    frame.height(),
    frame.scale_factor(),
    frame.frame_id(),
    frame.texture(),
    frame.target(),
    frame.internal_format(),
    frame.format(),
    frame.type(),
  )

private fun address(pointer: Pointer?): Long =
  if (pointer == null || pointer.isNull) 0L else pointer.address()

private fun stringView(value: MaplibreNativeC.mln_buffer_view): String {
  val size = Math.toIntExact(value.size())
  if (size == 0) return ""
  val bytes = ByteArray(size)
  BytePointer(value.data()).get(bytes, 0, size)
  return String(bytes, StandardCharsets.UTF_8)
}

private fun pointerOrNull(pointer: NativePointer): Pointer? =
  if (pointer.isNull) null else AddressPointer(pointer.address)

private fun latLng(value: MaplibreNativeC.mln_lat_lng): LatLng =
  LatLng(value.latitude(), value.longitude())

private class RenderedQueryGeometryScope(value: RenderedQueryGeometry) : AutoCloseable {
  private val owned = mutableListOf<Pointer>()
  val geometry: MaplibreNativeC.mln_rendered_query_geometry =
    when (value) {
      is RenderedQueryGeometry.Point ->
        own(MaplibreNativeC.mln_rendered_query_geometry_point(screenPoint(value.point)))
      is RenderedQueryGeometry.Box ->
        own(MaplibreNativeC.mln_rendered_query_geometry_box(screenBox(value.box)))
      is RenderedQueryGeometry.LineString ->
        own(
          MaplibreNativeC.mln_rendered_query_geometry_line_string(
            screenPointArray(value.points),
            value.points.size.toLong(),
          )
        )
    }

  override fun close() {
    owned.asReversed().forEach(Pointer::close)
  }

  private fun <T : Pointer> own(pointer: T): T {
    owned += pointer
    return pointer
  }

  private fun screenPoint(value: ScreenPoint): MaplibreNativeC.mln_screen_point =
    own(MaplibreNativeC.mln_screen_point().x(value.x).y(value.y))

  private fun screenBox(value: ScreenBox): MaplibreNativeC.mln_screen_box =
    own(MaplibreNativeC.mln_screen_box().min(screenPoint(value.min)).max(screenPoint(value.max)))

  private fun screenPointArray(values: List<ScreenPoint>): MaplibreNativeC.mln_screen_point? {
    if (values.isEmpty()) {
      return null
    }
    val out = own(MaplibreNativeC.mln_screen_point(values.size.toLong()))
    values.forEachIndexed { index, value -> out.position(index.toLong()).x(value.x).y(value.y) }
    out.position(0)
    return out
  }
}

private class RenderedFeatureQueryOptionsScope(value: RenderedFeatureQueryOptions?) :
  AutoCloseable {
  private val strings = mutableListOf<StringViewScope>()
  private val filter = value?.filterTransit?.let(::ByteArrayViewScope)
  private val layerIds = value?.layerIds?.let { stringViewArray(it) }
  val options: MaplibreNativeC.mln_rendered_feature_query_options? = value?.let {
    MaplibreNativeC.mln_rendered_feature_query_options_default().apply {
      var fields = 0
      it.layerIds?.let { layerIdValues ->
        fields = fields or MaplibreNativeC.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
        if (layerIds != null) {
          layer_ids(layerIds)
        }
        layer_id_count(layerIdValues.size.toLong())
      }
      filter?.let { nativeFilter -> filter(nativeFilter.view) }
      fields(fields)
    }
  }

  override fun close() {
    options?.close()
    layerIds?.close()
    filter?.close()
    strings.asReversed().forEach(StringViewScope::close)
  }

  private fun stringViewArray(values: List<String>): MaplibreNativeC.mln_buffer_view? {
    if (values.isEmpty()) {
      return null
    }
    val out = MaplibreNativeC.mln_buffer_view(values.size.toLong())
    values.forEachIndexed { index, value ->
      val scope = StringViewScope(value)
      strings += scope
      out.position(index.toLong()).put<MaplibreNativeC.mln_buffer_view>(scope.view)
    }
    out.position(0)
    return out
  }
}

private class SourceFeatureQueryOptionsScope(value: SourceFeatureQueryOptions?) : AutoCloseable {
  private val strings = mutableListOf<StringViewScope>()
  private val filter = value?.filterTransit?.let(::ByteArrayViewScope)
  private val sourceLayerIds = value?.sourceLayerIds?.let { stringViewArray(it) }
  val options: MaplibreNativeC.mln_source_feature_query_options? = value?.let {
    MaplibreNativeC.mln_source_feature_query_options_default().apply {
      var fields = 0
      it.sourceLayerIds?.let { sourceLayerIdValues ->
        fields = fields or MaplibreNativeC.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
        if (sourceLayerIds != null) {
          source_layer_ids(sourceLayerIds)
        }
        source_layer_id_count(sourceLayerIdValues.size.toLong())
      }
      filter?.let { nativeFilter -> filter(nativeFilter.view) }
      fields(fields)
    }
  }

  override fun close() {
    options?.close()
    sourceLayerIds?.close()
    filter?.close()
    strings.asReversed().forEach(StringViewScope::close)
  }

  private fun stringViewArray(values: List<String>): MaplibreNativeC.mln_buffer_view? {
    if (values.isEmpty()) {
      return null
    }
    val out = MaplibreNativeC.mln_buffer_view(values.size.toLong())
    values.forEachIndexed { index, value ->
      val scope = StringViewScope(value)
      strings += scope
      out.position(index.toLong()).put<MaplibreNativeC.mln_buffer_view>(scope.view)
    }
    out.position(0)
    return out
  }
}

private class FeatureStateSelectorScope(value: FeatureStateSelector) : AutoCloseable {
  private val sourceId = StringViewScope(value.sourceId)
  private val sourceLayerId = value.sourceLayerId?.let(::StringViewScope)
  private val featureId = value.featureId?.let(::StringViewScope)
  private val stateKey = value.stateKey?.let(::StringViewScope)
  val selector: MaplibreNativeC.mln_feature_state_selector =
    MaplibreNativeC.mln_feature_state_selector()

  init {
    selector.size(selector.sizeof())
    selector.source_id(sourceId.view)
    var fields = 0
    sourceLayerId?.let {
      fields = fields or MaplibreNativeC.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
      selector.source_layer_id(it.view)
    }
    featureId?.let {
      fields = fields or MaplibreNativeC.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
      selector.feature_id(it.view)
    }
    stateKey?.let {
      fields = fields or MaplibreNativeC.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
      selector.state_key(it.view)
    }
    selector.fields(fields)
  }

  override fun close() {
    selector.close()
    stateKey?.close()
    featureId?.close()
    sourceLayerId?.close()
    sourceId.close()
  }
}

private class StringViewScope(value: String) : AutoCloseable {
  private val bytes: BytePointer
  val view: MaplibreNativeC.mln_buffer_view = MaplibreNativeC.mln_buffer_view()

  init {
    val utf8 = value.toByteArray(StandardCharsets.UTF_8)
    bytes = BytePointer(Math.max(utf8.size, 1).toLong())
    if (utf8.isNotEmpty()) bytes.put(utf8, 0, utf8.size)
    view.data(if (utf8.isEmpty()) null else bytes)
    view.size(utf8.size.toLong())
  }

  override fun close() {
    view.close()
    bytes.close()
  }
}

/** A `void*` built from a raw address, for backend-native pointers and user data. */
private class AddressPointer(address: Long) : Pointer(null as Pointer?) {
  init {
    this.address = address
  }
}

/** Direct test seam for the JavaCPP render, query, geometry, and feature adapter. */
internal object JavaCppRenderStructs {
  fun featureRoundTrip(value: Feature): Feature = FeatureScope(value).use { feature(it.feature) }

  fun geometryRoundTrip(value: Geometry): Geometry =
    featureRoundTrip(Feature(value, emptyList(), FeatureIdentifier.Null)).geometry

  fun renderedQueryGeometryType(value: RenderedQueryGeometry): Int =
    RenderedQueryGeometryScope(value).use { it.geometry.type() }

  fun textureImageInfoSnapshot(
    width: Int,
    height: Int,
    stride: Int,
    byteLength: Long,
  ): TextureImageInfo =
    MaplibreNativeC.mln_texture_image_info().use {
      it.width(width).height(height).stride(stride).byte_length(byteLength)
      textureImageInfo(it)
    }

  fun featureQueryCleanupAfterCopyFailure(): Int {
    var destroys = 0
    try {
      featureQueryResult(
        1L,
        counter = { _, outCount ->
          outCount.put(1L)
          MaplibreStatus.OK.nativeCode
        },
        getter = { _, _, outFeature ->
          outFeature.feature().property_count(Int.MAX_VALUE.toLong() + 1)
          MaplibreStatus.OK.nativeCode
        },
        destroyer = { destroys++ },
      )
    } catch (_: ArithmeticException) {
      return destroys
    }
    error("feature conversion unexpectedly succeeded")
  }

  fun metalSnapshot(value: MetalBorrowedTextureDescriptor): RenderDescriptorSnapshot =
    metalBorrowedTextureDescriptor(value).use {
      RenderDescriptorSnapshot(
        it.extent().width(),
        it.extent().height(),
        it.extent().scale_factor(),
        address(it.texture()),
        0L,
        0,
      )
    }

  fun vulkanSnapshot(value: VulkanBorrowedTextureDescriptor): RenderDescriptorSnapshot =
    vulkanBorrowedTextureDescriptor(value).use {
      RenderDescriptorSnapshot(
        it.extent().width(),
        it.extent().height(),
        it.extent().scale_factor(),
        address(it.image()),
        address(it.image_view()),
        it.final_layout(),
      )
    }

  fun openGlSnapshot(value: OpenGLBorrowedTextureDescriptor): RenderDescriptorSnapshot =
    openglBorrowedTextureDescriptor(value).use {
      RenderDescriptorSnapshot(
        it.extent().width(),
        it.extent().height(),
        it.extent().scale_factor(),
        it.texture().toLong(),
        address(it.context().data_egl().display()),
        it.target(),
      )
    }

  data class RenderDescriptorSnapshot(
    val width: Int,
    val height: Int,
    val scaleFactor: Double,
    val firstPointer: Long,
    val secondPointer: Long,
    val extra: Int,
  )
}
