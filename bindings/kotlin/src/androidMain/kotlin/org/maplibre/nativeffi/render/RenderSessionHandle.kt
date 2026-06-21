package org.maplibre.nativeffi.render

import java.nio.charset.StandardCharsets
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.bytedeco.javacpp.PointerPointer
import org.bytedeco.javacpp.SizeTPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Owned Android JNI render session handle. Close it on the map owner thread. */
public actual class RenderSessionHandle
private constructor(private val map: MapHandle, private val handleAddress: Long) : AutoCloseable {
  private val mapRetention = map.retainChild()
  private val core = HandleStateCore("RenderSessionHandle", handleAddress, map)
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
      MaplibreNativeC.mln_render_session_resize(
        renderSession(requireLiveAddress()),
        width,
        height,
        scaleFactor,
      )
    )
  }

  public actual fun renderUpdate() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("render")
    Status.check(
      MaplibreNativeC.mln_render_session_render_update(renderSession(requireLiveAddress()))
    )
  }

  public actual fun detach() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("detach")
    Status.check(MaplibreNativeC.mln_render_session_detach(renderSession(requireLiveAddress())))
  }

  public actual fun reduceMemoryUse() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("reduce memory use")
    Status.check(
      MaplibreNativeC.mln_render_session_reduce_memory_use(renderSession(requireLiveAddress()))
    )
  }

  public actual fun clearData() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("clear data")
    Status.check(MaplibreNativeC.mln_render_session_clear_data(renderSession(requireLiveAddress())))
  }

  public actual fun dumpDebugLogs() {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("dump debug logs")
    Status.check(
      MaplibreNativeC.mln_render_session_dump_debug_logs(renderSession(requireLiveAddress()))
    )
  }

  public actual fun setFeatureState(selector: FeatureStateSelector, value: JsonValue) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("set feature state")
    FeatureStateSelectorScope(selector).use { nativeSelector ->
      JsonScope(value).use { nativeValue ->
        Status.check(
          MaplibreNativeC.mln_render_session_set_feature_state(
            renderSession(requireLiveAddress()),
            nativeSelector.selector,
            nativeValue.value,
          )
        )
      }
    }
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): JsonValue {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("get feature state")
    FeatureStateSelectorScope(selector).use { nativeSelector ->
      PointerPointer<Pointer>(1).use { outState ->
        outState.put(0, null as Pointer?)
        Status.check(
          MaplibreNativeC.mln_render_session_get_feature_state(
            renderSession(requireLiveAddress()),
            nativeSelector.selector,
            outState,
          )
        )
        return jsonSnapshot(outState) ?: JsonValue.ObjectValue(emptyList())
      }
    }
  }

  public actual fun removeFeatureState(selector: FeatureStateSelector) {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("remove feature state")
    FeatureStateSelectorScope(selector).use { nativeSelector ->
      Status.check(
        MaplibreNativeC.mln_render_session_remove_feature_state(
          renderSession(requireLiveAddress()),
          nativeSelector.selector,
        )
      )
    }
  }

  public actual fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature> {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("query rendered features")
    RenderedQueryGeometryScope(geometry).use { nativeGeometry ->
      RenderedFeatureQueryOptionsScope(options).use { nativeOptions ->
        PointerPointer<Pointer>(1).use { outResult ->
          outResult.put(0, null as Pointer?)
          Status.check(
            MaplibreNativeC.mln_render_session_query_rendered_features(
              renderSession(requireLiveAddress()),
              nativeGeometry.geometry,
              nativeOptions.options,
              outResult,
            )
          )
          return featureQueryResult(outResult)
        }
      }
    }
  }

  public actual fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature> {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("query source features")
    StringViewScope(sourceId).use { nativeSourceId ->
      SourceFeatureQueryOptionsScope(options).use { nativeOptions ->
        PointerPointer<Pointer>(1).use { outResult ->
          outResult.put(0, null as Pointer?)
          Status.check(
            MaplibreNativeC.mln_render_session_query_source_features(
              renderSession(requireLiveAddress()),
              nativeSourceId.view,
              nativeOptions.options,
              outResult,
            )
          )
          return featureQueryResult(outResult)
        }
      }
    }
  }

  public actual fun queryFeatureExtension(
    sourceId: String,
    feature: Feature,
    extension: String,
    extensionField: String,
    arguments: JsonValue?,
  ): FeatureExtensionResult {
    NativeAccess.ensureLoaded()
    activeFrame.ensureInactive("query feature extension")
    StringViewScope(sourceId).use { nativeSourceId ->
      FeatureScope(feature).use { nativeFeature ->
        StringViewScope(extension).use { nativeExtension ->
          StringViewScope(extensionField).use { nativeExtensionField ->
            JsonScope(arguments ?: JsonValue.Null).use { nativeArguments ->
              PointerPointer<Pointer>(1).use { outResult ->
                outResult.put(0, null as Pointer?)
                Status.check(
                  MaplibreNativeC.mln_render_session_query_feature_extensions(
                    renderSession(requireLiveAddress()),
                    nativeSourceId.view,
                    nativeFeature.feature,
                    nativeExtension.view,
                    nativeExtensionField.view,
                    if (arguments == null) null else nativeArguments.value,
                    outResult,
                  )
                )
                return featureExtensionResult(outResult)
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
        renderSession(requireLiveAddress()),
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
        renderSession(requireLiveAddress()),
        buffer.borrowBuffer(),
        buffer.byteLength(),
        outInfo,
      )
    )
    return textureImageInfo(outInfo)
  }

  public actual fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle {
    NativeAccess.ensureLoaded()
    activeFrame.beginAcquire()
    val nativeFrame = MaplibreNativeC.mln_metal_owned_texture_frame()
    nativeFrame.size(nativeFrame.sizeof())
    try {
      Status.check(
        MaplibreNativeC.mln_metal_owned_texture_acquire_frame(
          renderSession(requireLiveAddress()),
          nativeFrame,
        )
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
        MaplibreNativeC.mln_vulkan_owned_texture_acquire_frame(
          renderSession(requireLiveAddress()),
          nativeFrame,
        )
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
        MaplibreNativeC.mln_opengl_owned_texture_acquire_frame(
          renderSession(requireLiveAddress()),
          nativeFrame,
        )
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
      destroy = { MaplibreNativeC.mln_render_session_destroy(renderSession(handleAddress)) },
      afterSuccess = { mapRetention.close() },
    )
  }

  private fun requireLiveAddress(): Long {
    core.requireLive()
    return handleAddress
  }

  internal fun releaseMetalFrame(frame: MaplibreNativeC.mln_metal_owned_texture_frame) {
    Status.check(
      MaplibreNativeC.mln_metal_owned_texture_release_frame(
        renderSession(requireLiveAddress()),
        frame,
      )
    )
  }

  internal fun releaseVulkanFrame(frame: MaplibreNativeC.mln_vulkan_owned_texture_frame) {
    Status.check(
      MaplibreNativeC.mln_vulkan_owned_texture_release_frame(
        renderSession(requireLiveAddress()),
        frame,
      )
    )
  }

  internal fun releaseOpenGLFrame(frame: MaplibreNativeC.mln_opengl_owned_texture_frame) {
    Status.check(
      MaplibreNativeC.mln_opengl_owned_texture_release_frame(
        renderSession(requireLiveAddress()),
        frame,
      )
    )
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
          map(map.nativeAddress()),
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
          map(map.nativeAddress()),
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
          map(map.nativeAddress()),
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
          map(map.nativeAddress()),
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
          map(map.nativeAddress()),
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
          map(map.nativeAddress()),
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
          map(map.nativeAddress()),
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
          map(map.nativeAddress()),
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
          map(map.nativeAddress()),
          openglSurfaceDescriptor(descriptor),
          outSession,
        )
      }

    private fun attach(
      map: MapHandle,
      block: (PointerPointer<MaplibreNativeC.mln_render_session>) -> Int,
    ): RenderSessionHandle {
      NativeAccess.ensureLoaded()
      PointerPointer<MaplibreNativeC.mln_render_session>(1).use { outSession ->
        outSession.put(0, null as Pointer?)
        Status.check(block(outSession))
        val session = outSession.get(MaplibreNativeC.mln_render_session::class.java, 0)
        val address = if (session == null || session.isNull) 0L else session.address()
        require(address != 0L) { "render session attach returned a null session" }
        return RenderSessionHandle(map, address)
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
  TextureImageInfo(info.width(), info.height(), info.stride(), info.byte_length())

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

private fun jsonSnapshot(outSnapshot: PointerPointer<Pointer>): JsonValue? {
  val snapshotPointer = outSnapshot.get(Pointer::class.java, 0) ?: return null
  if (snapshotPointer.isNull) return null
  val snapshot = MaplibreNativeC.mln_json_snapshot(snapshotPointer)
  return try {
    PointerPointer<Pointer>(1).use { outValue ->
      outValue.put(0, null as Pointer?)
      Status.check(MaplibreNativeC.mln_json_snapshot_get(snapshot, outValue))
      val valuePointer = outValue.get(Pointer::class.java, 0) ?: return null
      if (valuePointer.isNull) null else jsonValue(MaplibreNativeC.mln_json_value(valuePointer))
    }
  } finally {
    MaplibreNativeC.mln_json_snapshot_destroy(snapshot)
  }
}

private fun jsonValue(value: MaplibreNativeC.mln_json_value): JsonValue =
  when (value.type()) {
    MaplibreNativeC.MLN_JSON_VALUE_TYPE_NULL -> JsonValue.Null
    MaplibreNativeC.MLN_JSON_VALUE_TYPE_BOOL -> JsonValue.Bool(value.data_bool_value())
    MaplibreNativeC.MLN_JSON_VALUE_TYPE_UINT -> JsonValue.UInt(value.data_uint_value())
    MaplibreNativeC.MLN_JSON_VALUE_TYPE_INT -> JsonValue.Int(value.data_int_value())
    MaplibreNativeC.MLN_JSON_VALUE_TYPE_DOUBLE -> JsonValue.DoubleValue(value.data_double_value())
    MaplibreNativeC.MLN_JSON_VALUE_TYPE_STRING ->
      JsonValue.StringValue(stringView(value.data_string_value()))
    MaplibreNativeC.MLN_JSON_VALUE_TYPE_ARRAY -> jsonArray(value.data_array_value())
    MaplibreNativeC.MLN_JSON_VALUE_TYPE_OBJECT -> jsonObject(value.data_object_value())
    else -> JsonValue.Unknown(value.type(), value.size())
  }

private fun jsonArray(array: MaplibreNativeC.mln_json_array): JsonValue.Array {
  val nativeValues = array.values()
  return JsonValue.Array(
    List(Math.toIntExact(array.value_count())) { index ->
      jsonValue(nativeValues.getPointer(index.toLong()))
    }
  )
}

private fun jsonObject(obj: MaplibreNativeC.mln_json_object): JsonValue.ObjectValue {
  val nativeMembers = obj.members()
  return JsonValue.ObjectValue(
    List(Math.toIntExact(obj.member_count())) { index ->
      val member = nativeMembers.getPointer(index.toLong())
      JsonValue.Member(stringView(member.key()), jsonValue(member.value()))
    }
  )
}

private fun stringView(value: MaplibreNativeC.mln_string_view): String {
  val size = Math.toIntExact(value.size())
  if (size == 0) return ""
  val bytes = ByteArray(size)
  value.data().get(bytes, 0, size)
  return String(bytes, StandardCharsets.UTF_8)
}

private fun map(address: Long): MaplibreNativeC.mln_map =
  MaplibreNativeC.mln_map(AddressPointer(address))

private fun renderSession(address: Long): MaplibreNativeC.mln_render_session =
  MaplibreNativeC.mln_render_session(AddressPointer(address))

private fun pointerOrNull(pointer: NativePointer): Pointer? =
  if (pointer.isNull) null else AddressPointer(pointer.address)

private fun featureQueryResult(outResult: PointerPointer<Pointer>): List<QueriedFeature> {
  val pointer = outResult.get(Pointer::class.java, 0)
  val result = MaplibreNativeC.mln_feature_query_result(pointer)
  return try {
    SizeTPointer(1).use { outCount ->
      Status.check(MaplibreNativeC.mln_feature_query_result_count(result, outCount))
      val count = Math.toIntExact(outCount.get())
      List(count) { index ->
        val outFeature = MaplibreNativeC.mln_queried_feature()
        try {
          outFeature.size(outFeature.sizeof())
          Status.check(
            MaplibreNativeC.mln_feature_query_result_get(result, index.toLong(), outFeature)
          )
          queriedFeature(outFeature)
        } finally {
          outFeature.close()
        }
      }
    }
  } finally {
    MaplibreNativeC.mln_feature_query_result_destroy(result)
  }
}

private fun featureExtensionResult(outResult: PointerPointer<Pointer>): FeatureExtensionResult {
  val pointer = outResult.get(Pointer::class.java, 0)
  val result = MaplibreNativeC.mln_feature_extension_result(pointer)
  return try {
    val info = MaplibreNativeC.mln_feature_extension_result_info()
    try {
      info.size(info.sizeof())
      Status.check(MaplibreNativeC.mln_feature_extension_result_get(result, info))
      when (info.type()) {
        MaplibreNativeC.MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE ->
          FeatureExtensionResult.Value(jsonValue(info.data_value()))
        MaplibreNativeC.MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION ->
          FeatureExtensionResult.FeatureCollection(
            featureCollection(info.data_feature_collection())
          )
        else -> FeatureExtensionResult.Unknown(info.type())
      }
    } finally {
      info.close()
    }
  } finally {
    MaplibreNativeC.mln_feature_extension_result_destroy(result)
  }
}

private fun queriedFeature(value: MaplibreNativeC.mln_queried_feature): QueriedFeature {
  val fields = value.fields()
  val sourceId =
    if ((fields and MaplibreNativeC.MLN_QUERIED_FEATURE_SOURCE_ID) != 0) {
      stringView(value.source_id())
    } else {
      null
    }
  val sourceLayerId =
    if ((fields and MaplibreNativeC.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0) {
      stringView(value.source_layer_id())
    } else {
      null
    }
  val state =
    if ((fields and MaplibreNativeC.MLN_QUERIED_FEATURE_STATE) != 0 && !value.state().isNull) {
      jsonValue(value.state())
    } else {
      null
    }
  return QueriedFeature(feature(value.feature()), sourceId, sourceLayerId, state)
}

private fun featureCollection(value: MaplibreNativeC.mln_feature_collection): List<Feature> {
  val count = Math.toIntExact(value.feature_count())
  val nativeFeatures = value.features()
  return List(count) { index -> feature(nativeFeatures.getPointer(index.toLong())) }
}

private fun feature(value: MaplibreNativeC.mln_feature): Feature {
  val properties =
    List(Math.toIntExact(value.property_count())) { index ->
      val member = value.properties().getPointer(index.toLong())
      JsonValue.Member(stringView(member.key()), jsonValue(member.value()))
    }
  return Feature(geometry(value.geometry()), properties, featureIdentifier(value))
}

private fun featureIdentifier(value: MaplibreNativeC.mln_feature): FeatureIdentifier =
  when (value.identifier_type()) {
    MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_NULL -> FeatureIdentifier.Null
    MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_UINT ->
      FeatureIdentifier.UInt(value.identifier_uint_value())
    MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_INT ->
      FeatureIdentifier.Int(value.identifier_int_value())
    MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE ->
      FeatureIdentifier.DoubleValue(value.identifier_double_value())
    MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_STRING ->
      FeatureIdentifier.StringValue(stringView(value.identifier_string_value()))
    else -> FeatureIdentifier.Unknown(value.identifier_type())
  }

private fun geometry(value: MaplibreNativeC.mln_geometry): Geometry =
  when (value.type()) {
    MaplibreNativeC.MLN_GEOMETRY_TYPE_EMPTY -> Geometry.Empty
    MaplibreNativeC.MLN_GEOMETRY_TYPE_POINT -> Geometry.Point(latLng(value.data_point()))
    MaplibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING ->
      Geometry.LineString(coordinateSpan(value.data_line_string()))
    MaplibreNativeC.MLN_GEOMETRY_TYPE_POLYGON -> Geometry.Polygon(polygon(value.data_polygon()))
    MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT ->
      Geometry.MultiPoint(coordinateSpan(value.data_multi_point()))
    MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING -> {
      val native = value.data_multi_line_string()
      val lines = native.lines()
      Geometry.MultiLineString(
        List(Math.toIntExact(native.line_count())) { index ->
          coordinateSpan(lines.getPointer(index.toLong()))
        }
      )
    }
    MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON -> {
      val native = value.data_multi_polygon()
      val polygons = native.polygons()
      Geometry.MultiPolygon(
        List(Math.toIntExact(native.polygon_count())) { index ->
          polygon(polygons.getPointer(index.toLong()))
        }
      )
    }
    MaplibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION -> {
      val native = value.data_geometry_collection()
      val geometries = native.geometries()
      Geometry.Collection(
        List(Math.toIntExact(native.geometry_count())) { index ->
          geometry(geometries.getPointer(index.toLong()))
        }
      )
    }
    else -> Geometry.Unknown(value.type(), value.size())
  }

private fun coordinateSpan(value: MaplibreNativeC.mln_coordinate_span): List<LatLng> {
  val coordinates = value.coordinates()
  return List(Math.toIntExact(value.coordinate_count())) { index ->
    latLng(coordinates.getPointer(index.toLong()))
  }
}

private fun polygon(value: MaplibreNativeC.mln_polygon_geometry): List<List<LatLng>> {
  val rings = value.rings()
  return List(Math.toIntExact(value.ring_count())) { index ->
    coordinateSpan(rings.getPointer(index.toLong()))
  }
}

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
  private val filter = value?.filter?.let(::JsonScope)
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
      filter?.let { nativeFilter -> filter(nativeFilter.value) }
      fields(fields)
    }
  }

  override fun close() {
    options?.close()
    layerIds?.close()
    filter?.close()
    strings.asReversed().forEach(StringViewScope::close)
  }

  private fun stringViewArray(values: List<String>): MaplibreNativeC.mln_string_view? {
    if (values.isEmpty()) {
      return null
    }
    val out = MaplibreNativeC.mln_string_view(values.size.toLong())
    values.forEachIndexed { index, value ->
      val scope = StringViewScope(value)
      strings += scope
      out.position(index.toLong()).put<MaplibreNativeC.mln_string_view>(scope.view)
    }
    out.position(0)
    return out
  }
}

private class SourceFeatureQueryOptionsScope(value: SourceFeatureQueryOptions?) : AutoCloseable {
  private val strings = mutableListOf<StringViewScope>()
  private val filter = value?.filter?.let(::JsonScope)
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
      filter?.let { nativeFilter -> filter(nativeFilter.value) }
      fields(fields)
    }
  }

  override fun close() {
    options?.close()
    sourceLayerIds?.close()
    filter?.close()
    strings.asReversed().forEach(StringViewScope::close)
  }

  private fun stringViewArray(values: List<String>): MaplibreNativeC.mln_string_view? {
    if (values.isEmpty()) {
      return null
    }
    val out = MaplibreNativeC.mln_string_view(values.size.toLong())
    values.forEachIndexed { index, value ->
      val scope = StringViewScope(value)
      strings += scope
      out.position(index.toLong()).put<MaplibreNativeC.mln_string_view>(scope.view)
    }
    out.position(0)
    return out
  }
}

private class FeatureScope(feature: Feature) : AutoCloseable {
  private val scope = FeatureDescriptorScope()
  val feature: MaplibreNativeC.mln_feature = scope.feature(feature, 0)

  override fun close() {
    scope.close()
  }
}

private class FeatureDescriptorScope : AutoCloseable {
  private val owned = mutableListOf<Pointer>()
  private val strings = mutableListOf<StringViewScope>()
  private val jsonValues = mutableListOf<JsonScope>()

  override fun close() {
    jsonValues.asReversed().forEach(JsonScope::close)
    owned.asReversed().forEach(Pointer::close)
    strings.asReversed().forEach(StringViewScope::close)
  }

  fun feature(value: Feature, depth: Int): MaplibreNativeC.mln_feature {
    val out = own(MaplibreNativeC.mln_feature())
    out.size(out.sizeof())
    out.geometry(geometry(value.geometry, depth + 1))
    if (value.properties.isNotEmpty()) {
      val nativeMembers = own(MaplibreNativeC.mln_json_member(value.properties.size.toLong()))
      value.properties.forEachIndexed { index, member ->
        nativeMembers.position(index.toLong())
        nativeMembers.key(string(member.key))
        nativeMembers.value(json(member.value))
      }
      nativeMembers.position(0)
      out.properties(nativeMembers)
    }
    out.property_count(value.properties.size.toLong())
    featureIdentifier(out, value.identifier)
    return out
  }

  private fun geometry(value: Geometry, depth: Int): MaplibreNativeC.mln_geometry {
    Status.requireArgument(depth <= Geometry.MAX_COLLECTION_DEPTH) {
      "Geometry collection depth exceeds ${Geometry.MAX_COLLECTION_DEPTH}"
    }
    val out = own(MaplibreNativeC.mln_geometry())
    out.size(out.sizeof())
    when (value) {
      Geometry.Empty -> out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_EMPTY)
      is Geometry.Point ->
        out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_POINT).data_point(latLng(value.coordinate))
      is Geometry.LineString ->
        out
          .type(MaplibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING)
          .data_line_string(coordinateSpan(value.coordinates))
      is Geometry.Polygon ->
        out
          .type(MaplibreNativeC.MLN_GEOMETRY_TYPE_POLYGON)
          .data_polygon(polygonGeometry(value.rings))
      is Geometry.MultiPoint ->
        out
          .type(MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT)
          .data_multi_point(coordinateSpan(value.coordinates))
      is Geometry.MultiLineString ->
        out
          .type(MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING)
          .data_multi_line_string(multiLineGeometry(value.lines))
      is Geometry.MultiPolygon ->
        out
          .type(MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON)
          .data_multi_polygon(multiPolygonGeometry(value.polygons))
      is Geometry.Collection ->
        out
          .type(MaplibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION)
          .data_geometry_collection(geometryCollection(value.geometries, depth + 1))
      is Geometry.Unknown ->
        throw Status.invalidArgument("unknown geometries cannot be used as input")
    }
    return out
  }

  private fun coordinateSpan(values: List<LatLng>): MaplibreNativeC.mln_coordinate_span {
    val out = own(MaplibreNativeC.mln_coordinate_span())
    if (values.isNotEmpty()) {
      val coordinates = own(MaplibreNativeC.mln_lat_lng(values.size.toLong()))
      values.forEachIndexed { index, value ->
        coordinates.position(index.toLong()).latitude(value.latitude).longitude(value.longitude)
      }
      coordinates.position(0)
      out.coordinates(coordinates)
    }
    out.coordinate_count(values.size.toLong())
    return out
  }

  private fun coordinateSpans(values: List<List<LatLng>>): MaplibreNativeC.mln_coordinate_span? {
    if (values.isEmpty()) {
      return null
    }
    val out = own(MaplibreNativeC.mln_coordinate_span(values.size.toLong()))
    values.forEachIndexed { index, value ->
      out.position(index.toLong()).put<MaplibreNativeC.mln_coordinate_span>(coordinateSpan(value))
    }
    out.position(0)
    return out
  }

  private fun polygonGeometry(rings: List<List<LatLng>>): MaplibreNativeC.mln_polygon_geometry {
    val out = own(MaplibreNativeC.mln_polygon_geometry())
    coordinateSpans(rings)?.let(out::rings)
    out.ring_count(rings.size.toLong())
    return out
  }

  private fun multiLineGeometry(
    lines: List<List<LatLng>>
  ): MaplibreNativeC.mln_multi_line_geometry {
    val out = own(MaplibreNativeC.mln_multi_line_geometry())
    coordinateSpans(lines)?.let(out::lines)
    out.line_count(lines.size.toLong())
    return out
  }

  private fun multiPolygonGeometry(
    polygons: List<List<List<LatLng>>>
  ): MaplibreNativeC.mln_multi_polygon_geometry {
    val out = own(MaplibreNativeC.mln_multi_polygon_geometry())
    if (polygons.isNotEmpty()) {
      val nativePolygons = own(MaplibreNativeC.mln_polygon_geometry(polygons.size.toLong()))
      polygons.forEachIndexed { index, polygon ->
        nativePolygons
          .position(index.toLong())
          .put<MaplibreNativeC.mln_polygon_geometry>(polygonGeometry(polygon))
      }
      nativePolygons.position(0)
      out.polygons(nativePolygons)
    }
    out.polygon_count(polygons.size.toLong())
    return out
  }

  private fun geometryCollection(
    geometries: List<Geometry>,
    depth: Int,
  ): MaplibreNativeC.mln_geometry_collection {
    val out = own(MaplibreNativeC.mln_geometry_collection())
    if (geometries.isNotEmpty()) {
      val nativeGeometries = own(MaplibreNativeC.mln_geometry(geometries.size.toLong()))
      geometries.forEachIndexed { index, childGeometry ->
        nativeGeometries
          .position(index.toLong())
          .put<MaplibreNativeC.mln_geometry>(geometry(childGeometry, depth))
      }
      nativeGeometries.position(0)
      out.geometries(nativeGeometries)
    }
    out.geometry_count(geometries.size.toLong())
    return out
  }

  private fun featureIdentifier(out: MaplibreNativeC.mln_feature, value: FeatureIdentifier) {
    when (value) {
      FeatureIdentifier.Null ->
        out.identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_NULL)
      is FeatureIdentifier.UInt ->
        out
          .identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_UINT)
          .identifier_uint_value(value.value)
      is FeatureIdentifier.Int ->
        out
          .identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_INT)
          .identifier_int_value(value.value)
      is FeatureIdentifier.DoubleValue ->
        out
          .identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE)
          .identifier_double_value(value.value)
      is FeatureIdentifier.StringValue ->
        out
          .identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_STRING)
          .identifier_string_value(string(value.value))
      is FeatureIdentifier.Unknown ->
        throw Status.invalidArgument("unknown feature identifiers cannot be used as input")
    }
  }

  private fun <T : Pointer> own(pointer: T): T {
    owned += pointer
    return pointer
  }

  private fun string(value: String): MaplibreNativeC.mln_string_view {
    val scope = StringViewScope(value)
    strings += scope
    return scope.view
  }

  private fun json(value: JsonValue): MaplibreNativeC.mln_json_value {
    val scope = JsonScope(value)
    jsonValues += scope
    return scope.value
  }

  private fun latLng(value: LatLng): MaplibreNativeC.mln_lat_lng =
    own(MaplibreNativeC.mln_lat_lng().latitude(value.latitude).longitude(value.longitude))
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

private class JsonScope(value: JsonValue) : AutoCloseable {
  private val owned = mutableListOf<Pointer>()
  private val strings = mutableListOf<StringViewScope>()
  val value: MaplibreNativeC.mln_json_value = jsonValue(value)

  override fun close() {
    owned.asReversed().forEach(Pointer::close)
    strings.asReversed().forEach(StringViewScope::close)
  }

  private fun <T : Pointer> own(pointer: T): T {
    owned += pointer
    return pointer
  }

  private fun string(value: String): MaplibreNativeC.mln_string_view {
    val scope = StringViewScope(value)
    strings += scope
    return scope.view
  }

  private fun jsonValue(value: JsonValue, depth: Int = 0): MaplibreNativeC.mln_json_value {
    Status.requireArgument(depth <= JsonValue.MAX_DESCRIPTOR_DEPTH) {
      "JSON descriptor depth exceeds ${JsonValue.MAX_DESCRIPTOR_DEPTH}"
    }
    val out = own(MaplibreNativeC.mln_json_value())
    out.size(out.sizeof())
    when (value) {
      JsonValue.Null -> out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_NULL)
      is JsonValue.Bool ->
        out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_BOOL).data_bool_value(value.value)
      is JsonValue.UInt ->
        out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_UINT).data_uint_value(value.value)
      is JsonValue.Int ->
        out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_INT).data_int_value(value.value)
      is JsonValue.DoubleValue ->
        out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_DOUBLE).data_double_value(value.value)
      is JsonValue.StringValue ->
        out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_STRING).data_string_value(string(value.value))
      is JsonValue.Array -> {
        out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_ARRAY)
        val array = own(MaplibreNativeC.mln_json_array())
        if (value.values.isNotEmpty()) {
          val nativeValues = own(MaplibreNativeC.mln_json_value(value.values.size.toLong()))
          value.values.forEachIndexed { index, child ->
            nativeValues
              .position(index.toLong())
              .put<MaplibreNativeC.mln_json_value>(jsonValue(child, depth + 1))
          }
          nativeValues.position(0)
          array.values(nativeValues)
        }
        array.value_count(value.values.size.toLong())
        out.data_array_value(array)
      }
      is JsonValue.ObjectValue -> {
        out.type(MaplibreNativeC.MLN_JSON_VALUE_TYPE_OBJECT)
        val obj = own(MaplibreNativeC.mln_json_object())
        if (value.members.isNotEmpty()) {
          val nativeMembers = own(MaplibreNativeC.mln_json_member(value.members.size.toLong()))
          value.members.forEachIndexed { index, member ->
            nativeMembers.position(index.toLong())
            nativeMembers.key(string(member.key))
            nativeMembers.value(jsonValue(member.value, depth + 1))
          }
          nativeMembers.position(0)
          obj.members(nativeMembers)
        }
        obj.member_count(value.members.size.toLong())
        out.data_object_value(obj)
      }
      is JsonValue.Unknown ->
        throw Status.invalidArgument("unknown JSON values cannot be used as input")
    }
    return out
  }
}

private class StringViewScope(value: String) : AutoCloseable {
  private val bytes: BytePointer
  val view: MaplibreNativeC.mln_string_view = MaplibreNativeC.mln_string_view()

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

private class AddressPointer(address: Long) : Pointer(null as Pointer?) {
  init {
    this.address = address
  }
}
