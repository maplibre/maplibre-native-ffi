package org.maplibre.nativeffi.internal.loader

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import java.lang.invoke.MethodHandle
import java.nio.charset.StandardCharsets
import java.nio.file.Path
import java.util.NoSuchElementException
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.camera.UnitBezier
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.ProjectedMeters
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.geo.TileId
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.c.mln_animation_options
import org.maplibre.nativeffi.internal.c.mln_bound_options
import org.maplibre.nativeffi.internal.c.mln_camera_fit_options
import org.maplibre.nativeffi.internal.c.mln_camera_options
import org.maplibre.nativeffi.internal.c.mln_canonical_tile_id
import org.maplibre.nativeffi.internal.c.mln_coordinate_span
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_options
import org.maplibre.nativeffi.internal.c.mln_edge_insets
import org.maplibre.nativeffi.internal.c.mln_egl_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_feature
import org.maplibre.nativeffi.internal.c.mln_feature_collection
import org.maplibre.nativeffi.internal.c.mln_feature_extension_result_info
import org.maplibre.nativeffi.internal.c.mln_feature_state_selector
import org.maplibre.nativeffi.internal.c.mln_free_camera_options
import org.maplibre.nativeffi.internal.c.mln_geojson
import org.maplibre.nativeffi.internal.c.mln_geometry
import org.maplibre.nativeffi.internal.c.mln_geometry_collection
import org.maplibre.nativeffi.internal.c.mln_json_member
import org.maplibre.nativeffi.internal.c.mln_json_value
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_lat_lng_bounds
import org.maplibre.nativeffi.internal.c.mln_map_options
import org.maplibre.nativeffi.internal.c.mln_map_tile_options
import org.maplibre.nativeffi.internal.c.mln_map_viewport_options
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_metal_surface_descriptor
import org.maplibre.nativeffi.internal.c.mln_multi_line_geometry
import org.maplibre.nativeffi.internal.c.mln_multi_polygon_geometry
import org.maplibre.nativeffi.internal.c.mln_offline_geometry_region_definition
import org.maplibre.nativeffi.internal.c.mln_offline_region_definition
import org.maplibre.nativeffi.internal.c.mln_offline_region_info
import org.maplibre.nativeffi.internal.c.mln_offline_region_status
import org.maplibre.nativeffi.internal.c.mln_offline_tile_pyramid_region_definition
import org.maplibre.nativeffi.internal.c.mln_opengl_borrowed_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_opengl_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_opengl_surface_descriptor
import org.maplibre.nativeffi.internal.c.mln_polygon_geometry
import org.maplibre.nativeffi.internal.c.mln_premultiplied_rgba8_image
import org.maplibre.nativeffi.internal.c.mln_projected_meters
import org.maplibre.nativeffi.internal.c.mln_projection_mode
import org.maplibre.nativeffi.internal.c.mln_quaternion
import org.maplibre.nativeffi.internal.c.mln_queried_feature
import org.maplibre.nativeffi.internal.c.mln_render_target_extent
import org.maplibre.nativeffi.internal.c.mln_rendered_feature_query_options
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry
import org.maplibre.nativeffi.internal.c.mln_rendering_stats
import org.maplibre.nativeffi.internal.c.mln_resource_request
import org.maplibre.nativeffi.internal.c.mln_resource_response
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_operation_completed
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_response_error
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_status
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_tile_count_limit
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_frame
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_map
import org.maplibre.nativeffi.internal.c.mln_runtime_event_style_image_missing
import org.maplibre.nativeffi.internal.c.mln_runtime_event_tile_action
import org.maplibre.nativeffi.internal.c.mln_runtime_options
import org.maplibre.nativeffi.internal.c.mln_screen_box
import org.maplibre.nativeffi.internal.c.mln_screen_line_string
import org.maplibre.nativeffi.internal.c.mln_screen_point
import org.maplibre.nativeffi.internal.c.mln_source_feature_query_options
import org.maplibre.nativeffi.internal.c.mln_string_view
import org.maplibre.nativeffi.internal.c.mln_style_image_info
import org.maplibre.nativeffi.internal.c.mln_style_image_options
import org.maplibre.nativeffi.internal.c.mln_style_source_info
import org.maplibre.nativeffi.internal.c.mln_style_tile_source_options
import org.maplibre.nativeffi.internal.c.mln_texture_image_info
import org.maplibre.nativeffi.internal.c.mln_tile_id
import org.maplibre.nativeffi.internal.c.mln_unit_bezier
import org.maplibre.nativeffi.internal.c.mln_vec3
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_descriptor
import org.maplibre.nativeffi.internal.c.mln_wgl_context_descriptor
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.map.ConstrainMode
import org.maplibre.nativeffi.map.DebugOption
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.NorthOrientation
import org.maplibre.nativeffi.map.ProjectionModeOptions
import org.maplibre.nativeffi.map.RenderingStats
import org.maplibre.nativeffi.map.TileLodMode
import org.maplibre.nativeffi.map.TileOperation
import org.maplibre.nativeffi.map.TileOptions
import org.maplibre.nativeffi.map.ViewportMode
import org.maplibre.nativeffi.map.ViewportOptions
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.FrameScope
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureFrame
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.NativeBuffer
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLContextDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureFrame
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.TextureImageInfo
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureFrame
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.render.WglContextDescriptor
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceLoadingMethod
import org.maplibre.nativeffi.resource.ResourcePriority
import org.maplibre.nativeffi.resource.ResourceRequest
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceStoragePolicy
import org.maplibre.nativeffi.resource.ResourceUsage
import org.maplibre.nativeffi.runtime.OfflineOperationKind
import org.maplibre.nativeffi.runtime.OfflineOperationResultKind
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Ensures the native library is loaded before JVM FFM downcalls run. */
internal object NativeAccess {
  const val EXPECTED_C_ABI_VERSION: Long = 0L
  const val DEFAULT_LOG_SEVERITY_MASK: Int = (1 shl 1) or (1 shl 2)

  private val lock = Any()

  @Volatile private var initialized = false

  fun ensureLoaded() {
    if (initialized) {
      return
    }

    synchronized(lock) {
      if (initialized) {
        return
      }

      NativeLibrary.load()
      checkNativeAccessAndAbi()
      initialized = true
    }
  }

  fun load(libraryPath: Path) {
    synchronized(lock) {
      NativeLibrary.load(libraryPath)
      checkNativeAccessAndAbi()
      initialized = true
    }
  }

  internal fun checkAbiVersion(version: Long) {
    if (version != EXPECTED_C_ABI_VERSION) {
      throw AbiVersionMismatchException(version, EXPECTED_C_ABI_VERSION)
    }
  }

  internal fun checkNativeAccessAndAbi(cVersion: () -> Long) {
    val version =
      try {
        cVersion()
      } catch (error: ExceptionInInitializerError) {
        val cause = deepestCause(error)
        if (cause is IllegalCallerException) {
          throw nativeAccessFailure(cause)
        }
        if (cause is NoSuchElementException || cause is UnsatisfiedLinkError) {
          throw missingSymbols(error)
        }
        throw error
      } catch (error: IllegalCallerException) {
        throw nativeAccessFailure(error)
      } catch (error: NoSuchElementException) {
        throw missingSymbols(error)
      } catch (error: UnsatisfiedLinkError) {
        throw missingSymbols(error)
      }

    checkAbiVersion(version)
  }

  private fun checkNativeAccessAndAbi() {
    checkNativeAccessAndAbi(::cVersion)
  }

  internal fun cVersion(): Long =
    intFunction("mln_c_version").invokeWithArguments().let { Integer.toUnsignedLong(it as Int) }

  internal fun supportedRenderBackendMask(): Int =
    intFunction("mln_supported_render_backend_mask").invokeWithArguments() as Int

  internal fun supportedOpenGLContextProviderMask(): Int =
    intFunction("mln_opengl_supported_context_provider_mask").invokeWithArguments() as Int

  internal fun networkStatus(): Int =
    Arena.ofConfined().use { arena ->
      val outStatus = arena.allocate(ValueLayout.JAVA_INT)
      Status.check(
        statusOutFunction("mln_network_status_get").invokeWithArguments(outStatus) as Int
      )
      outStatus.get(ValueLayout.JAVA_INT, 0)
    }

  internal fun setNetworkStatus(status: Int) {
    Status.check(statusInFunction("mln_network_status_set").invokeWithArguments(status) as Int)
  }

  internal fun setAsyncLogSeverityMask(mask: Int) {
    Status.check(
      statusInFunction("mln_log_set_async_severity_mask").invokeWithArguments(mask) as Int
    )
  }

  internal fun setLogCallback(callback: MemorySegment): Int =
    logSetCallbackFunction().invokeWithArguments(callback, MemorySegment.NULL) as Int

  internal fun clearLogCallback(): Int =
    intFunction("mln_log_clear_callback").invokeWithArguments() as Int

  internal fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters =
    Arena.ofConfined().use { arena ->
      val nativeCoordinate = arena.allocate(latLngLayout)
      nativeCoordinate.set(ValueLayout.JAVA_DOUBLE, 0, coordinate.latitude)
      nativeCoordinate.set(
        ValueLayout.JAVA_DOUBLE,
        Double.SIZE_BYTES.toLong(),
        coordinate.longitude,
      )
      val outMeters = arena.allocate(projectedMetersLayout)
      Status.check(
        projectedMetersForLatLngFunction().invokeWithArguments(nativeCoordinate, outMeters) as Int
      )
      ProjectedMeters(
        outMeters.get(ValueLayout.JAVA_DOUBLE, 0),
        outMeters.get(ValueLayout.JAVA_DOUBLE, Double.SIZE_BYTES.toLong()),
      )
    }

  internal fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng =
    Arena.ofConfined().use { arena ->
      val nativeMeters = arena.allocate(projectedMetersLayout)
      nativeMeters.set(ValueLayout.JAVA_DOUBLE, 0, meters.northing)
      nativeMeters.set(ValueLayout.JAVA_DOUBLE, Double.SIZE_BYTES.toLong(), meters.easting)
      val outCoordinate = arena.allocate(latLngLayout)
      Status.check(
        latLngForProjectedMetersFunction().invokeWithArguments(nativeMeters, outCoordinate) as Int
      )
      LatLng(
        outCoordinate.get(ValueLayout.JAVA_DOUBLE, 0),
        outCoordinate.get(ValueLayout.JAVA_DOUBLE, Double.SIZE_BYTES.toLong()),
      )
    }

  internal fun createRuntime(options: RuntimeOptions): MemorySegment =
    Arena.ofConfined().use { arena ->
      val nativeOptions = runtimeOptions(options, arena)
      val outRuntime = arena.allocate(ValueLayout.ADDRESS)
      outRuntime.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(runtimeCreateFunction().invokeWithArguments(nativeOptions, outRuntime) as Int)
      outRuntime.get(ValueLayout.ADDRESS, 0).also { runtime ->
        require(runtime != MemorySegment.NULL) { "mln_runtime_create returned a null runtime" }
      }
    }

  internal fun runRuntimeOnce(runtime: MemorySegment) {
    Status.check(runtimeStatusFunction("mln_runtime_run_once").invokeWithArguments(runtime) as Int)
  }

  internal fun destroyRuntime(runtime: MemorySegment): Int =
    runtimeStatusFunction("mln_runtime_destroy").invokeWithArguments(runtime) as Int

  internal fun setResourceProvider(runtime: MemorySegment, provider: MemorySegment): Int =
    runtimeSetResourceProviderFunction().invokeWithArguments(runtime, provider) as Int

  internal fun startAmbientCacheOperation(runtime: MemorySegment, operation: Int): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeAmbientCacheOperationStartFunction()
          .invokeWithArguments(runtime, operation, outOperationId) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startCreateOfflineRegion(
    runtime: MemorySegment,
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeOfflineRegionCreateStartFunction()
          .invokeWithArguments(
            runtime,
            offlineRegionDefinition(definition, arena),
            nativeBytes(arena, metadata),
            metadata.size.toLong(),
            outOperationId,
          ) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startOfflineRegion(runtime: MemorySegment, regionId: Long): Long =
    startRuntimeLongOperation("mln_runtime_offline_region_get_start", runtime, regionId)

  internal fun startOfflineRegions(runtime: MemorySegment): Long =
    startRuntimeOperation("mln_runtime_offline_regions_list_start", runtime)

  internal fun startMergeOfflineRegionsDatabase(runtime: MemorySegment, path: String): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeAddressOperationStartFunction("mln_runtime_offline_regions_merge_database_start")
          .invokeWithArguments(runtime, cString(arena, path), outOperationId) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startUpdateOfflineRegionMetadata(
    runtime: MemorySegment,
    regionId: Long,
    metadata: ByteArray,
  ): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeLongAddressLongOperationStartFunction(
            "mln_runtime_offline_region_update_metadata_start"
          )
          .invokeWithArguments(
            runtime,
            regionId,
            nativeBytes(arena, metadata),
            metadata.size.toLong(),
            outOperationId,
          ) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startOfflineRegionStatus(runtime: MemorySegment, regionId: Long): Long =
    startRuntimeLongOperation("mln_runtime_offline_region_get_status_start", runtime, regionId)

  internal fun startSetOfflineRegionObserved(
    runtime: MemorySegment,
    regionId: Long,
    observed: Boolean,
  ): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeLongBooleanOperationStartFunction("mln_runtime_offline_region_set_observed_start")
          .invokeWithArguments(runtime, regionId, observed, outOperationId) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startSetOfflineRegionDownloadState(
    runtime: MemorySegment,
    regionId: Long,
    downloadState: Int,
  ): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeLongIntOperationStartFunction("mln_runtime_offline_region_set_download_state_start")
          .invokeWithArguments(runtime, regionId, downloadState, outOperationId) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startInvalidateOfflineRegion(runtime: MemorySegment, regionId: Long): Long =
    startRuntimeLongOperation("mln_runtime_offline_region_invalidate_start", runtime, regionId)

  internal fun startDeleteOfflineRegion(runtime: MemorySegment, regionId: Long): Long =
    startRuntimeLongOperation("mln_runtime_offline_region_delete_start", runtime, regionId)

  internal fun discardOfflineOperation(runtime: MemorySegment, operationId: Long): Int =
    runtimeOfflineOperationDiscardFunction().invokeWithArguments(runtime, operationId) as Int

  internal fun setResourceTransform(runtime: MemorySegment, descriptor: MemorySegment): Int =
    runtimeSetResourceTransformFunction().invokeWithArguments(runtime, descriptor) as Int

  internal fun clearResourceTransform(runtime: MemorySegment): Int =
    runtimeClearResourceTransformFunction().invokeWithArguments(runtime) as Int

  internal fun completeResourceRequest(handle: MemorySegment, response: ResourceResponse): Int =
    Arena.ofConfined().use { arena ->
      resourceRequestCompleteFunction()
        .invokeWithArguments(handle, resourceResponse(response, arena)) as Int
    }

  internal fun isResourceRequestCancelled(handle: MemorySegment): Boolean =
    Arena.ofConfined().use { arena ->
      val outCancelled = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        resourceRequestCancelledFunction().invokeWithArguments(handle, outCancelled) as Int
      )
      outCancelled.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun releaseResourceRequest(handle: MemorySegment) {
    resourceRequestReleaseFunction().invokeWithArguments(handle)
  }

  internal fun createMap(runtime: MemorySegment, options: MapOptions): MemorySegment =
    Arena.ofConfined().use { arena ->
      val outMap = arena.allocate(ValueLayout.ADDRESS)
      outMap.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        mapCreateFunction().invokeWithArguments(runtime, mapOptions(options, arena), outMap) as Int
      )
      outMap.get(ValueLayout.ADDRESS, 0).also { map ->
        require(map != MemorySegment.NULL) { "mln_map_create returned a null map" }
      }
    }

  internal fun destroyMap(map: MemorySegment): Int =
    mapStatusFunction("mln_map_destroy").invokeWithArguments(map) as Int

  internal fun setMapStyleUrl(map: MemorySegment, url: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_style_url")
          .invokeWithArguments(map, cString(arena, url)) as Int
      )
    }
  }

  internal fun setMapStyleJson(map: MemorySegment, json: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_style_json")
          .invokeWithArguments(map, cString(arena, json)) as Int
      )
    }
  }

  internal fun addStyleSourceJson(map: MemorySegment, sourceId: String, sourceJson: JsonValue) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_add_style_source_json")
          .invokeWithArguments(map, stringView(arena, sourceId), jsonValue(arena, sourceJson))
          as Int
      )
    }
  }

  internal fun removeStyleSource(map: MemorySegment, sourceId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outRemoved = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_remove_style_source")
          .invokeWithArguments(map, stringView(arena, sourceId), outRemoved) as Int
      )
      outRemoved.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleSourceExists(map: MemorySegment, sourceId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outExists = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_style_source_exists")
          .invokeWithArguments(map, stringView(arena, sourceId), outExists) as Int
      )
      outExists.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleSourceType(map: MemorySegment, sourceId: String): SourceType? =
    Arena.ofConfined().use { arena ->
      val outType = arena.allocate(ValueLayout.JAVA_INT)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_get_style_source_type")
          .invokeWithArguments(map, stringView(arena, sourceId), outType, outFound) as Int
      )
      if (outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        SourceType.fromNative(outType.get(ValueLayout.JAVA_INT, 0))
      } else null
    }

  internal fun styleSourceInfo(map: MemorySegment, sourceId: String): SourceInfo? =
    Arena.ofConfined().use { arena ->
      val sourceIdView = stringView(arena, sourceId)
      val outInfo = arena.allocate(STYLE_SOURCE_INFO_SIZE)
      outInfo.set(
        ValueLayout.JAVA_INT,
        STYLE_SOURCE_INFO_SIZE_OFFSET,
        STYLE_SOURCE_INFO_SIZE.toInt(),
      )
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_get_style_source_info")
          .invokeWithArguments(map, sourceIdView, outInfo, outFound) as Int
      )
      if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        return@use null
      }
      val attribution =
        if (outInfo.get(ValueLayout.JAVA_BOOLEAN, STYLE_SOURCE_INFO_HAS_ATTRIBUTION_OFFSET)) {
          copyStyleSourceAttribution(
            map,
            sourceIdView,
            outInfo.get(ValueLayout.JAVA_LONG, STYLE_SOURCE_INFO_ATTRIBUTION_SIZE_OFFSET),
            arena,
          ) ?: return@use null
        } else null
      SourceInfo(
        SourceType.fromNative(outInfo.get(ValueLayout.JAVA_INT, STYLE_SOURCE_INFO_TYPE_OFFSET)),
        outInfo.get(ValueLayout.JAVA_BOOLEAN, STYLE_SOURCE_INFO_IS_VOLATILE_OFFSET),
        attribution,
      )
    }

  internal fun styleSourceIds(map: MemorySegment): List<String> =
    Arena.ofConfined().use { arena ->
      val outList = arena.allocate(ValueLayout.ADDRESS)
      outList.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(mapListStyleSourceIdsFunction().invokeWithArguments(map, outList) as Int)
      styleIdList(outList.get(ValueLayout.ADDRESS, 0))
    }

  internal fun addGeoJsonSourceUrl(map: MemorySegment, sourceId: String, url: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_add_geojson_source_url")
          .invokeWithArguments(map, stringView(arena, sourceId), stringView(arena, url)) as Int
      )
    }
  }

  internal fun addGeoJsonSourceData(map: MemorySegment, sourceId: String, data: GeoJson) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_add_geojson_source_data")
          .invokeWithArguments(map, stringView(arena, sourceId), geoJson(arena, data)) as Int
      )
    }
  }

  internal fun setGeoJsonSourceUrl(map: MemorySegment, sourceId: String, url: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_set_geojson_source_url")
          .invokeWithArguments(map, stringView(arena, sourceId), stringView(arena, url)) as Int
      )
    }
  }

  internal fun setGeoJsonSourceData(map: MemorySegment, sourceId: String, data: GeoJson) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_set_geojson_source_data")
          .invokeWithArguments(map, stringView(arena, sourceId), geoJson(arena, data)) as Int
      )
    }
  }

  internal fun addCustomGeometrySource(
    map: MemorySegment,
    sourceId: String,
    options: MemorySegment,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_add_custom_geometry_source")
          .invokeWithArguments(map, stringView(arena, sourceId), options) as Int
      )
    }
  }

  internal fun setCustomGeometrySourceTileData(
    map: MemorySegment,
    sourceId: String,
    tileId: CanonicalTileId,
    data: GeoJson,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewCanonicalTileIdAddressStatusFunction(
            "mln_map_set_custom_geometry_source_tile_data"
          )
          .invokeWithArguments(
            map,
            stringView(arena, sourceId),
            canonicalTileId(arena, tileId),
            geoJson(arena, data),
          ) as Int
      )
    }
  }

  internal fun invalidateCustomGeometrySourceTile(
    map: MemorySegment,
    sourceId: String,
    tileId: CanonicalTileId,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewCanonicalTileIdStatusFunction("mln_map_invalidate_custom_geometry_source_tile")
          .invokeWithArguments(map, stringView(arena, sourceId), canonicalTileId(arena, tileId))
          as Int
      )
    }
  }

  internal fun invalidateCustomGeometrySourceRegion(
    map: MemorySegment,
    sourceId: String,
    bounds: LatLngBounds,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewLatLngBoundsStatusFunction("mln_map_invalidate_custom_geometry_source_region")
          .invokeWithArguments(map, stringView(arena, sourceId), latLngBounds(arena, bounds)) as Int
      )
    }
  }

  internal fun addVectorSourceUrl(
    map: MemorySegment,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    addTileSourceUrl("mln_map_add_vector_source_url", map, sourceId, url, options)
  }

  internal fun addVectorSourceTiles(
    map: MemorySegment,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    addTileSourceTiles("mln_map_add_vector_source_tiles", map, sourceId, tiles, options)
  }

  internal fun addRasterSourceUrl(
    map: MemorySegment,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    addTileSourceUrl("mln_map_add_raster_source_url", map, sourceId, url, options)
  }

  internal fun addRasterSourceTiles(
    map: MemorySegment,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    addTileSourceTiles("mln_map_add_raster_source_tiles", map, sourceId, tiles, options)
  }

  internal fun addRasterDemSourceUrl(
    map: MemorySegment,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    addTileSourceUrl("mln_map_add_raster_dem_source_url", map, sourceId, url, options)
  }

  internal fun addRasterDemSourceTiles(
    map: MemorySegment,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    addTileSourceTiles("mln_map_add_raster_dem_source_tiles", map, sourceId, tiles, options)
  }

  internal fun setStyleImage(
    map: MemorySegment,
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_set_style_image")
          .invokeWithArguments(
            map,
            stringView(arena, imageId),
            premultipliedRgba8Image(arena, image),
            styleImageOptions(arena, options),
          ) as Int
      )
    }
  }

  internal fun removeStyleImage(map: MemorySegment, imageId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outRemoved = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_remove_style_image")
          .invokeWithArguments(map, stringView(arena, imageId), outRemoved) as Int
      )
      outRemoved.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleImageExists(map: MemorySegment, imageId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outExists = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_style_image_exists")
          .invokeWithArguments(map, stringView(arena, imageId), outExists) as Int
      )
      outExists.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleImageInfo(map: MemorySegment, imageId: String): StyleImageInfo? =
    Arena.ofConfined().use { arena ->
      val outInfo = styleImageInfoDefault(arena)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_get_style_image_info")
          .invokeWithArguments(map, stringView(arena, imageId), outInfo, outFound) as Int
      )
      if (outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) styleImageInfo(outInfo) else null
    }

  internal fun copyStyleImagePremultipliedRgba8(map: MemorySegment, imageId: String): StyleImage? {
    val info = styleImageInfo(map, imageId) ?: return null
    return Arena.ofConfined().use { arena ->
      val outPixels = arena.allocate(info.byteLength)
      val outByteLength = arena.allocate(ValueLayout.JAVA_LONG)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressLongTwoAddressStatusFunction(
            "mln_map_copy_style_image_premultiplied_rgba8"
          )
          .invokeWithArguments(
            map,
            stringView(arena, imageId),
            outPixels,
            info.byteLength,
            outByteLength,
            outFound,
          ) as Int
      )
      if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        return@use null
      }
      val byteLength = outByteLength.get(ValueLayout.JAVA_LONG, 0)
      StyleImage(
        PremultipliedRgba8Image(
          info.width,
          info.height,
          info.stride,
          copyBytes(outPixels, byteLength),
        ),
        info.pixelRatio,
        info.sdf,
      )
    }
  }

  internal fun addImageSourceUrl(
    map: MemorySegment,
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ) {
    val coordinateSnapshot = coordinates.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressLongStringViewStatusFunction("mln_map_add_image_source_url")
          .invokeWithArguments(
            map,
            stringView(arena, sourceId),
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
            stringView(arena, url),
          ) as Int
      )
    }
  }

  internal fun addImageSourceImage(
    map: MemorySegment,
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ) {
    val coordinateSnapshot = coordinates.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressLongAddressStatusFunction("mln_map_add_image_source_image")
          .invokeWithArguments(
            map,
            stringView(arena, sourceId),
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
            premultipliedRgba8Image(arena, image),
          ) as Int
      )
    }
  }

  internal fun setImageSourceUrl(map: MemorySegment, sourceId: String, url: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_set_image_source_url")
          .invokeWithArguments(map, stringView(arena, sourceId), stringView(arena, url)) as Int
      )
    }
  }

  internal fun setImageSourceImage(
    map: MemorySegment,
    sourceId: String,
    image: PremultipliedRgba8Image,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_set_image_source_image")
          .invokeWithArguments(
            map,
            stringView(arena, sourceId),
            premultipliedRgba8Image(arena, image),
          ) as Int
      )
    }
  }

  internal fun setImageSourceCoordinates(
    map: MemorySegment,
    sourceId: String,
    coordinates: List<LatLng>,
  ) {
    val coordinateSnapshot = coordinates.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressLongStatusFunction("mln_map_set_image_source_coordinates")
          .invokeWithArguments(
            map,
            stringView(arena, sourceId),
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
          ) as Int
      )
    }
  }

  internal fun imageSourceCoordinates(map: MemorySegment, sourceId: String): List<LatLng>? =
    Arena.ofConfined().use { arena ->
      val outCoordinates = arena.allocate(latLngLayout.byteSize() * IMAGE_SOURCE_COORDINATE_COUNT)
      val outCoordinateCount = arena.allocate(ValueLayout.JAVA_LONG)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressLongTwoAddressStatusFunction("mln_map_get_image_source_coordinates")
          .invokeWithArguments(
            map,
            stringView(arena, sourceId),
            outCoordinates,
            IMAGE_SOURCE_COORDINATE_COUNT.toLong(),
            outCoordinateCount,
            outFound,
          ) as Int
      )
      if (outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        latLngArray(
          outCoordinates,
          Math.toIntExact(outCoordinateCount.get(ValueLayout.JAVA_LONG, 0)),
        )
      } else null
    }

  internal fun addStyleLayerJson(map: MemorySegment, layerJson: JsonValue, beforeLayerId: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStringViewStatusFunction("mln_map_add_style_layer_json")
          .invokeWithArguments(map, jsonValue(arena, layerJson), stringView(arena, beforeLayerId))
          as Int
      )
    }
  }

  internal fun removeStyleLayer(map: MemorySegment, layerId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outRemoved = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_remove_style_layer")
          .invokeWithArguments(map, stringView(arena, layerId), outRemoved) as Int
      )
      outRemoved.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleLayerExists(map: MemorySegment, layerId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outExists = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_style_layer_exists")
          .invokeWithArguments(map, stringView(arena, layerId), outExists) as Int
      )
      outExists.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleLayerType(map: MemorySegment, layerId: String): String? =
    Arena.ofConfined().use { arena ->
      val outType = arena.allocate(STRING_VIEW_SIZE)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_get_style_layer_type")
          .invokeWithArguments(map, stringView(arena, layerId), outType, outFound) as Int
      )
      if (outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) stringView(outType) else null
    }

  internal fun styleLayerIds(map: MemorySegment): List<String> =
    Arena.ofConfined().use { arena ->
      val outList = arena.allocate(ValueLayout.ADDRESS)
      outList.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(mapListStyleLayerIdsFunction().invokeWithArguments(map, outList) as Int)
      styleIdList(outList.get(ValueLayout.ADDRESS, 0))
    }

  internal fun addHillshadeLayer(
    map: MemorySegment,
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapThreeStringViewsStatusFunction("mln_map_add_hillshade_layer")
          .invokeWithArguments(
            map,
            stringView(arena, layerId),
            stringView(arena, sourceId),
            stringView(arena, beforeLayerId),
          ) as Int
      )
    }
  }

  internal fun addColorReliefLayer(
    map: MemorySegment,
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapThreeStringViewsStatusFunction("mln_map_add_color_relief_layer")
          .invokeWithArguments(
            map,
            stringView(arena, layerId),
            stringView(arena, sourceId),
            stringView(arena, beforeLayerId),
          ) as Int
      )
    }
  }

  internal fun addLocationIndicatorLayer(
    map: MemorySegment,
    layerId: String,
    beforeLayerId: String,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_add_location_indicator_layer")
          .invokeWithArguments(map, stringView(arena, layerId), stringView(arena, beforeLayerId))
          as Int
      )
    }
  }

  internal fun setLocationIndicatorLocation(
    map: MemorySegment,
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewLatLngDoubleStatusFunction("mln_map_set_location_indicator_location")
          .invokeWithArguments(map, stringView(arena, layerId), latLng(coordinate, arena), altitude)
          as Int
      )
    }
  }

  internal fun setLocationIndicatorBearing(map: MemorySegment, layerId: String, bearing: Double) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewDoubleStatusFunction("mln_map_set_location_indicator_bearing")
          .invokeWithArguments(map, stringView(arena, layerId), bearing) as Int
      )
    }
  }

  internal fun setLocationIndicatorAccuracyRadius(
    map: MemorySegment,
    layerId: String,
    radius: Double,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewDoubleStatusFunction("mln_map_set_location_indicator_accuracy_radius")
          .invokeWithArguments(map, stringView(arena, layerId), radius) as Int
      )
    }
  }

  internal fun setLocationIndicatorImageName(
    map: MemorySegment,
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewIntStringViewStatusFunction("mln_map_set_location_indicator_image_name")
          .invokeWithArguments(
            map,
            stringView(arena, layerId),
            imageKind.nativeValue,
            stringView(arena, imageId),
          ) as Int
      )
    }
  }

  internal fun moveStyleLayer(map: MemorySegment, layerId: String, beforeLayerId: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_move_style_layer")
          .invokeWithArguments(map, stringView(arena, layerId), stringView(arena, beforeLayerId))
          as Int
      )
    }
  }

  internal fun styleLayerJson(map: MemorySegment, layerId: String): JsonValue? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.ADDRESS)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      outSnapshot.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_get_style_layer_json")
          .invokeWithArguments(map, stringView(arena, layerId), outSnapshot, outFound) as Int
      )
      if (outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        jsonSnapshot(outSnapshot.get(ValueLayout.ADDRESS, 0))
      } else null
    }

  internal fun setStyleLightJson(map: MemorySegment, lightJson: JsonValue) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_style_light_json")
          .invokeWithArguments(map, jsonValue(arena, lightJson)) as Int
      )
    }
  }

  internal fun setStyleLightProperty(map: MemorySegment, propertyName: String, value: JsonValue) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_set_style_light_property")
          .invokeWithArguments(map, stringView(arena, propertyName), jsonValue(arena, value)) as Int
      )
    }
  }

  internal fun styleLightProperty(map: MemorySegment, propertyName: String): JsonValue? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.ADDRESS)
      outSnapshot.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_get_style_light_property")
          .invokeWithArguments(map, stringView(arena, propertyName), outSnapshot) as Int
      )
      jsonSnapshot(outSnapshot.get(ValueLayout.ADDRESS, 0))
    }

  internal fun setLayerProperty(
    map: MemorySegment,
    layerId: String,
    propertyName: String,
    value: JsonValue,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsAddressStatusFunction("mln_map_set_layer_property")
          .invokeWithArguments(
            map,
            stringView(arena, layerId),
            stringView(arena, propertyName),
            jsonValue(arena, value),
          ) as Int
      )
    }
  }

  internal fun layerProperty(
    map: MemorySegment,
    layerId: String,
    propertyName: String,
  ): JsonValue? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.ADDRESS)
      outSnapshot.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        mapTwoStringViewsAddressStatusFunction("mln_map_get_layer_property")
          .invokeWithArguments(
            map,
            stringView(arena, layerId),
            stringView(arena, propertyName),
            outSnapshot,
          ) as Int
      )
      jsonSnapshot(outSnapshot.get(ValueLayout.ADDRESS, 0))
    }

  internal fun setLayerFilter(map: MemorySegment, layerId: String, filter: JsonValue) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_set_layer_filter")
          .invokeWithArguments(map, stringView(arena, layerId), jsonValue(arena, filter)) as Int
      )
    }
  }

  internal fun clearLayerFilter(map: MemorySegment, layerId: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_set_layer_filter")
          .invokeWithArguments(map, stringView(arena, layerId), MemorySegment.NULL) as Int
      )
    }
  }

  internal fun layerFilter(map: MemorySegment, layerId: String): JsonValue? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.ADDRESS)
      outSnapshot.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_get_layer_filter")
          .invokeWithArguments(map, stringView(arena, layerId), outSnapshot) as Int
      )
      jsonSnapshot(outSnapshot.get(ValueLayout.ADDRESS, 0))
    }

  internal fun requestRepaint(map: MemorySegment) {
    Status.check(mapStatusFunction("mln_map_request_repaint").invokeWithArguments(map) as Int)
  }

  internal fun requestStillImage(map: MemorySegment) {
    Status.check(mapStatusFunction("mln_map_request_still_image").invokeWithArguments(map) as Int)
  }

  internal fun setDebugOptions(map: MemorySegment, options: Set<DebugOption>) {
    val mask = options.fold(0) { acc, option -> acc or option.nativeMask }
    Status.check(
      mapIntStatusFunction("mln_map_set_debug_options").invokeWithArguments(map, mask) as Int
    )
  }

  internal fun debugOptions(map: MemorySegment): Set<DebugOption> =
    Arena.ofConfined().use { arena ->
      val outOptions = arena.allocate(ValueLayout.JAVA_INT)
      Status.check(
        mapAddressStatusFunction("mln_map_get_debug_options").invokeWithArguments(map, outOptions)
          as Int
      )
      debugOptions(outOptions.get(ValueLayout.JAVA_INT, 0))
    }

  internal fun setRenderingStatsViewEnabled(map: MemorySegment, enabled: Boolean) {
    Status.check(
      mapBooleanStatusFunction("mln_map_set_rendering_stats_view_enabled")
        .invokeWithArguments(map, enabled) as Int
    )
  }

  internal fun renderingStatsViewEnabled(map: MemorySegment): Boolean =
    Arena.ofConfined().use { arena ->
      val outEnabled = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapAddressStatusFunction("mln_map_get_rendering_stats_view_enabled")
          .invokeWithArguments(map, outEnabled) as Int
      )
      outEnabled.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun isFullyLoaded(map: MemorySegment): Boolean =
    Arena.ofConfined().use { arena ->
      val outLoaded = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapAddressStatusFunction("mln_map_is_fully_loaded").invokeWithArguments(map, outLoaded)
          as Int
      )
      outLoaded.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun dumpDebugLogs(map: MemorySegment) {
    Status.check(mapStatusFunction("mln_map_dump_debug_logs").invokeWithArguments(map) as Int)
  }

  internal fun viewportOptions(map: MemorySegment): ViewportOptions =
    Arena.ofConfined().use { arena ->
      val outOptions = viewportOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_viewport_options")
          .invokeWithArguments(map, outOptions) as Int
      )
      readViewportOptions(outOptions)
    }

  internal fun setViewportOptions(map: MemorySegment, options: ViewportOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_viewport_options")
          .invokeWithArguments(map, viewportOptions(arena, options)) as Int
      )
    }
  }

  internal fun tileOptions(map: MemorySegment): TileOptions =
    Arena.ofConfined().use { arena ->
      val outOptions = tileOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_tile_options").invokeWithArguments(map, outOptions)
          as Int
      )
      readTileOptions(outOptions)
    }

  internal fun setTileOptions(map: MemorySegment, options: TileOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_tile_options")
          .invokeWithArguments(map, tileOptions(arena, options)) as Int
      )
    }
  }

  internal fun projectionMode(map: MemorySegment): ProjectionModeOptions =
    Arena.ofConfined().use { arena ->
      val outMode = projectionModeDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_projection_mode").invokeWithArguments(map, outMode)
          as Int
      )
      projectionModeOptions(outMode)
    }

  internal fun setProjectionMode(map: MemorySegment, mode: ProjectionModeOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_projection_mode")
          .invokeWithArguments(map, projectionModeOptions(arena, mode)) as Int
      )
    }
  }

  internal fun camera(map: MemorySegment): CameraOptions =
    Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_camera").invokeWithArguments(map, outCamera) as Int
      )
      cameraOptions(outCamera)
    }

  internal fun jumpTo(map: MemorySegment, camera: CameraOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_jump_to")
          .invokeWithArguments(map, cameraOptions(arena, camera)) as Int
      )
    }
  }

  internal fun easeTo(map: MemorySegment, camera: CameraOptions, animation: AnimationOptions?) {
    mapCameraAnimationCommand("mln_map_ease_to", map, camera, animation)
  }

  internal fun flyTo(map: MemorySegment, camera: CameraOptions, animation: AnimationOptions?) {
    mapCameraAnimationCommand("mln_map_fly_to", map, camera, animation)
  }

  internal fun moveBy(map: MemorySegment, deltaX: Double, deltaY: Double) {
    Status.check(
      mapDoubleDoubleStatusFunction("mln_map_move_by").invokeWithArguments(map, deltaX, deltaY)
        as Int
    )
  }

  internal fun moveByAnimated(
    map: MemorySegment,
    deltaX: Double,
    deltaY: Double,
    animation: AnimationOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapDoubleDoubleAddressStatusFunction("mln_map_move_by_animated")
          .invokeWithArguments(map, deltaX, deltaY, animationOptions(arena, animation)) as Int
      )
    }
  }

  internal fun scaleBy(map: MemorySegment, scale: Double, anchor: ScreenPoint?) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapDoubleAddressStatusFunction("mln_map_scale_by")
          .invokeWithArguments(
            map,
            scale,
            anchor?.let { screenPoint(it, arena) } ?: MemorySegment.NULL,
          ) as Int
      )
    }
  }

  internal fun scaleByAnimated(
    map: MemorySegment,
    scale: Double,
    anchor: ScreenPoint?,
    animation: AnimationOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapDoubleAddressAddressStatusFunction("mln_map_scale_by_animated")
          .invokeWithArguments(
            map,
            scale,
            anchor?.let { screenPoint(it, arena) } ?: MemorySegment.NULL,
            animationOptions(arena, animation),
          ) as Int
      )
    }
  }

  internal fun rotateBy(map: MemorySegment, first: ScreenPoint, second: ScreenPoint) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoScreenPointsStatusFunction("mln_map_rotate_by")
          .invokeWithArguments(map, screenPoint(first, arena), screenPoint(second, arena)) as Int
      )
    }
  }

  internal fun rotateByAnimated(
    map: MemorySegment,
    first: ScreenPoint,
    second: ScreenPoint,
    animation: AnimationOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoScreenPointsAddressStatusFunction("mln_map_rotate_by_animated")
          .invokeWithArguments(
            map,
            screenPoint(first, arena),
            screenPoint(second, arena),
            animationOptions(arena, animation),
          ) as Int
      )
    }
  }

  internal fun pitchBy(map: MemorySegment, pitch: Double) {
    Status.check(mapDoubleStatusFunction("mln_map_pitch_by").invokeWithArguments(map, pitch) as Int)
  }

  internal fun pitchByAnimated(map: MemorySegment, pitch: Double, animation: AnimationOptions?) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapDoubleAddressStatusFunction("mln_map_pitch_by_animated")
          .invokeWithArguments(map, pitch, animationOptions(arena, animation)) as Int
      )
    }
  }

  internal fun cancelTransitions(map: MemorySegment) {
    Status.check(mapStatusFunction("mln_map_cancel_transitions").invokeWithArguments(map) as Int)
  }

  internal fun cameraForLatLngBounds(
    map: MemorySegment,
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): CameraOptions =
    Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapLatLngBoundsAddressAddressStatusFunction("mln_map_camera_for_lat_lng_bounds")
          .invokeWithArguments(
            map,
            latLngBounds(arena, bounds),
            cameraFitOptions(arena, fitOptions),
            outCamera,
          ) as Int
      )
      cameraOptions(outCamera)
    }

  internal fun cameraForLatLngs(
    map: MemorySegment,
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): CameraOptions {
    val coordinateSnapshot = coordinates.toList()
    return Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapAddressLongAddressAddressStatusFunction("mln_map_camera_for_lat_lngs")
          .invokeWithArguments(
            map,
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
            cameraFitOptions(arena, fitOptions),
            outCamera,
          ) as Int
      )
      cameraOptions(outCamera)
    }
  }

  internal fun cameraForGeometry(
    map: MemorySegment,
    geometry: Geometry,
    fitOptions: CameraFitOptions?,
  ): CameraOptions =
    Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapAddressAddressAddressStatusFunction("mln_map_camera_for_geometry")
          .invokeWithArguments(
            map,
            geometry(arena, geometry, 0),
            cameraFitOptions(arena, fitOptions),
            outCamera,
          ) as Int
      )
      cameraOptions(outCamera)
    }

  internal fun latLngBoundsForCamera(map: MemorySegment, camera: CameraOptions): LatLngBounds =
    mapLatLngBoundsForCamera("mln_map_lat_lng_bounds_for_camera", map, camera)

  internal fun latLngBoundsForCameraUnwrapped(
    map: MemorySegment,
    camera: CameraOptions,
  ): LatLngBounds =
    mapLatLngBoundsForCamera("mln_map_lat_lng_bounds_for_camera_unwrapped", map, camera)

  internal fun bounds(map: MemorySegment): BoundOptions =
    Arena.ofConfined().use { arena ->
      val outBounds = boundOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_bounds").invokeWithArguments(map, outBounds) as Int
      )
      boundOptions(outBounds)
    }

  internal fun setBounds(map: MemorySegment, options: BoundOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_bounds")
          .invokeWithArguments(map, boundOptions(arena, options)) as Int
      )
    }
  }

  internal fun freeCameraOptions(map: MemorySegment): FreeCameraOptions =
    Arena.ofConfined().use { arena ->
      val outOptions = freeCameraOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_free_camera_options")
          .invokeWithArguments(map, outOptions) as Int
      )
      readFreeCameraOptions(outOptions)
    }

  internal fun setFreeCameraOptions(map: MemorySegment, options: FreeCameraOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_free_camera_options")
          .invokeWithArguments(map, freeCameraOptions(arena, options)) as Int
      )
    }
  }

  internal fun pixelForLatLng(map: MemorySegment, coordinate: LatLng): ScreenPoint =
    Arena.ofConfined().use { arena ->
      val outPoint = arena.allocate(screenPointLayout)
      Status.check(
        mapLatLngAddressStatusFunction("mln_map_pixel_for_lat_lng")
          .invokeWithArguments(map, latLng(coordinate, arena), outPoint) as Int
      )
      screenPoint(outPoint)
    }

  internal fun latLngForPixel(map: MemorySegment, point: ScreenPoint): LatLng =
    Arena.ofConfined().use { arena ->
      val outCoordinate = arena.allocate(latLngLayout)
      Status.check(
        mapScreenPointAddressStatusFunction("mln_map_lat_lng_for_pixel")
          .invokeWithArguments(map, screenPoint(point, arena), outCoordinate) as Int
      )
      latLng(outCoordinate)
    }

  internal fun pixelsForLatLngs(map: MemorySegment, coordinates: List<LatLng>): List<ScreenPoint> {
    val coordinateSnapshot = coordinates.toList()
    if (coordinateSnapshot.isEmpty()) {
      Arena.ofConfined().use { arena ->
        Status.check(
          mapAddressLongAddressStatusFunction("mln_map_pixels_for_lat_lngs")
            .invokeWithArguments(map, MemorySegment.NULL, 0L, MemorySegment.NULL) as Int
        )
      }
      return emptyList()
    }
    return Arena.ofConfined().use { arena ->
      val outPoints = arena.allocate(screenPointLayout.byteSize() * coordinateSnapshot.size)
      Status.check(
        mapAddressLongAddressStatusFunction("mln_map_pixels_for_lat_lngs")
          .invokeWithArguments(
            map,
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
            outPoints,
          ) as Int
      )
      screenPointArray(outPoints, coordinateSnapshot.size)
    }
  }

  internal fun latLngsForPixels(map: MemorySegment, points: List<ScreenPoint>): List<LatLng> {
    val pointSnapshot = points.toList()
    if (pointSnapshot.isEmpty()) {
      Arena.ofConfined().use { arena ->
        Status.check(
          mapAddressLongAddressStatusFunction("mln_map_lat_lngs_for_pixels")
            .invokeWithArguments(map, MemorySegment.NULL, 0L, MemorySegment.NULL) as Int
        )
      }
      return emptyList()
    }
    return Arena.ofConfined().use { arena ->
      val outCoordinates = arena.allocate(latLngLayout.byteSize() * pointSnapshot.size)
      Status.check(
        mapAddressLongAddressStatusFunction("mln_map_lat_lngs_for_pixels")
          .invokeWithArguments(
            map,
            screenPointArray(arena, pointSnapshot),
            pointSnapshot.size.toLong(),
            outCoordinates,
          ) as Int
      )
      latLngArray(outCoordinates, pointSnapshot.size)
    }
  }

  internal fun attachMetalOwnedTexture(
    map: MemorySegment,
    descriptor: MetalOwnedTextureDescriptor,
  ): MemorySegment =
    attachRenderSession(
      map,
      "mln_metal_owned_texture_attach",
      metalOwnedTextureDescriptor(descriptor),
    )

  internal fun attachMetalBorrowedTexture(
    map: MemorySegment,
    descriptor: MetalBorrowedTextureDescriptor,
  ): MemorySegment =
    attachRenderSession(
      map,
      "mln_metal_borrowed_texture_attach",
      metalBorrowedTextureDescriptor(descriptor),
    )

  internal fun attachVulkanOwnedTexture(
    map: MemorySegment,
    descriptor: VulkanOwnedTextureDescriptor,
  ): MemorySegment =
    attachRenderSession(
      map,
      "mln_vulkan_owned_texture_attach",
      vulkanOwnedTextureDescriptor(descriptor),
    )

  internal fun attachVulkanBorrowedTexture(
    map: MemorySegment,
    descriptor: VulkanBorrowedTextureDescriptor,
  ): MemorySegment =
    attachRenderSession(
      map,
      "mln_vulkan_borrowed_texture_attach",
      vulkanBorrowedTextureDescriptor(descriptor),
    )

  internal fun attachOpenGLOwnedTexture(
    map: MemorySegment,
    descriptor: OpenGLOwnedTextureDescriptor,
  ): MemorySegment =
    attachRenderSession(
      map,
      "mln_opengl_owned_texture_attach",
      openglOwnedTextureDescriptor(descriptor),
    )

  internal fun attachOpenGLBorrowedTexture(
    map: MemorySegment,
    descriptor: OpenGLBorrowedTextureDescriptor,
  ): MemorySegment =
    attachRenderSession(
      map,
      "mln_opengl_borrowed_texture_attach",
      openglBorrowedTextureDescriptor(descriptor),
    )

  internal fun attachMetalSurface(
    map: MemorySegment,
    descriptor: MetalSurfaceDescriptor,
  ): MemorySegment =
    attachRenderSession(map, "mln_metal_surface_attach", metalSurfaceDescriptor(descriptor))

  internal fun attachVulkanSurface(
    map: MemorySegment,
    descriptor: VulkanSurfaceDescriptor,
  ): MemorySegment =
    attachRenderSession(map, "mln_vulkan_surface_attach", vulkanSurfaceDescriptor(descriptor))

  internal fun attachOpenGLSurface(
    map: MemorySegment,
    descriptor: OpenGLSurfaceDescriptor,
  ): MemorySegment =
    attachRenderSession(map, "mln_opengl_surface_attach", openglSurfaceDescriptor(descriptor))

  internal fun destroyRenderSession(session: MemorySegment): Int =
    renderSessionStatusFunction("mln_render_session_destroy").invokeWithArguments(session) as Int

  internal fun resizeRenderSession(
    session: MemorySegment,
    width: Int,
    height: Int,
    scaleFactor: Double,
  ) {
    Status.check(
      renderSessionResizeFunction().invokeWithArguments(session, width, height, scaleFactor) as Int
    )
  }

  internal fun renderUpdate(session: MemorySegment) {
    Status.check(
      renderSessionStatusFunction("mln_render_session_render_update").invokeWithArguments(session)
        as Int
    )
  }

  internal fun detachRenderSession(session: MemorySegment) {
    Status.check(
      renderSessionStatusFunction("mln_render_session_detach").invokeWithArguments(session) as Int
    )
  }

  internal fun reduceRenderSessionMemoryUse(session: MemorySegment) {
    Status.check(
      renderSessionStatusFunction("mln_render_session_reduce_memory_use")
        .invokeWithArguments(session) as Int
    )
  }

  internal fun clearRenderSessionData(session: MemorySegment) {
    Status.check(
      renderSessionStatusFunction("mln_render_session_clear_data").invokeWithArguments(session)
        as Int
    )
  }

  internal fun dumpRenderSessionDebugLogs(session: MemorySegment) {
    Status.check(
      renderSessionStatusFunction("mln_render_session_dump_debug_logs").invokeWithArguments(session)
        as Int
    )
  }

  internal fun setFeatureState(
    session: MemorySegment,
    selector: FeatureStateSelector,
    value: JsonValue,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        renderSessionTwoAddressStatusFunction("mln_render_session_set_feature_state")
          .invokeWithArguments(
            session,
            featureStateSelector(arena, selector),
            jsonValue(arena, value),
          ) as Int
      )
    }
  }

  internal fun getFeatureState(session: MemorySegment, selector: FeatureStateSelector): JsonValue =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.ADDRESS)
      outSnapshot.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        renderSessionTwoAddressStatusFunction("mln_render_session_get_feature_state")
          .invokeWithArguments(session, featureStateSelector(arena, selector), outSnapshot) as Int
      )
      jsonSnapshot(outSnapshot.get(ValueLayout.ADDRESS, 0)) ?: JsonValue.ObjectValue(emptyList())
    }

  internal fun removeFeatureState(session: MemorySegment, selector: FeatureStateSelector) {
    Arena.ofConfined().use { arena ->
      Status.check(
        renderSessionAddressStatusFunction("mln_render_session_remove_feature_state")
          .invokeWithArguments(session, featureStateSelector(arena, selector)) as Int
      )
    }
  }

  internal fun queryRenderedFeatures(
    session: MemorySegment,
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature> =
    Arena.ofConfined().use { arena ->
      val outResult = arena.allocate(ValueLayout.ADDRESS)
      outResult.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        renderSessionQueryRenderedFeaturesFunction()
          .invokeWithArguments(
            session,
            renderedQueryGeometry(arena, geometry),
            renderedFeatureQueryOptions(arena, options),
            outResult,
          ) as Int
      )
      featureQueryResult(outResult.get(ValueLayout.ADDRESS, 0))
    }

  internal fun querySourceFeatures(
    session: MemorySegment,
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature> =
    Arena.ofConfined().use { arena ->
      val outResult = arena.allocate(ValueLayout.ADDRESS)
      outResult.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        renderSessionQuerySourceFeaturesFunction()
          .invokeWithArguments(
            session,
            stringView(arena, sourceId),
            sourceFeatureQueryOptions(arena, options),
            outResult,
          ) as Int
      )
      featureQueryResult(outResult.get(ValueLayout.ADDRESS, 0))
    }

  internal fun queryFeatureExtension(
    session: MemorySegment,
    sourceId: String,
    feature: Feature,
    extension: String,
    extensionField: String,
    arguments: JsonValue?,
  ): FeatureExtensionResult =
    Arena.ofConfined().use { arena ->
      val outResult = arena.allocate(ValueLayout.ADDRESS)
      outResult.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        renderSessionQueryFeatureExtensionsFunction()
          .invokeWithArguments(
            session,
            stringView(arena, sourceId),
            feature(arena, feature, 0),
            stringView(arena, extension),
            stringView(arena, extensionField),
            arguments?.let { jsonValue(arena, it) } ?: MemorySegment.NULL,
            outResult,
          ) as Int
      )
      featureExtensionResult(outResult.get(ValueLayout.ADDRESS, 0))
    }

  internal fun textureImageInfo(session: MemorySegment): TextureImageInfo =
    Arena.ofConfined().use { arena ->
      val outInfo = textureImageInfo(arena)
      val status =
        textureReadPremultipliedRgba8Function()
          .invokeWithArguments(session, MemorySegment.NULL, 0L, outInfo) as Int
      val info = readTextureImageInfo(outInfo)
      if (status == 0 || (status == -1 && info.byteLength > 0L)) {
        info
      } else {
        Status.check(status)
        error("unreachable")
      }
    }

  internal fun readPremultipliedRgba8(
    session: MemorySegment,
    buffer: NativeBuffer,
  ): TextureImageInfo =
    Arena.ofConfined().use { arena ->
      val outInfo = textureImageInfo(arena)
      buffer.borrow { segment, length ->
        Status.check(
          textureReadPremultipliedRgba8Function()
            .invokeWithArguments(session, segment, length, outInfo) as Int
        )
      }
      readTextureImageInfo(outInfo)
    }

  internal fun acquireMetalOwnedTextureFrame(session: MemorySegment): OwnedTextureFrameSegment {
    val arena = Arena.ofShared()
    val frame = mln_metal_owned_texture_frame.allocate(arena)
    mln_metal_owned_texture_frame.size(frame, mln_metal_owned_texture_frame.sizeof().toInt())
    try {
      Status.check(
        metalOwnedTextureAcquireFrameFunction().invokeWithArguments(session, frame) as Int
      )
      return OwnedTextureFrameSegment(frame, arena)
    } catch (error: Throwable) {
      arena.close()
      throw error
    }
  }

  internal fun acquireVulkanOwnedTextureFrame(session: MemorySegment): OwnedTextureFrameSegment {
    val arena = Arena.ofShared()
    val frame = mln_vulkan_owned_texture_frame.allocate(arena)
    mln_vulkan_owned_texture_frame.size(frame, mln_vulkan_owned_texture_frame.sizeof().toInt())
    try {
      Status.check(
        vulkanOwnedTextureAcquireFrameFunction().invokeWithArguments(session, frame) as Int
      )
      return OwnedTextureFrameSegment(frame, arena)
    } catch (error: Throwable) {
      arena.close()
      throw error
    }
  }

  internal fun acquireOpenGLOwnedTextureFrame(session: MemorySegment): OwnedTextureFrameSegment {
    val arena = Arena.ofShared()
    val frame = mln_opengl_owned_texture_frame.allocate(arena)
    mln_opengl_owned_texture_frame.size(frame, mln_opengl_owned_texture_frame.sizeof().toInt())
    try {
      Status.check(
        openglOwnedTextureAcquireFrameFunction().invokeWithArguments(session, frame) as Int
      )
      return OwnedTextureFrameSegment(frame, arena)
    } catch (error: Throwable) {
      arena.close()
      throw error
    }
  }

  internal fun releaseMetalOwnedTextureFrame(session: MemorySegment, frame: MemorySegment) {
    Status.check(metalOwnedTextureReleaseFrameFunction().invokeWithArguments(session, frame) as Int)
  }

  internal fun releaseVulkanOwnedTextureFrame(session: MemorySegment, frame: MemorySegment) {
    Status.check(
      vulkanOwnedTextureReleaseFrameFunction().invokeWithArguments(session, frame) as Int
    )
  }

  internal fun releaseOpenGLOwnedTextureFrame(session: MemorySegment, frame: MemorySegment) {
    Status.check(
      openglOwnedTextureReleaseFrameFunction().invokeWithArguments(session, frame) as Int
    )
  }

  internal fun metalOwnedTextureFrame(
    segment: MemorySegment,
    scope: FrameScope,
  ): MetalOwnedTextureFrame =
    MetalOwnedTextureFrame(
      scope,
      mln_metal_owned_texture_frame.generation(segment),
      mln_metal_owned_texture_frame.width(segment),
      mln_metal_owned_texture_frame.height(segment),
      mln_metal_owned_texture_frame.scale_factor(segment),
      mln_metal_owned_texture_frame.frame_id(segment),
      scopedPointer(mln_metal_owned_texture_frame.texture(segment), scope),
      scopedPointer(mln_metal_owned_texture_frame.device(segment), scope),
      mln_metal_owned_texture_frame.pixel_format(segment),
    )

  internal fun vulkanOwnedTextureFrame(
    segment: MemorySegment,
    scope: FrameScope,
  ): VulkanOwnedTextureFrame =
    VulkanOwnedTextureFrame(
      scope,
      mln_vulkan_owned_texture_frame.generation(segment),
      mln_vulkan_owned_texture_frame.width(segment),
      mln_vulkan_owned_texture_frame.height(segment),
      mln_vulkan_owned_texture_frame.scale_factor(segment),
      mln_vulkan_owned_texture_frame.frame_id(segment),
      scopedPointer(mln_vulkan_owned_texture_frame.image(segment), scope),
      scopedPointer(mln_vulkan_owned_texture_frame.image_view(segment), scope),
      scopedPointer(mln_vulkan_owned_texture_frame.device(segment), scope),
      mln_vulkan_owned_texture_frame.format(segment),
      mln_vulkan_owned_texture_frame.layout(segment),
    )

  internal fun openglOwnedTextureFrame(
    segment: MemorySegment,
    scope: FrameScope,
  ): OpenGLOwnedTextureFrame =
    OpenGLOwnedTextureFrame(
      scope,
      mln_opengl_owned_texture_frame.generation(segment),
      mln_opengl_owned_texture_frame.width(segment),
      mln_opengl_owned_texture_frame.height(segment),
      mln_opengl_owned_texture_frame.scale_factor(segment),
      mln_opengl_owned_texture_frame.frame_id(segment),
      mln_opengl_owned_texture_frame.texture(segment),
      mln_opengl_owned_texture_frame.target(segment),
      mln_opengl_owned_texture_frame.internal_format(segment),
      mln_opengl_owned_texture_frame.format(segment),
      mln_opengl_owned_texture_frame.type(segment),
    )

  internal fun createMapProjection(map: MemorySegment): MemorySegment =
    Arena.ofConfined().use { arena ->
      val outProjection = arena.allocate(ValueLayout.ADDRESS)
      outProjection.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        mapAddressStatusFunction("mln_map_projection_create")
          .invokeWithArguments(map, outProjection) as Int
      )
      outProjection.get(ValueLayout.ADDRESS, 0).also { projection ->
        require(projection != MemorySegment.NULL) {
          "mln_map_projection_create returned a null projection"
        }
      }
    }

  internal fun destroyMapProjection(projection: MemorySegment): Int =
    mapStatusFunction("mln_map_projection_destroy").invokeWithArguments(projection) as Int

  internal fun projectionCamera(projection: MemorySegment): CameraOptions =
    Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_projection_get_camera")
          .invokeWithArguments(projection, outCamera) as Int
      )
      cameraOptions(outCamera)
    }

  internal fun setProjectionCamera(projection: MemorySegment, camera: CameraOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_projection_set_camera")
          .invokeWithArguments(projection, cameraOptions(arena, camera)) as Int
      )
    }
  }

  internal fun setProjectionVisibleCoordinates(
    projection: MemorySegment,
    coordinates: List<LatLng>,
    padding: EdgeInsets,
  ) {
    val coordinateSnapshot = coordinates.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        projectionAddressLongEdgeInsetsStatusFunction("mln_map_projection_set_visible_coordinates")
          .invokeWithArguments(
            projection,
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
            edgeInsets(arena, padding),
          ) as Int
      )
    }
  }

  internal fun setProjectionVisibleGeometry(
    projection: MemorySegment,
    geometry: Geometry,
    padding: EdgeInsets,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        projectionAddressEdgeInsetsStatusFunction("mln_map_projection_set_visible_geometry")
          .invokeWithArguments(projection, geometry(arena, geometry, 0), edgeInsets(arena, padding))
          as Int
      )
    }
  }

  internal fun projectionPixelForLatLng(
    projection: MemorySegment,
    coordinate: LatLng,
  ): ScreenPoint =
    Arena.ofConfined().use { arena ->
      val outPoint = arena.allocate(screenPointLayout)
      Status.check(
        projectionLatLngAddressStatusFunction("mln_map_projection_pixel_for_lat_lng")
          .invokeWithArguments(projection, latLng(coordinate, arena), outPoint) as Int
      )
      screenPoint(outPoint)
    }

  internal fun projectionLatLngForPixel(projection: MemorySegment, point: ScreenPoint): LatLng =
    Arena.ofConfined().use { arena ->
      val outCoordinate = arena.allocate(latLngLayout)
      Status.check(
        projectionScreenPointAddressStatusFunction("mln_map_projection_lat_lng_for_pixel")
          .invokeWithArguments(projection, screenPoint(point, arena), outCoordinate) as Int
      )
      latLng(outCoordinate)
    }

  internal fun setResourceTransformResponseUrl(response: MemorySegment, value: String): Int =
    Arena.ofConfined().use { arena ->
      val bytes = value.toByteArray(StandardCharsets.UTF_8)
      resourceTransformResponseSetUrlFunction()
        .invokeWithArguments(response, nativeBytes(arena, bytes), bytes.size.toLong()) as Int
    }

  internal fun customGeometrySourceOptions(
    arena: Arena,
    value: CustomGeometrySourceOptions,
    fetchTile: MemorySegment,
    cancelTile: MemorySegment,
  ): MemorySegment {
    val segment = arena.allocate(CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZE_OFFSET,
      CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZE.toInt(),
    )
    segment.set(ValueLayout.ADDRESS, CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE_OFFSET, fetchTile)
    segment.set(ValueLayout.ADDRESS, CUSTOM_GEOMETRY_SOURCE_OPTIONS_CANCEL_TILE_OFFSET, cancelTile)
    segment.set(
      ValueLayout.ADDRESS,
      CUSTOM_GEOMETRY_SOURCE_OPTIONS_USER_DATA_OFFSET,
      MemorySegment.NULL,
    )
    var fields = 0
    value.minZoom?.let {
      fields = fields or CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
      segment.set(ValueLayout.JAVA_DOUBLE, CUSTOM_GEOMETRY_SOURCE_OPTIONS_MIN_ZOOM_OFFSET, it)
    }
    value.maxZoom?.let {
      fields = fields or CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
      segment.set(ValueLayout.JAVA_DOUBLE, CUSTOM_GEOMETRY_SOURCE_OPTIONS_MAX_ZOOM_OFFSET, it)
    }
    value.tolerance?.let {
      fields = fields or CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
      segment.set(ValueLayout.JAVA_DOUBLE, CUSTOM_GEOMETRY_SOURCE_OPTIONS_TOLERANCE_OFFSET, it)
    }
    value.tileSize?.let {
      fields = fields or CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
      segment.set(ValueLayout.JAVA_INT, CUSTOM_GEOMETRY_SOURCE_OPTIONS_TILE_SIZE_OFFSET, it)
    }
    value.buffer?.let {
      fields = fields or CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
      segment.set(ValueLayout.JAVA_INT, CUSTOM_GEOMETRY_SOURCE_OPTIONS_BUFFER_OFFSET, it)
    }
    value.clip?.let {
      fields = fields or CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
      segment.set(ValueLayout.JAVA_BOOLEAN, CUSTOM_GEOMETRY_SOURCE_OPTIONS_CLIP_OFFSET, it)
    }
    value.wrap?.let {
      fields = fields or CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
      segment.set(ValueLayout.JAVA_BOOLEAN, CUSTOM_GEOMETRY_SOURCE_OPTIONS_WRAP_OFFSET, it)
    }
    segment.set(ValueLayout.JAVA_INT, CUSTOM_GEOMETRY_SOURCE_OPTIONS_FIELDS_OFFSET, fields)
    return segment
  }

  internal fun canonicalTileId(segment: MemorySegment): CanonicalTileId =
    CanonicalTileId(
      segment.get(ValueLayout.JAVA_INT, CANONICAL_TILE_ID_Z_OFFSET),
      Integer.toUnsignedLong(segment.get(ValueLayout.JAVA_INT, CANONICAL_TILE_ID_X_OFFSET)),
      Integer.toUnsignedLong(segment.get(ValueLayout.JAVA_INT, CANONICAL_TILE_ID_Y_OFFSET)),
    )

  private fun canonicalTileId(arena: Arena, value: CanonicalTileId): MemorySegment {
    val segment = arena.allocate(canonicalTileIdLayout)
    segment.set(ValueLayout.JAVA_INT, CANONICAL_TILE_ID_Z_OFFSET, value.z)
    segment.set(ValueLayout.JAVA_INT, CANONICAL_TILE_ID_X_OFFSET, value.x.toInt())
    segment.set(ValueLayout.JAVA_INT, CANONICAL_TILE_ID_Y_OFFSET, value.y.toInt())
    return segment
  }

  private fun attachRenderSession(
    map: MemorySegment,
    functionName: String,
    descriptor: (Arena) -> MemorySegment,
  ): MemorySegment =
    Arena.ofConfined().use { arena ->
      val outSession = arena.allocate(ValueLayout.ADDRESS)
      outSession.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(
        mapAddressAddressStatusFunction(functionName)
          .invokeWithArguments(map, descriptor(arena), outSession) as Int
      )
      outSession.get(ValueLayout.ADDRESS, 0).also { session ->
        require(session != MemorySegment.NULL) { "$functionName returned a null render session" }
      }
    }

  private fun metalOwnedTextureDescriptor(
    descriptor: MetalOwnedTextureDescriptor
  ): (Arena) -> MemorySegment = { arena ->
    MapLibreNativeC.mln_metal_owned_texture_descriptor_default(arena).also { segment ->
      fillRenderTargetExtent(mln_metal_owned_texture_descriptor.extent(segment), descriptor.extent)
      fillMetalContext(mln_metal_owned_texture_descriptor.context(segment), descriptor.context)
    }
  }

  private fun metalBorrowedTextureDescriptor(
    descriptor: MetalBorrowedTextureDescriptor
  ): (Arena) -> MemorySegment = { arena ->
    MapLibreNativeC.mln_metal_borrowed_texture_descriptor_default(arena).also { segment ->
      fillRenderTargetExtent(
        mln_metal_borrowed_texture_descriptor.extent(segment),
        descriptor.extent,
      )
      mln_metal_borrowed_texture_descriptor.texture(segment, nativePointer(descriptor.texture))
    }
  }

  private fun vulkanOwnedTextureDescriptor(
    descriptor: VulkanOwnedTextureDescriptor
  ): (Arena) -> MemorySegment = { arena ->
    MapLibreNativeC.mln_vulkan_owned_texture_descriptor_default(arena).also { segment ->
      fillRenderTargetExtent(mln_vulkan_owned_texture_descriptor.extent(segment), descriptor.extent)
      fillVulkanContext(mln_vulkan_owned_texture_descriptor.context(segment), descriptor.context)
    }
  }

  private fun vulkanBorrowedTextureDescriptor(
    descriptor: VulkanBorrowedTextureDescriptor
  ): (Arena) -> MemorySegment = { arena ->
    MapLibreNativeC.mln_vulkan_borrowed_texture_descriptor_default(arena).also { segment ->
      fillRenderTargetExtent(
        mln_vulkan_borrowed_texture_descriptor.extent(segment),
        descriptor.extent,
      )
      fillVulkanContext(mln_vulkan_borrowed_texture_descriptor.context(segment), descriptor.context)
      mln_vulkan_borrowed_texture_descriptor.image(segment, nativePointer(descriptor.image))
      mln_vulkan_borrowed_texture_descriptor.image_view(
        segment,
        nativePointer(descriptor.imageView),
      )
      mln_vulkan_borrowed_texture_descriptor.format(segment, descriptor.format)
      mln_vulkan_borrowed_texture_descriptor.initial_layout(segment, descriptor.initialLayout)
      descriptor.finalLayout?.let {
        mln_vulkan_borrowed_texture_descriptor.final_layout(segment, it)
      }
    }
  }

  private fun openglOwnedTextureDescriptor(
    descriptor: OpenGLOwnedTextureDescriptor
  ): (Arena) -> MemorySegment = { arena ->
    MapLibreNativeC.mln_opengl_owned_texture_descriptor_default(arena).also { segment ->
      fillRenderTargetExtent(mln_opengl_owned_texture_descriptor.extent(segment), descriptor.extent)
      fillOpenGLContext(mln_opengl_owned_texture_descriptor.context(segment), descriptor.context)
    }
  }

  private fun openglBorrowedTextureDescriptor(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): (Arena) -> MemorySegment = { arena ->
    MapLibreNativeC.mln_opengl_borrowed_texture_descriptor_default(arena).also { segment ->
      fillRenderTargetExtent(
        mln_opengl_borrowed_texture_descriptor.extent(segment),
        descriptor.extent,
      )
      fillOpenGLContext(mln_opengl_borrowed_texture_descriptor.context(segment), descriptor.context)
      mln_opengl_borrowed_texture_descriptor.texture(segment, descriptor.texture)
      mln_opengl_borrowed_texture_descriptor.target(segment, descriptor.target)
    }
  }

  private fun metalSurfaceDescriptor(descriptor: MetalSurfaceDescriptor): (Arena) -> MemorySegment =
    { arena ->
      MapLibreNativeC.mln_metal_surface_descriptor_default(arena).also { segment ->
        fillRenderTargetExtent(mln_metal_surface_descriptor.extent(segment), descriptor.extent)
        fillMetalContext(mln_metal_surface_descriptor.context(segment), descriptor.context)
        mln_metal_surface_descriptor.layer(segment, nativePointer(descriptor.layer))
      }
    }

  private fun vulkanSurfaceDescriptor(
    descriptor: VulkanSurfaceDescriptor
  ): (Arena) -> MemorySegment = { arena ->
    MapLibreNativeC.mln_vulkan_surface_descriptor_default(arena).also { segment ->
      fillRenderTargetExtent(mln_vulkan_surface_descriptor.extent(segment), descriptor.extent)
      fillVulkanContext(mln_vulkan_surface_descriptor.context(segment), descriptor.context)
      mln_vulkan_surface_descriptor.surface(segment, nativePointer(descriptor.surface))
    }
  }

  private fun openglSurfaceDescriptor(
    descriptor: OpenGLSurfaceDescriptor
  ): (Arena) -> MemorySegment = { arena ->
    MapLibreNativeC.mln_opengl_surface_descriptor_default(arena).also { segment ->
      fillRenderTargetExtent(mln_opengl_surface_descriptor.extent(segment), descriptor.extent)
      fillOpenGLContext(mln_opengl_surface_descriptor.context(segment), descriptor.context)
      mln_opengl_surface_descriptor.surface(segment, nativePointer(descriptor.surface))
    }
  }

  private fun fillRenderTargetExtent(segment: MemorySegment, extent: RenderTargetExtent) {
    mln_render_target_extent.size(segment, mln_render_target_extent.sizeof().toInt())
    mln_render_target_extent.width(segment, extent.width)
    mln_render_target_extent.height(segment, extent.height)
    mln_render_target_extent.scale_factor(segment, extent.scaleFactor)
  }

  private fun fillMetalContext(segment: MemorySegment, context: MetalContextDescriptor) {
    mln_metal_context_descriptor.size(segment, mln_metal_context_descriptor.sizeof().toInt())
    mln_metal_context_descriptor.device(segment, nativePointer(context.device))
  }

  private fun fillVulkanContext(segment: MemorySegment, context: VulkanContextDescriptor) {
    mln_vulkan_context_descriptor.size(segment, mln_vulkan_context_descriptor.sizeof().toInt())
    mln_vulkan_context_descriptor.instance(segment, nativePointer(context.instance))
    mln_vulkan_context_descriptor.physical_device(segment, nativePointer(context.physicalDevice))
    mln_vulkan_context_descriptor.device(segment, nativePointer(context.device))
    mln_vulkan_context_descriptor.graphics_queue(segment, nativePointer(context.graphicsQueue))
    mln_vulkan_context_descriptor.graphics_queue_family_index(
      segment,
      context.graphicsQueueFamilyIndex,
    )
    mln_vulkan_context_descriptor.get_instance_proc_addr(
      segment,
      nativePointer(context.getInstanceProcAddr),
    )
    mln_vulkan_context_descriptor.get_device_proc_addr(
      segment,
      nativePointer(context.getDeviceProcAddr),
    )
  }

  private fun fillOpenGLContext(segment: MemorySegment, context: OpenGLContextDescriptor) {
    mln_opengl_context_descriptor.size(segment, mln_opengl_context_descriptor.sizeof().toInt())
    val data = mln_opengl_context_descriptor.data(segment)
    when (context) {
      is WglContextDescriptor -> {
        mln_opengl_context_descriptor.platform(
          segment,
          MapLibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_WGL(),
        )
        fillWglContext(mln_opengl_context_descriptor.data.wgl(data), context)
      }
      is EglContextDescriptor -> {
        mln_opengl_context_descriptor.platform(
          segment,
          MapLibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_EGL(),
        )
        fillEglContext(mln_opengl_context_descriptor.data.egl(data), context)
      }
    }
  }

  private fun fillWglContext(segment: MemorySegment, context: WglContextDescriptor) {
    mln_wgl_context_descriptor.size(segment, mln_wgl_context_descriptor.sizeof().toInt())
    mln_wgl_context_descriptor.device_context(segment, nativePointer(context.deviceContext))
    mln_wgl_context_descriptor.share_context(segment, nativePointer(context.shareContext))
    mln_wgl_context_descriptor.get_proc_address(segment, nativePointer(context.getProcAddress))
  }

  private fun fillEglContext(segment: MemorySegment, context: EglContextDescriptor) {
    mln_egl_context_descriptor.size(segment, mln_egl_context_descriptor.sizeof().toInt())
    mln_egl_context_descriptor.display(segment, nativePointer(context.display))
    mln_egl_context_descriptor.config(segment, nativePointer(context.config))
    mln_egl_context_descriptor.share_context(segment, nativePointer(context.shareContext))
    mln_egl_context_descriptor.get_proc_address(segment, nativePointer(context.getProcAddress))
  }

  private fun featureStateSelector(arena: Arena, selector: FeatureStateSelector): MemorySegment {
    val segment = arena.allocate(FEATURE_STATE_SELECTOR_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      FEATURE_STATE_SELECTOR_SIZE_OFFSET,
      FEATURE_STATE_SELECTOR_SIZE.toInt(),
    )
    segment.set(
      ValueLayout.ADDRESS,
      FEATURE_STATE_SELECTOR_SOURCE_ID_OFFSET,
      stringView(arena, selector.sourceId),
    )
    var fields = 0
    selector.sourceLayerId?.let {
      fields = fields or FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
      segment.set(
        ValueLayout.ADDRESS,
        FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID_OFFSET,
        stringView(arena, it),
      )
    }
    selector.featureId?.let {
      fields = fields or FEATURE_STATE_SELECTOR_FEATURE_ID
      segment.set(
        ValueLayout.ADDRESS,
        FEATURE_STATE_SELECTOR_FEATURE_ID_OFFSET,
        stringView(arena, it),
      )
    }
    selector.stateKey?.let {
      fields = fields or FEATURE_STATE_SELECTOR_STATE_KEY
      segment.set(
        ValueLayout.ADDRESS,
        FEATURE_STATE_SELECTOR_STATE_KEY_OFFSET,
        stringView(arena, it),
      )
    }
    segment.set(ValueLayout.JAVA_INT, FEATURE_STATE_SELECTOR_FIELDS_OFFSET, fields)
    return segment
  }

  private fun renderedQueryGeometry(arena: Arena, value: RenderedQueryGeometry): MemorySegment {
    val segment = arena.allocate(RENDERED_QUERY_GEOMETRY_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      RENDERED_QUERY_GEOMETRY_SIZE_OFFSET,
      RENDERED_QUERY_GEOMETRY_SIZE.toInt(),
    )
    when (value) {
      is RenderedQueryGeometry.Point -> {
        segment.set(ValueLayout.JAVA_INT, RENDERED_QUERY_GEOMETRY_TYPE_OFFSET, QUERY_GEOMETRY_POINT)
        segment
          .asSlice(RENDERED_QUERY_GEOMETRY_DATA_OFFSET, SCREEN_POINT_SIZE)
          .copyFrom(screenPoint(value.point, arena))
      }
      is RenderedQueryGeometry.Box -> {
        segment.set(ValueLayout.JAVA_INT, RENDERED_QUERY_GEOMETRY_TYPE_OFFSET, QUERY_GEOMETRY_BOX)
        segment
          .asSlice(RENDERED_QUERY_GEOMETRY_DATA_OFFSET, SCREEN_BOX_SIZE)
          .copyFrom(screenBox(arena, value.box))
      }
      is RenderedQueryGeometry.LineString -> {
        segment.set(
          ValueLayout.JAVA_INT,
          RENDERED_QUERY_GEOMETRY_TYPE_OFFSET,
          QUERY_GEOMETRY_LINE_STRING,
        )
        segment
          .asSlice(RENDERED_QUERY_GEOMETRY_DATA_OFFSET, SCREEN_LINE_STRING_SIZE)
          .copyFrom(screenLineString(arena, value.points))
      }
    }
    return segment
  }

  private fun renderedFeatureQueryOptions(
    arena: Arena,
    value: RenderedFeatureQueryOptions?,
  ): MemorySegment {
    if (value == null) {
      return MemorySegment.NULL
    }
    val segment = arena.allocate(RENDERED_FEATURE_QUERY_OPTIONS_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      RENDERED_FEATURE_QUERY_OPTIONS_SIZE_OFFSET,
      RENDERED_FEATURE_QUERY_OPTIONS_SIZE.toInt(),
    )
    var fields = 0
    value.layerIds?.let { layerIds ->
      val layerIdSnapshot = layerIds.toList()
      fields = fields or RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
      segment.set(
        ValueLayout.ADDRESS,
        RENDERED_FEATURE_QUERY_OPTIONS_LAYER_IDS_OFFSET,
        stringViewArray(arena, layerIdSnapshot),
      )
      segment.set(
        ValueLayout.JAVA_LONG,
        RENDERED_FEATURE_QUERY_OPTIONS_LAYER_ID_COUNT_OFFSET,
        layerIdSnapshot.size.toLong(),
      )
    }
    value.filter?.let { filter ->
      segment.set(
        ValueLayout.ADDRESS,
        RENDERED_FEATURE_QUERY_OPTIONS_FILTER_OFFSET,
        jsonValue(arena, filter),
      )
    }
    segment.set(ValueLayout.JAVA_INT, RENDERED_FEATURE_QUERY_OPTIONS_FIELDS_OFFSET, fields)
    return segment
  }

  private fun sourceFeatureQueryOptions(
    arena: Arena,
    value: SourceFeatureQueryOptions?,
  ): MemorySegment {
    if (value == null) {
      return MemorySegment.NULL
    }
    val segment = arena.allocate(SOURCE_FEATURE_QUERY_OPTIONS_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      SOURCE_FEATURE_QUERY_OPTIONS_SIZE_OFFSET,
      SOURCE_FEATURE_QUERY_OPTIONS_SIZE.toInt(),
    )
    var fields = 0
    value.sourceLayerIds?.let { sourceLayerIds ->
      val sourceLayerIdSnapshot = sourceLayerIds.toList()
      fields = fields or SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
      segment.set(
        ValueLayout.ADDRESS,
        SOURCE_FEATURE_QUERY_OPTIONS_SOURCE_LAYER_IDS_OFFSET,
        stringViewArray(arena, sourceLayerIdSnapshot),
      )
      segment.set(
        ValueLayout.JAVA_LONG,
        SOURCE_FEATURE_QUERY_OPTIONS_SOURCE_LAYER_ID_COUNT_OFFSET,
        sourceLayerIdSnapshot.size.toLong(),
      )
    }
    value.filter?.let { filter ->
      segment.set(
        ValueLayout.ADDRESS,
        SOURCE_FEATURE_QUERY_OPTIONS_FILTER_OFFSET,
        jsonValue(arena, filter),
      )
    }
    segment.set(ValueLayout.JAVA_INT, SOURCE_FEATURE_QUERY_OPTIONS_FIELDS_OFFSET, fields)
    return segment
  }

  private fun textureImageInfo(arena: Arena): MemorySegment =
    mln_texture_image_info.allocate(arena).also { segment ->
      mln_texture_image_info.size(segment, mln_texture_image_info.sizeof().toInt())
    }

  private fun readTextureImageInfo(segment: MemorySegment): TextureImageInfo =
    TextureImageInfo(
      mln_texture_image_info.width(segment),
      mln_texture_image_info.height(segment),
      mln_texture_image_info.stride(segment),
      mln_texture_image_info.byte_length(segment),
    )

  private fun nativePointer(pointer: NativePointer): MemorySegment =
    if (pointer.isNull) MemorySegment.NULL else MemorySegment.ofAddress(pointer.address)

  private fun scopedPointer(pointer: MemorySegment, scope: FrameScope): NativePointer =
    if (pointer == MemorySegment.NULL) NativePointer.NULL
    else NativePointer.scoped(pointer.address(), scope)

  internal fun takeOfflineRegionStatusResult(
    runtime: MemorySegment,
    operationId: Long,
    markTaken: () -> Unit,
  ): OfflineRegionStatus =
    Arena.ofConfined().use { arena ->
      val status = arena.allocate(OFFLINE_REGION_STATUS_SIZE)
      status.set(
        ValueLayout.JAVA_INT,
        OFFLINE_REGION_STATUS_SIZE_OFFSET,
        OFFLINE_REGION_STATUS_SIZE.toInt(),
      )
      Status.check(
        runtimeOfflineRegionStatusTakeResultFunction()
          .invokeWithArguments(runtime, operationId, status) as Int
      )
      try {
        offlineRegionStatus(status)
      } finally {
        markTaken()
      }
    }

  internal fun takeCreateOfflineRegionResult(
    runtime: MemorySegment,
    operationId: Long,
    markTaken: () -> Unit,
  ): OfflineRegionInfo =
    takeOfflineRegionSnapshot(
      runtime,
      operationId,
      runtimeOfflineRegionCreateTakeResultFunction(),
      markTaken,
    )

  internal fun takeOfflineRegionResult(
    runtime: MemorySegment,
    operationId: Long,
    markTaken: () -> Unit,
  ): OfflineRegionInfo? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.ADDRESS)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      outSnapshot.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      outFound.set(ValueLayout.JAVA_BOOLEAN, 0, false)
      Status.check(
        runtimeOfflineRegionGetTakeResultFunction()
          .invokeWithArguments(runtime, operationId, outSnapshot, outFound) as Int
      )
      if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        markTaken()
        return@use null
      }
      try {
        offlineRegionSnapshot(outSnapshot.get(ValueLayout.ADDRESS, 0))
      } finally {
        markTaken()
      }
    }

  internal fun takeOfflineRegionsResult(
    runtime: MemorySegment,
    operationId: Long,
    markTaken: () -> Unit,
  ): List<OfflineRegionInfo> =
    takeOfflineRegionList(
      runtime,
      operationId,
      runtimeOfflineRegionsListTakeResultFunction(),
      markTaken,
    )

  internal fun takeMergeOfflineRegionsDatabaseResult(
    runtime: MemorySegment,
    operationId: Long,
    markTaken: () -> Unit,
  ): List<OfflineRegionInfo> =
    takeOfflineRegionList(
      runtime,
      operationId,
      runtimeOfflineRegionsMergeDatabaseTakeResultFunction(),
      markTaken,
    )

  internal fun takeUpdateOfflineRegionMetadataResult(
    runtime: MemorySegment,
    operationId: Long,
    markTaken: () -> Unit,
  ): OfflineRegionInfo =
    takeOfflineRegionSnapshot(
      runtime,
      operationId,
      runtimeOfflineRegionUpdateMetadataTakeResultFunction(),
      markTaken,
    )

  internal fun pollRuntimeEvent(runtime: MemorySegment): NativeRuntimeEvent? =
    Arena.ofConfined().use { arena ->
      val event = arena.allocate(RUNTIME_EVENT_SIZE)
      event.set(ValueLayout.JAVA_INT, RUNTIME_EVENT_SIZE_OFFSET, RUNTIME_EVENT_SIZE.toInt())
      val hasEvent = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      hasEvent.set(ValueLayout.JAVA_BOOLEAN, 0, false)
      Status.check(runtimePollEventFunction().invokeWithArguments(runtime, event, hasEvent) as Int)
      if (!hasEvent.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        return@use null
      }
      val payloadSize = event.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_PAYLOAD_SIZE_OFFSET)
      val payload =
        boundedAddress(event.get(ValueLayout.ADDRESS, RUNTIME_EVENT_PAYLOAD_OFFSET), payloadSize)
      val payloadType = event.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_PAYLOAD_TYPE_OFFSET)
      val message = event.get(ValueLayout.ADDRESS, RUNTIME_EVENT_MESSAGE_OFFSET)
      val messageSize = event.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_MESSAGE_SIZE_OFFSET)
      NativeRuntimeEvent(
        type = event.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_TYPE_OFFSET),
        sourceType = event.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_SOURCE_TYPE_OFFSET),
        sourceAddress = event.get(ValueLayout.ADDRESS, RUNTIME_EVENT_SOURCE_OFFSET).address(),
        code = event.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_CODE_OFFSET),
        payload = runtimeEventPayload(payloadType, payload, payloadSize),
        message = copyString(message, messageSize),
      )
    }

  private fun intFunction(name: String): MethodHandle = downcall(name)

  private fun statusOutFunction(name: String): MethodHandle = downcall(name)

  private fun statusInFunction(name: String): MethodHandle = downcall(name)

  private fun logSetCallbackFunction(): MethodHandle = downcall("mln_log_set_callback")

  private fun projectedMetersForLatLngFunction(): MethodHandle =
    downcall("mln_projected_meters_for_lat_lng")

  private fun latLngForProjectedMetersFunction(): MethodHandle =
    downcall("mln_lat_lng_for_projected_meters")

  private fun runtimeCreateFunction(): MethodHandle = downcall("mln_runtime_create")

  private fun runtimeStatusFunction(name: String): MethodHandle = downcall(name)

  private fun runtimeAmbientCacheOperationStartFunction(): MethodHandle =
    downcall("mln_runtime_run_ambient_cache_operation_start")

  private fun runtimeOfflineRegionCreateStartFunction(): MethodHandle =
    downcall("mln_runtime_offline_region_create_start")

  private fun runtimeOfflineOperationDiscardFunction(): MethodHandle =
    downcall("mln_runtime_offline_operation_discard")

  private fun runtimeSetResourceProviderFunction(): MethodHandle =
    downcall("mln_runtime_set_resource_provider")

  private fun runtimeSetResourceTransformFunction(): MethodHandle =
    downcall("mln_runtime_set_resource_transform")

  private fun runtimeClearResourceTransformFunction(): MethodHandle =
    downcall("mln_runtime_clear_resource_transform")

  private fun resourceTransformResponseSetUrlFunction(): MethodHandle =
    downcall("mln_resource_transform_response_set_url")

  private fun resourceRequestCompleteFunction(): MethodHandle =
    downcall("mln_resource_request_complete")

  private fun resourceRequestCancelledFunction(): MethodHandle =
    downcall("mln_resource_request_cancelled")

  private fun resourceRequestReleaseFunction(): MethodHandle =
    downcall("mln_resource_request_release")

  private fun mapCreateFunction(): MethodHandle = downcall("mln_map_create")

  private fun mapStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapIntStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapBooleanStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapAddressAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapAddressAddressAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapStringViewAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapAddressStringViewStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapTwoStringViewsStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapStringViewCanonicalTileIdStatusFunction(name: String): MethodHandle =
    downcall(name)

  private fun mapStringViewCanonicalTileIdAddressStatusFunction(name: String): MethodHandle =
    downcall(name)

  private fun mapStringViewLatLngBoundsStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapThreeStringViewsStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapTwoStringViewsAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapStringViewLatLngDoubleStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapDoubleStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapDoubleDoubleStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapDoubleAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapDoubleDoubleAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapDoubleAddressAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapTwoAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapTwoScreenPointsStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapTwoScreenPointsAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapLatLngBoundsAddressAddressStatusFunction(name: String): MethodHandle =
    downcall(name)

  private fun mapAddressLongAddressAddressStatusFunction(name: String): MethodHandle =
    downcall(name)

  private fun projectionAddressLongEdgeInsetsStatusFunction(name: String): MethodHandle =
    downcall(name)

  private fun projectionAddressEdgeInsetsStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapLatLngAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapScreenPointAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapAddressLongAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun projectionLatLngAddressStatusFunction(name: String): MethodHandle =
    mapLatLngAddressStatusFunction(name)

  private fun projectionScreenPointAddressStatusFunction(name: String): MethodHandle =
    mapScreenPointAddressStatusFunction(name)

  private fun renderSessionStatusFunction(name: String): MethodHandle = downcall(name)

  private fun renderSessionResizeFunction(): MethodHandle = downcall("mln_render_session_resize")

  private fun renderSessionAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun renderSessionTwoAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun renderSessionQueryRenderedFeaturesFunction(): MethodHandle =
    downcall("mln_render_session_query_rendered_features")

  private fun renderSessionQuerySourceFeaturesFunction(): MethodHandle =
    downcall("mln_render_session_query_source_features")

  private fun renderSessionQueryFeatureExtensionsFunction(): MethodHandle =
    downcall("mln_render_session_query_feature_extensions")

  private fun textureReadPremultipliedRgba8Function(): MethodHandle =
    downcall("mln_texture_read_premultiplied_rgba8")

  private fun metalOwnedTextureAcquireFrameFunction(): MethodHandle =
    renderSessionAddressStatusFunction("mln_metal_owned_texture_acquire_frame")

  private fun vulkanOwnedTextureAcquireFrameFunction(): MethodHandle =
    renderSessionAddressStatusFunction("mln_vulkan_owned_texture_acquire_frame")

  private fun openglOwnedTextureAcquireFrameFunction(): MethodHandle =
    renderSessionAddressStatusFunction("mln_opengl_owned_texture_acquire_frame")

  private fun metalOwnedTextureReleaseFrameFunction(): MethodHandle =
    renderSessionAddressStatusFunction("mln_metal_owned_texture_release_frame")

  private fun vulkanOwnedTextureReleaseFrameFunction(): MethodHandle =
    renderSessionAddressStatusFunction("mln_vulkan_owned_texture_release_frame")

  private fun openglOwnedTextureReleaseFrameFunction(): MethodHandle =
    renderSessionAddressStatusFunction("mln_opengl_owned_texture_release_frame")

  private fun mapStringViewDoubleStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapStringViewIntStringViewStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapStringViewTwoAddressStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapStringViewAddressLongTwoAddressStatusFunction(name: String): MethodHandle =
    downcall(name)

  private fun mapStringViewAddressLongStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapStringViewAddressLongStringViewStatusFunction(name: String): MethodHandle =
    downcall(name)

  private fun mapStringViewAddressLongAddressStatusFunction(name: String): MethodHandle =
    downcall(name)

  private fun mapListStyleSourceIdsFunction(): MethodHandle =
    downcall("mln_map_list_style_source_ids")

  private fun mapListStyleLayerIdsFunction(): MethodHandle =
    downcall("mln_map_list_style_layer_ids")

  private fun styleIdListCountFunction(): MethodHandle = downcall("mln_style_id_list_count")

  private fun styleIdListGetFunction(): MethodHandle = downcall("mln_style_id_list_get")

  private fun styleIdListDestroyFunction(): MethodHandle = downcall("mln_style_id_list_destroy")

  private fun copyStyleSourceAttributionFunction(): MethodHandle =
    downcall("mln_map_copy_style_source_attribution")

  private fun jsonSnapshotGetFunction(): MethodHandle = downcall("mln_json_snapshot_get")

  private fun jsonSnapshotDestroyFunction(): MethodHandle = downcall("mln_json_snapshot_destroy")

  private fun featureQueryResultCountFunction(): MethodHandle =
    downcall("mln_feature_query_result_count")

  private fun featureQueryResultGetFunction(): MethodHandle =
    downcall("mln_feature_query_result_get")

  private fun featureQueryResultDestroyFunction(): MethodHandle =
    downcall("mln_feature_query_result_destroy")

  private fun featureExtensionResultGetFunction(): MethodHandle =
    downcall("mln_feature_extension_result_get")

  private fun featureExtensionResultDestroyFunction(): MethodHandle =
    downcall("mln_feature_extension_result_destroy")

  private fun runtimeOfflineRegionStatusTakeResultFunction(): MethodHandle =
    downcall("mln_runtime_offline_region_get_status_take_result")

  private fun runtimeOfflineRegionCreateTakeResultFunction(): MethodHandle =
    runtimeOfflineOperationSnapshotTakeResultFunction(
      "mln_runtime_offline_region_create_take_result"
    )

  private fun runtimeOfflineRegionGetTakeResultFunction(): MethodHandle =
    downcall("mln_runtime_offline_region_get_take_result")

  private fun runtimeOfflineRegionsListTakeResultFunction(): MethodHandle =
    runtimeOfflineOperationListTakeResultFunction("mln_runtime_offline_regions_list_take_result")

  private fun runtimeOfflineRegionsMergeDatabaseTakeResultFunction(): MethodHandle =
    runtimeOfflineOperationListTakeResultFunction(
      "mln_runtime_offline_regions_merge_database_take_result"
    )

  private fun runtimeOfflineRegionUpdateMetadataTakeResultFunction(): MethodHandle =
    runtimeOfflineOperationSnapshotTakeResultFunction(
      "mln_runtime_offline_region_update_metadata_take_result"
    )

  private fun runtimeOfflineOperationSnapshotTakeResultFunction(name: String): MethodHandle =
    downcall(name)

  private fun runtimeOfflineOperationListTakeResultFunction(name: String): MethodHandle =
    runtimeOfflineOperationSnapshotTakeResultFunction(name)

  private fun offlineRegionSnapshotGetFunction(): MethodHandle =
    downcall("mln_offline_region_snapshot_get")

  private fun offlineRegionSnapshotDestroyFunction(): MethodHandle =
    downcall("mln_offline_region_snapshot_destroy")

  private fun offlineRegionListCountFunction(): MethodHandle =
    downcall("mln_offline_region_list_count")

  private fun offlineRegionListGetFunction(): MethodHandle = downcall("mln_offline_region_list_get")

  private fun offlineRegionListDestroyFunction(): MethodHandle =
    downcall("mln_offline_region_list_destroy")

  private fun runtimePollEventFunction(): MethodHandle = downcall("mln_runtime_poll_event")

  private fun addTileSourceUrl(
    functionName: String,
    map: MemorySegment,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsAddressStatusFunction(functionName)
          .invokeWithArguments(
            map,
            stringView(arena, sourceId),
            stringView(arena, url),
            tileSourceOptions(arena, options),
          ) as Int
      )
    }
  }

  private fun addTileSourceTiles(
    functionName: String,
    map: MemorySegment,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    val tileSnapshot = tiles.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressLongAddressStatusFunction(functionName)
          .invokeWithArguments(
            map,
            stringView(arena, sourceId),
            stringViewArray(arena, tileSnapshot),
            tileSnapshot.size.toLong(),
            tileSourceOptions(arena, options),
          ) as Int
      )
    }
  }

  private fun startRuntimeOperation(name: String, runtime: MemorySegment): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeOperationStartFunction(name).invokeWithArguments(runtime, outOperationId) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  private fun startRuntimeLongOperation(name: String, runtime: MemorySegment, value: Long): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeLongOperationStartFunction(name).invokeWithArguments(runtime, value, outOperationId)
          as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  private fun runtimeOperationStartFunction(name: String): MethodHandle = downcall(name)

  private fun runtimeLongOperationStartFunction(name: String): MethodHandle = downcall(name)

  private fun runtimeAddressOperationStartFunction(name: String): MethodHandle = downcall(name)

  private fun runtimeLongAddressLongOperationStartFunction(name: String): MethodHandle =
    downcall(name)

  private fun runtimeLongBooleanOperationStartFunction(name: String): MethodHandle = downcall(name)

  private fun runtimeLongIntOperationStartFunction(name: String): MethodHandle = downcall(name)

  private fun runtimeOptions(options: RuntimeOptions, arena: Arena): MemorySegment {
    val nativeOptions = MapLibreNativeC.mln_runtime_options_default(arena)
    var flags = mln_runtime_options.flags(nativeOptions)
    mln_runtime_options.asset_path(nativeOptions, optionalCString(arena, options.assetPath))
    mln_runtime_options.cache_path(nativeOptions, optionalCString(arena, options.cachePath))
    options.maximumCacheSize?.let { maximumCacheSize ->
      flags = flags or MapLibreNativeC.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE()
      mln_runtime_options.maximum_cache_size(nativeOptions, maximumCacheSize)
    }
    mln_runtime_options.flags(nativeOptions, flags)
    return nativeOptions
  }

  private fun optionalCString(arena: Arena, value: String?): MemorySegment =
    value?.let { cString(arena, it) } ?: MemorySegment.NULL

  private fun cString(arena: Arena, value: String): MemorySegment {
    if ('\u0000' in value) {
      throw Status.invalidArgument("C string inputs cannot contain embedded NUL characters")
    }
    return arena.allocateFrom(value)
  }

  private fun stringView(arena: Arena, value: String): MemorySegment {
    val bytes = value.toByteArray(StandardCharsets.UTF_8)
    val segment = arena.allocate(STRING_VIEW_SIZE)
    segment.set(ValueLayout.ADDRESS, STRING_VIEW_DATA_OFFSET, nativeBytes(arena, bytes))
    segment.set(ValueLayout.JAVA_LONG, STRING_VIEW_SIZE_OFFSET, bytes.size.toLong())
    return segment
  }

  private fun stringViewArray(arena: Arena, values: List<String>): MemorySegment {
    if (values.isEmpty()) {
      return MemorySegment.NULL
    }
    val array = arena.allocate(STRING_VIEW_SIZE * values.size)
    values.forEachIndexed { index, value ->
      array.asSlice(index * STRING_VIEW_SIZE, STRING_VIEW_SIZE).copyFrom(stringView(arena, value))
    }
    return array
  }

  private fun tileSourceOptions(arena: Arena, value: TileSourceOptions?): MemorySegment {
    if (value == null) {
      return MemorySegment.NULL
    }
    val segment = arena.allocate(TILE_SOURCE_OPTIONS_SIZE)
    var fields = 0
    segment.set(
      ValueLayout.JAVA_INT,
      TILE_SOURCE_OPTIONS_SIZE_OFFSET,
      TILE_SOURCE_OPTIONS_SIZE.toInt(),
    )
    value.minZoom?.let {
      fields = fields or TILE_SOURCE_OPTION_MIN_ZOOM
      segment.set(ValueLayout.JAVA_DOUBLE, TILE_SOURCE_OPTIONS_MIN_ZOOM_OFFSET, it)
    }
    value.maxZoom?.let {
      fields = fields or TILE_SOURCE_OPTION_MAX_ZOOM
      segment.set(ValueLayout.JAVA_DOUBLE, TILE_SOURCE_OPTIONS_MAX_ZOOM_OFFSET, it)
    }
    value.attribution?.let {
      fields = fields or TILE_SOURCE_OPTION_ATTRIBUTION
      segment
        .asSlice(TILE_SOURCE_OPTIONS_ATTRIBUTION_OFFSET, STRING_VIEW_SIZE)
        .copyFrom(stringView(arena, it))
    }
    value.scheme?.let {
      fields = fields or TILE_SOURCE_OPTION_SCHEME
      segment.set(ValueLayout.JAVA_INT, TILE_SOURCE_OPTIONS_SCHEME_OFFSET, it.nativeValue)
    }
    value.bounds?.let {
      fields = fields or TILE_SOURCE_OPTION_BOUNDS
      latLngBounds(it, segment.asSlice(TILE_SOURCE_OPTIONS_BOUNDS_OFFSET))
    }
    value.tileSize?.let {
      fields = fields or TILE_SOURCE_OPTION_TILE_SIZE
      segment.set(ValueLayout.JAVA_INT, TILE_SOURCE_OPTIONS_TILE_SIZE_OFFSET, it)
    }
    value.vectorEncoding?.let {
      fields = fields or TILE_SOURCE_OPTION_VECTOR_ENCODING
      segment.set(ValueLayout.JAVA_INT, TILE_SOURCE_OPTIONS_VECTOR_ENCODING_OFFSET, it.nativeValue)
    }
    value.rasterDemEncoding?.let {
      fields = fields or TILE_SOURCE_OPTION_RASTER_ENCODING
      segment.set(ValueLayout.JAVA_INT, TILE_SOURCE_OPTIONS_RASTER_ENCODING_OFFSET, it.nativeValue)
    }
    segment.set(ValueLayout.JAVA_INT, TILE_SOURCE_OPTIONS_FIELDS_OFFSET, fields)
    return segment
  }

  private fun edgeInsets(segment: MemorySegment): EdgeInsets =
    EdgeInsets(
      segment.get(ValueLayout.JAVA_DOUBLE, EDGE_INSETS_TOP_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, EDGE_INSETS_LEFT_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, EDGE_INSETS_BOTTOM_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, EDGE_INSETS_RIGHT_OFFSET),
    )

  private fun writeEdgeInsets(segment: MemorySegment, value: EdgeInsets) {
    segment.set(ValueLayout.JAVA_DOUBLE, EDGE_INSETS_TOP_OFFSET, value.top)
    segment.set(ValueLayout.JAVA_DOUBLE, EDGE_INSETS_LEFT_OFFSET, value.left)
    segment.set(ValueLayout.JAVA_DOUBLE, EDGE_INSETS_BOTTOM_OFFSET, value.bottom)
    segment.set(ValueLayout.JAVA_DOUBLE, EDGE_INSETS_RIGHT_OFFSET, value.right)
  }

  private fun edgeInsets(arena: Arena, value: EdgeInsets): MemorySegment {
    val segment = arena.allocate(edgeInsetsLayout)
    writeEdgeInsets(segment, value)
    return segment
  }

  private fun viewportOptionsDefault(arena: Arena): MemorySegment =
    MapLibreNativeC.mln_map_viewport_options_default(arena)

  private fun viewportOptions(arena: Arena, value: ViewportOptions): MemorySegment {
    val segment = viewportOptionsDefault(arena)
    var fields = 0
    value.northOrientation?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown north orientation cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION()
      mln_map_viewport_options.north_orientation(segment, it.nativeValue)
    }
    value.constrainMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown constrain mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE()
      mln_map_viewport_options.constrain_mode(segment, it.nativeValue)
    }
    value.viewportMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown viewport mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE()
      mln_map_viewport_options.viewport_mode(segment, it.nativeValue)
    }
    value.frustumOffset?.let {
      fields = fields or MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET()
      writeEdgeInsets(mln_map_viewport_options.frustum_offset(segment), it)
    }
    mln_map_viewport_options.fields(segment, fields)
    return segment
  }

  private fun readViewportOptions(segment: MemorySegment): ViewportOptions {
    val fields = mln_map_viewport_options.fields(segment)
    return ViewportOptions().apply {
      if ((fields and MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION()) != 0) {
        northOrientation =
          NorthOrientation.fromNative(mln_map_viewport_options.north_orientation(segment))
      }
      if ((fields and MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE()) != 0) {
        constrainMode = ConstrainMode.fromNative(mln_map_viewport_options.constrain_mode(segment))
      }
      if ((fields and MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE()) != 0) {
        viewportMode = ViewportMode.fromNative(mln_map_viewport_options.viewport_mode(segment))
      }
      if ((fields and MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET()) != 0) {
        frustumOffset = edgeInsets(mln_map_viewport_options.frustum_offset(segment))
      }
    }
  }

  private fun tileOptionsDefault(arena: Arena): MemorySegment =
    MapLibreNativeC.mln_map_tile_options_default(arena)

  private fun tileOptions(arena: Arena, value: TileOptions): MemorySegment {
    val segment = tileOptionsDefault(arena)
    var fields = 0
    value.prefetchZoomDelta?.let {
      fields = fields or MapLibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA()
      mln_map_tile_options.prefetch_zoom_delta(segment, it)
    }
    value.lodMinRadius?.let {
      fields = fields or MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS()
      mln_map_tile_options.lod_min_radius(segment, it)
    }
    value.lodScale?.let {
      fields = fields or MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE()
      mln_map_tile_options.lod_scale(segment, it)
    }
    value.lodPitchThreshold?.let {
      fields = fields or MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD()
      mln_map_tile_options.lod_pitch_threshold(segment, it)
    }
    value.lodZoomShift?.let {
      fields = fields or MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT()
      mln_map_tile_options.lod_zoom_shift(segment, it)
    }
    value.lodMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown tile LOD mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE()
      mln_map_tile_options.lod_mode(segment, it.nativeValue)
    }
    mln_map_tile_options.fields(segment, fields)
    return segment
  }

  private fun readTileOptions(segment: MemorySegment): TileOptions {
    val fields = mln_map_tile_options.fields(segment)
    return TileOptions().apply {
      if ((fields and MapLibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA()) != 0) {
        prefetchZoomDelta = mln_map_tile_options.prefetch_zoom_delta(segment)
      }
      if ((fields and MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS()) != 0) {
        lodMinRadius = mln_map_tile_options.lod_min_radius(segment)
      }
      if ((fields and MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE()) != 0) {
        lodScale = mln_map_tile_options.lod_scale(segment)
      }
      if ((fields and MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD()) != 0) {
        lodPitchThreshold = mln_map_tile_options.lod_pitch_threshold(segment)
      }
      if ((fields and MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT()) != 0) {
        lodZoomShift = mln_map_tile_options.lod_zoom_shift(segment)
      }
      if ((fields and MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE()) != 0) {
        lodMode = TileLodMode.fromNative(mln_map_tile_options.lod_mode(segment))
      }
    }
  }

  private fun projectionModeDefault(arena: Arena): MemorySegment =
    MapLibreNativeC.mln_projection_mode_default(arena)

  private fun projectionModeOptions(arena: Arena, value: ProjectionModeOptions): MemorySegment {
    val segment = projectionModeDefault(arena)
    var fields = 0
    value.axonometric?.let {
      fields = fields or MapLibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC()
      mln_projection_mode.axonometric(segment, it)
    }
    value.xSkew?.let {
      fields = fields or MapLibreNativeC.MLN_PROJECTION_MODE_X_SKEW()
      mln_projection_mode.x_skew(segment, it)
    }
    value.ySkew?.let {
      fields = fields or MapLibreNativeC.MLN_PROJECTION_MODE_Y_SKEW()
      mln_projection_mode.y_skew(segment, it)
    }
    mln_projection_mode.fields(segment, fields)
    return segment
  }

  private fun projectionModeOptions(segment: MemorySegment): ProjectionModeOptions {
    val fields = mln_projection_mode.fields(segment)
    return ProjectionModeOptions().apply {
      if ((fields and MapLibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC()) != 0) {
        axonometric = mln_projection_mode.axonometric(segment)
      }
      if ((fields and MapLibreNativeC.MLN_PROJECTION_MODE_X_SKEW()) != 0) {
        xSkew = mln_projection_mode.x_skew(segment)
      }
      if ((fields and MapLibreNativeC.MLN_PROJECTION_MODE_Y_SKEW()) != 0) {
        ySkew = mln_projection_mode.y_skew(segment)
      }
    }
  }

  private fun cameraOptionsDefault(arena: Arena): MemorySegment =
    MapLibreNativeC.mln_camera_options_default(arena)

  private fun cameraOptions(arena: Arena, value: CameraOptions): MemorySegment {
    val segment = cameraOptionsDefault(arena)
    var fields = 0
    value.center?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_OPTION_CENTER()
      mln_camera_options.latitude(segment, it.latitude)
      mln_camera_options.longitude(segment, it.longitude)
    }
    value.centerAltitude?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE()
      mln_camera_options.center_altitude(segment, it)
    }
    value.padding?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_OPTION_PADDING()
      writeEdgeInsets(mln_camera_options.padding(segment), it)
    }
    value.anchor?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_OPTION_ANCHOR()
      mln_camera_options.anchor(segment).copyFrom(screenPoint(it, arena))
    }
    value.zoom?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_OPTION_ZOOM()
      mln_camera_options.zoom(segment, it)
    }
    value.bearing?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_OPTION_BEARING()
      mln_camera_options.bearing(segment, it)
    }
    value.pitch?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_OPTION_PITCH()
      mln_camera_options.pitch(segment, it)
    }
    value.roll?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_OPTION_ROLL()
      mln_camera_options.roll(segment, it)
    }
    value.fieldOfView?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_OPTION_FOV()
      mln_camera_options.field_of_view(segment, it)
    }
    mln_camera_options.fields(segment, fields)
    return segment
  }

  private fun cameraOptions(segment: MemorySegment): CameraOptions {
    val fields = mln_camera_options.fields(segment)
    return CameraOptions().apply {
      if ((fields and MapLibreNativeC.MLN_CAMERA_OPTION_CENTER()) != 0) {
        center = LatLng(mln_camera_options.latitude(segment), mln_camera_options.longitude(segment))
      }
      if ((fields and MapLibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE()) != 0) {
        centerAltitude = mln_camera_options.center_altitude(segment)
      }
      if ((fields and MapLibreNativeC.MLN_CAMERA_OPTION_PADDING()) != 0) {
        padding = edgeInsets(mln_camera_options.padding(segment))
      }
      if ((fields and MapLibreNativeC.MLN_CAMERA_OPTION_ANCHOR()) != 0) {
        anchor = screenPoint(mln_camera_options.anchor(segment))
      }
      if ((fields and MapLibreNativeC.MLN_CAMERA_OPTION_ZOOM()) != 0) {
        zoom = mln_camera_options.zoom(segment)
      }
      if ((fields and MapLibreNativeC.MLN_CAMERA_OPTION_BEARING()) != 0) {
        bearing = mln_camera_options.bearing(segment)
      }
      if ((fields and MapLibreNativeC.MLN_CAMERA_OPTION_PITCH()) != 0) {
        pitch = mln_camera_options.pitch(segment)
      }
      if ((fields and MapLibreNativeC.MLN_CAMERA_OPTION_ROLL()) != 0) {
        roll = mln_camera_options.roll(segment)
      }
      if ((fields and MapLibreNativeC.MLN_CAMERA_OPTION_FOV()) != 0) {
        fieldOfView = mln_camera_options.field_of_view(segment)
      }
    }
  }

  private fun animationOptions(arena: Arena, value: AnimationOptions?): MemorySegment {
    if (value == null) {
      return MemorySegment.NULL
    }
    val segment = MapLibreNativeC.mln_animation_options_default(arena)
    var fields = 0
    value.durationMs?.let {
      fields = fields or MapLibreNativeC.MLN_ANIMATION_OPTION_DURATION()
      mln_animation_options.duration_ms(segment, it)
    }
    value.velocity?.let {
      fields = fields or MapLibreNativeC.MLN_ANIMATION_OPTION_VELOCITY()
      mln_animation_options.velocity(segment, it)
    }
    value.minZoom?.let {
      fields = fields or MapLibreNativeC.MLN_ANIMATION_OPTION_MIN_ZOOM()
      mln_animation_options.min_zoom(segment, it)
    }
    value.easing?.let {
      fields = fields or MapLibreNativeC.MLN_ANIMATION_OPTION_EASING()
      mln_animation_options.easing(segment).copyFrom(unitBezier(it, arena))
    }
    mln_animation_options.fields(segment, fields)
    return segment
  }

  private fun cameraFitOptions(arena: Arena, value: CameraFitOptions?): MemorySegment {
    if (value == null) {
      return MemorySegment.NULL
    }
    val segment = MapLibreNativeC.mln_camera_fit_options_default(arena)
    var fields = 0
    value.padding?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_FIT_OPTION_PADDING()
      writeEdgeInsets(mln_camera_fit_options.padding(segment), it)
    }
    value.bearing?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_FIT_OPTION_BEARING()
      mln_camera_fit_options.bearing(segment, it)
    }
    value.pitch?.let {
      fields = fields or MapLibreNativeC.MLN_CAMERA_FIT_OPTION_PITCH()
      mln_camera_fit_options.pitch(segment, it)
    }
    mln_camera_fit_options.fields(segment, fields)
    return segment
  }

  private fun mapCameraAnimationCommand(
    functionName: String,
    map: MemorySegment,
    camera: CameraOptions,
    animation: AnimationOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoAddressStatusFunction(functionName)
          .invokeWithArguments(
            map,
            cameraOptions(arena, camera),
            animationOptions(arena, animation),
          ) as Int
      )
    }
  }

  private fun mapLatLngBoundsForCamera(
    functionName: String,
    map: MemorySegment,
    camera: CameraOptions,
  ): LatLngBounds =
    Arena.ofConfined().use { arena ->
      val outBounds = arena.allocate(latLngBoundsLayout)
      Status.check(
        mapTwoAddressStatusFunction(functionName)
          .invokeWithArguments(map, cameraOptions(arena, camera), outBounds) as Int
      )
      latLngBounds(outBounds)
    }

  private fun boundOptionsDefault(arena: Arena): MemorySegment =
    MapLibreNativeC.mln_bound_options_default(arena)

  private fun boundOptions(arena: Arena, value: BoundOptions): MemorySegment {
    val segment = boundOptionsDefault(arena)
    var fields = 0
    value.bounds?.let {
      fields = fields or MapLibreNativeC.MLN_BOUND_OPTION_BOUNDS()
      mln_bound_options.bounds(segment).copyFrom(latLngBounds(arena, it))
    }
    value.minZoom?.let {
      fields = fields or MapLibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM()
      mln_bound_options.min_zoom(segment, it)
    }
    value.maxZoom?.let {
      fields = fields or MapLibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM()
      mln_bound_options.max_zoom(segment, it)
    }
    value.minPitch?.let {
      fields = fields or MapLibreNativeC.MLN_BOUND_OPTION_MIN_PITCH()
      mln_bound_options.min_pitch(segment, it)
    }
    value.maxPitch?.let {
      fields = fields or MapLibreNativeC.MLN_BOUND_OPTION_MAX_PITCH()
      mln_bound_options.max_pitch(segment, it)
    }
    mln_bound_options.fields(segment, fields)
    return segment
  }

  private fun boundOptions(segment: MemorySegment): BoundOptions {
    val fields = mln_bound_options.fields(segment)
    return BoundOptions().apply {
      if ((fields and MapLibreNativeC.MLN_BOUND_OPTION_BOUNDS()) != 0) {
        bounds = latLngBounds(mln_bound_options.bounds(segment))
      }
      if ((fields and MapLibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM()) != 0) {
        minZoom = mln_bound_options.min_zoom(segment)
      }
      if ((fields and MapLibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM()) != 0) {
        maxZoom = mln_bound_options.max_zoom(segment)
      }
      if ((fields and MapLibreNativeC.MLN_BOUND_OPTION_MIN_PITCH()) != 0) {
        minPitch = mln_bound_options.min_pitch(segment)
      }
      if ((fields and MapLibreNativeC.MLN_BOUND_OPTION_MAX_PITCH()) != 0) {
        maxPitch = mln_bound_options.max_pitch(segment)
      }
    }
  }

  private fun freeCameraOptionsDefault(arena: Arena): MemorySegment =
    MapLibreNativeC.mln_free_camera_options_default(arena)

  private fun freeCameraOptions(arena: Arena, value: FreeCameraOptions): MemorySegment {
    val segment = freeCameraOptionsDefault(arena)
    var fields = 0
    value.position?.let {
      fields = fields or MapLibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION()
      mln_free_camera_options.position(segment).copyFrom(vec3(it, arena))
    }
    value.orientation?.let {
      fields = fields or MapLibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION()
      mln_free_camera_options.orientation(segment).copyFrom(quaternion(it, arena))
    }
    mln_free_camera_options.fields(segment, fields)
    return segment
  }

  private fun readFreeCameraOptions(segment: MemorySegment): FreeCameraOptions {
    val fields = mln_free_camera_options.fields(segment)
    return FreeCameraOptions().apply {
      if ((fields and MapLibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION()) != 0) {
        position = vec3(mln_free_camera_options.position(segment))
      }
      if ((fields and MapLibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION()) != 0) {
        orientation = quaternion(mln_free_camera_options.orientation(segment))
      }
    }
  }

  private fun jsonValue(arena: Arena, value: JsonValue): MemorySegment {
    val segment = arena.allocate(JSON_VALUE_SIZE)
    writeJson(segment, value, arena, 0)
    return segment
  }

  private fun geometry(arena: Arena, value: Geometry, depth: Int): MemorySegment {
    Status.requireArgument(depth <= Geometry.MAX_COLLECTION_DEPTH) {
      "Geometry collection depth exceeds ${Geometry.MAX_COLLECTION_DEPTH}"
    }
    val segment = arena.allocate(GEOMETRY_SIZE)
    writeGeometry(segment, value, arena, depth)
    return segment
  }

  private fun writeGeometry(segment: MemorySegment, value: Geometry, arena: Arena, depth: Int) {
    Status.requireArgument(depth <= Geometry.MAX_COLLECTION_DEPTH) {
      "Geometry collection depth exceeds ${Geometry.MAX_COLLECTION_DEPTH}"
    }
    segment.set(ValueLayout.JAVA_INT, GEOMETRY_SIZE_OFFSET, GEOMETRY_SIZE.toInt())
    when (value) {
      Geometry.Empty -> segment.set(ValueLayout.JAVA_INT, GEOMETRY_TYPE_OFFSET, GEOMETRY_EMPTY)
      is Geometry.Point -> {
        segment.set(ValueLayout.JAVA_INT, GEOMETRY_TYPE_OFFSET, GEOMETRY_POINT)
        segment
          .asSlice(GEOMETRY_DATA_OFFSET, latLngLayout.byteSize())
          .copyFrom(latLng(value.coordinate, arena))
      }
      is Geometry.LineString -> {
        segment.set(ValueLayout.JAVA_INT, GEOMETRY_TYPE_OFFSET, GEOMETRY_LINE_STRING)
        segment
          .asSlice(GEOMETRY_DATA_OFFSET, COORDINATE_SPAN_SIZE)
          .copyFrom(coordinateSpan(arena, value.coordinates))
      }
      is Geometry.Polygon -> {
        segment.set(ValueLayout.JAVA_INT, GEOMETRY_TYPE_OFFSET, GEOMETRY_POLYGON)
        segment
          .asSlice(GEOMETRY_DATA_OFFSET, POLYGON_GEOMETRY_SIZE)
          .copyFrom(polygonGeometry(arena, value.rings))
      }
      is Geometry.MultiPoint -> {
        segment.set(ValueLayout.JAVA_INT, GEOMETRY_TYPE_OFFSET, GEOMETRY_MULTI_POINT)
        segment
          .asSlice(GEOMETRY_DATA_OFFSET, COORDINATE_SPAN_SIZE)
          .copyFrom(coordinateSpan(arena, value.coordinates))
      }
      is Geometry.MultiLineString -> {
        segment.set(ValueLayout.JAVA_INT, GEOMETRY_TYPE_OFFSET, GEOMETRY_MULTI_LINE_STRING)
        segment
          .asSlice(GEOMETRY_DATA_OFFSET, MULTI_LINE_GEOMETRY_SIZE)
          .copyFrom(multiLineGeometry(arena, value.lines))
      }
      is Geometry.MultiPolygon -> {
        segment.set(ValueLayout.JAVA_INT, GEOMETRY_TYPE_OFFSET, GEOMETRY_MULTI_POLYGON)
        segment
          .asSlice(GEOMETRY_DATA_OFFSET, MULTI_POLYGON_GEOMETRY_SIZE)
          .copyFrom(multiPolygonGeometry(arena, value.polygons))
      }
      is Geometry.Collection -> {
        segment.set(ValueLayout.JAVA_INT, GEOMETRY_TYPE_OFFSET, GEOMETRY_COLLECTION)
        segment
          .asSlice(GEOMETRY_DATA_OFFSET, GEOMETRY_COLLECTION_SIZE)
          .copyFrom(geometryCollection(arena, value.geometries, depth + 1))
      }
      is Geometry.Unknown ->
        throw Status.invalidArgument("unknown geometries cannot be used as input")
    }
  }

  private fun coordinateSpan(arena: Arena, coordinates: List<LatLng>): MemorySegment {
    val segment = arena.allocate(COORDINATE_SPAN_SIZE)
    segment.set(
      ValueLayout.ADDRESS,
      COORDINATE_SPAN_COORDINATES_OFFSET,
      latLngArray(arena, coordinates),
    )
    segment.set(ValueLayout.JAVA_LONG, COORDINATE_SPAN_COUNT_OFFSET, coordinates.size.toLong())
    return segment
  }

  private fun coordinateSpans(arena: Arena, spans: List<List<LatLng>>): MemorySegment {
    if (spans.isEmpty()) {
      return MemorySegment.NULL
    }
    val array = arena.allocate(COORDINATE_SPAN_SIZE * spans.size)
    spans.forEachIndexed { index, span ->
      array
        .asSlice(COORDINATE_SPAN_SIZE * index, COORDINATE_SPAN_SIZE)
        .copyFrom(coordinateSpan(arena, span))
    }
    return array
  }

  private fun polygonGeometry(arena: Arena, rings: List<List<LatLng>>): MemorySegment {
    val segment = arena.allocate(POLYGON_GEOMETRY_SIZE)
    segment.set(ValueLayout.ADDRESS, POLYGON_GEOMETRY_RINGS_OFFSET, coordinateSpans(arena, rings))
    segment.set(ValueLayout.JAVA_LONG, POLYGON_GEOMETRY_RING_COUNT_OFFSET, rings.size.toLong())
    return segment
  }

  private fun multiLineGeometry(arena: Arena, lines: List<List<LatLng>>): MemorySegment {
    val segment = arena.allocate(MULTI_LINE_GEOMETRY_SIZE)
    segment.set(
      ValueLayout.ADDRESS,
      MULTI_LINE_GEOMETRY_LINES_OFFSET,
      coordinateSpans(arena, lines),
    )
    segment.set(ValueLayout.JAVA_LONG, MULTI_LINE_GEOMETRY_LINE_COUNT_OFFSET, lines.size.toLong())
    return segment
  }

  private fun multiPolygonGeometry(
    arena: Arena,
    polygons: List<List<List<LatLng>>>,
  ): MemorySegment {
    val segment = arena.allocate(MULTI_POLYGON_GEOMETRY_SIZE)
    val nativePolygons =
      if (polygons.isEmpty()) MemorySegment.NULL
      else arena.allocate(POLYGON_GEOMETRY_SIZE * polygons.size)
    polygons.forEachIndexed { index, polygon ->
      nativePolygons
        .asSlice(POLYGON_GEOMETRY_SIZE * index, POLYGON_GEOMETRY_SIZE)
        .copyFrom(polygonGeometry(arena, polygon))
    }
    segment.set(ValueLayout.ADDRESS, MULTI_POLYGON_GEOMETRY_POLYGONS_OFFSET, nativePolygons)
    segment.set(
      ValueLayout.JAVA_LONG,
      MULTI_POLYGON_GEOMETRY_POLYGON_COUNT_OFFSET,
      polygons.size.toLong(),
    )
    return segment
  }

  private fun geometryCollection(
    arena: Arena,
    geometries: List<Geometry>,
    depth: Int,
  ): MemorySegment {
    val segment = arena.allocate(GEOMETRY_COLLECTION_SIZE)
    val nativeGeometries =
      if (geometries.isEmpty()) MemorySegment.NULL
      else arena.allocate(GEOMETRY_SIZE * geometries.size)
    geometries.forEachIndexed { index, geometry ->
      writeGeometry(
        nativeGeometries.asSlice(GEOMETRY_SIZE * index, GEOMETRY_SIZE),
        geometry,
        arena,
        depth,
      )
    }
    segment.set(ValueLayout.ADDRESS, GEOMETRY_COLLECTION_GEOMETRIES_OFFSET, nativeGeometries)
    segment.set(
      ValueLayout.JAVA_LONG,
      GEOMETRY_COLLECTION_GEOMETRY_COUNT_OFFSET,
      geometries.size.toLong(),
    )
    return segment
  }

  private fun geometry(segment: MemorySegment, depth: Int = 0): Geometry {
    Status.requireArgument(depth <= Geometry.MAX_COLLECTION_DEPTH) {
      "Geometry collection depth exceeds ${Geometry.MAX_COLLECTION_DEPTH}"
    }
    return when (val type = segment.get(ValueLayout.JAVA_INT, GEOMETRY_TYPE_OFFSET)) {
      GEOMETRY_EMPTY -> Geometry.Empty
      GEOMETRY_POINT ->
        Geometry.Point(latLng(segment.asSlice(GEOMETRY_DATA_OFFSET, latLngLayout.byteSize())))
      GEOMETRY_LINE_STRING ->
        Geometry.LineString(
          coordinateSpan(segment.asSlice(GEOMETRY_DATA_OFFSET, COORDINATE_SPAN_SIZE))
        )
      GEOMETRY_POLYGON ->
        Geometry.Polygon(
          polygonGeometry(segment.asSlice(GEOMETRY_DATA_OFFSET, POLYGON_GEOMETRY_SIZE))
        )
      GEOMETRY_MULTI_POINT ->
        Geometry.MultiPoint(
          coordinateSpan(segment.asSlice(GEOMETRY_DATA_OFFSET, COORDINATE_SPAN_SIZE))
        )
      GEOMETRY_MULTI_LINE_STRING -> {
        val value = segment.asSlice(GEOMETRY_DATA_OFFSET, MULTI_LINE_GEOMETRY_SIZE)
        val count =
          checkedInt(value.get(ValueLayout.JAVA_LONG, MULTI_LINE_GEOMETRY_LINE_COUNT_OFFSET))
        val lines = value.get(ValueLayout.ADDRESS, MULTI_LINE_GEOMETRY_LINES_OFFSET)
        Geometry.MultiLineString(
          List(count) { index ->
            coordinateSpan(
              lines
                .reinterpret(COORDINATE_SPAN_SIZE * count)
                .asSlice(index * COORDINATE_SPAN_SIZE, COORDINATE_SPAN_SIZE)
            )
          }
        )
      }
      GEOMETRY_MULTI_POLYGON -> {
        val value = segment.asSlice(GEOMETRY_DATA_OFFSET, MULTI_POLYGON_GEOMETRY_SIZE)
        val count =
          checkedInt(value.get(ValueLayout.JAVA_LONG, MULTI_POLYGON_GEOMETRY_POLYGON_COUNT_OFFSET))
        val polygons = value.get(ValueLayout.ADDRESS, MULTI_POLYGON_GEOMETRY_POLYGONS_OFFSET)
        Geometry.MultiPolygon(
          List(count) { index ->
            polygonGeometry(
              polygons
                .reinterpret(POLYGON_GEOMETRY_SIZE * count)
                .asSlice(index * POLYGON_GEOMETRY_SIZE, POLYGON_GEOMETRY_SIZE)
            )
          }
        )
      }
      GEOMETRY_COLLECTION -> {
        val value = segment.asSlice(GEOMETRY_DATA_OFFSET, GEOMETRY_COLLECTION_SIZE)
        val count =
          checkedInt(value.get(ValueLayout.JAVA_LONG, GEOMETRY_COLLECTION_GEOMETRY_COUNT_OFFSET))
        val geometries = value.get(ValueLayout.ADDRESS, GEOMETRY_COLLECTION_GEOMETRIES_OFFSET)
        Geometry.Collection(
          List(count) { index ->
            geometry(
              geometries
                .reinterpret(GEOMETRY_SIZE * count)
                .asSlice(index * GEOMETRY_SIZE, GEOMETRY_SIZE),
              depth + 1,
            )
          }
        )
      }
      else -> Geometry.Unknown(type, segment.get(ValueLayout.JAVA_INT, GEOMETRY_SIZE_OFFSET))
    }
  }

  private fun coordinateSpan(segment: MemorySegment): List<LatLng> {
    val count = checkedInt(segment.get(ValueLayout.JAVA_LONG, COORDINATE_SPAN_COUNT_OFFSET))
    return latLngArray(segment.get(ValueLayout.ADDRESS, COORDINATE_SPAN_COORDINATES_OFFSET), count)
  }

  private fun polygonGeometry(segment: MemorySegment): List<List<LatLng>> {
    val count = checkedInt(segment.get(ValueLayout.JAVA_LONG, POLYGON_GEOMETRY_RING_COUNT_OFFSET))
    val rings = segment.get(ValueLayout.ADDRESS, POLYGON_GEOMETRY_RINGS_OFFSET)
    return List(count) { index ->
      coordinateSpan(
        rings
          .reinterpret(COORDINATE_SPAN_SIZE * count)
          .asSlice(index * COORDINATE_SPAN_SIZE, COORDINATE_SPAN_SIZE)
      )
    }
  }

  private fun feature(arena: Arena, value: Feature, depth: Int): MemorySegment {
    val segment = arena.allocate(FEATURE_SIZE)
    writeFeature(segment, value, arena, depth)
    return segment
  }

  private fun writeFeature(segment: MemorySegment, value: Feature, arena: Arena, depth: Int) {
    segment.set(ValueLayout.JAVA_INT, FEATURE_SIZE_OFFSET, FEATURE_SIZE.toInt())
    segment.set(
      ValueLayout.ADDRESS,
      FEATURE_GEOMETRY_OFFSET,
      geometry(arena, value.geometry, depth + 1),
    )
    segment.set(
      ValueLayout.ADDRESS,
      FEATURE_PROPERTIES_OFFSET,
      jsonMembers(arena, value.properties, depth + 1),
    )
    segment.set(
      ValueLayout.JAVA_LONG,
      FEATURE_PROPERTY_COUNT_OFFSET,
      value.properties.size.toLong(),
    )
    writeFeatureIdentifier(segment, value.identifier, arena)
  }

  private fun writeFeatureIdentifier(
    segment: MemorySegment,
    value: FeatureIdentifier,
    arena: Arena,
  ) {
    when (value) {
      FeatureIdentifier.Null ->
        segment.set(ValueLayout.JAVA_INT, FEATURE_IDENTIFIER_TYPE_OFFSET, FEATURE_IDENTIFIER_NULL)
      is FeatureIdentifier.UInt -> {
        segment.set(ValueLayout.JAVA_INT, FEATURE_IDENTIFIER_TYPE_OFFSET, FEATURE_IDENTIFIER_UINT)
        segment.set(ValueLayout.JAVA_LONG, FEATURE_IDENTIFIER_OFFSET, value.value)
      }
      is FeatureIdentifier.Int -> {
        segment.set(ValueLayout.JAVA_INT, FEATURE_IDENTIFIER_TYPE_OFFSET, FEATURE_IDENTIFIER_INT)
        segment.set(ValueLayout.JAVA_LONG, FEATURE_IDENTIFIER_OFFSET, value.value)
      }
      is FeatureIdentifier.DoubleValue -> {
        segment.set(ValueLayout.JAVA_INT, FEATURE_IDENTIFIER_TYPE_OFFSET, FEATURE_IDENTIFIER_DOUBLE)
        segment.set(ValueLayout.JAVA_DOUBLE, FEATURE_IDENTIFIER_OFFSET, value.value)
      }
      is FeatureIdentifier.StringValue -> {
        segment.set(ValueLayout.JAVA_INT, FEATURE_IDENTIFIER_TYPE_OFFSET, FEATURE_IDENTIFIER_STRING)
        segment
          .asSlice(FEATURE_IDENTIFIER_OFFSET, STRING_VIEW_SIZE)
          .copyFrom(stringView(arena, value.value))
      }
      is FeatureIdentifier.Unknown ->
        throw Status.invalidArgument("unknown feature identifiers cannot be used as input")
    }
  }

  private fun feature(segment: MemorySegment): Feature {
    val properties =
      List(checkedInt(segment.get(ValueLayout.JAVA_LONG, FEATURE_PROPERTY_COUNT_OFFSET))) { index ->
        val members = segment.get(ValueLayout.ADDRESS, FEATURE_PROPERTIES_OFFSET)
        val member =
          members
            .reinterpret(JSON_MEMBER_SIZE * (index + 1))
            .asSlice(index * JSON_MEMBER_SIZE, JSON_MEMBER_SIZE)
        JsonValue.Member(
          stringView(member.asSlice(JSON_MEMBER_KEY_OFFSET, STRING_VIEW_SIZE)),
          readJson(
            member.get(ValueLayout.ADDRESS, JSON_MEMBER_VALUE_OFFSET).reinterpret(JSON_VALUE_SIZE),
            0,
          ),
        )
      }
    return Feature(
      geometry(
        segment.get(ValueLayout.ADDRESS, FEATURE_GEOMETRY_OFFSET).reinterpret(GEOMETRY_SIZE)
      ),
      properties,
      featureIdentifier(segment),
    )
  }

  private fun featureIdentifier(segment: MemorySegment): FeatureIdentifier =
    when (val type = segment.get(ValueLayout.JAVA_INT, FEATURE_IDENTIFIER_TYPE_OFFSET)) {
      FEATURE_IDENTIFIER_NULL -> FeatureIdentifier.Null
      FEATURE_IDENTIFIER_UINT ->
        FeatureIdentifier.UInt(segment.get(ValueLayout.JAVA_LONG, FEATURE_IDENTIFIER_OFFSET))
      FEATURE_IDENTIFIER_INT ->
        FeatureIdentifier.Int(segment.get(ValueLayout.JAVA_LONG, FEATURE_IDENTIFIER_OFFSET))
      FEATURE_IDENTIFIER_DOUBLE ->
        FeatureIdentifier.DoubleValue(
          segment.get(ValueLayout.JAVA_DOUBLE, FEATURE_IDENTIFIER_OFFSET)
        )
      FEATURE_IDENTIFIER_STRING ->
        FeatureIdentifier.StringValue(
          stringView(segment.asSlice(FEATURE_IDENTIFIER_OFFSET, STRING_VIEW_SIZE))
        )
      else -> FeatureIdentifier.Unknown(type)
    }

  private fun geoJson(arena: Arena, value: GeoJson): MemorySegment {
    val segment = arena.allocate(GEOJSON_SIZE)
    segment.set(ValueLayout.JAVA_INT, GEOJSON_SIZE_OFFSET, GEOJSON_SIZE.toInt())
    when (value) {
      is GeoJson.GeometryValue -> {
        segment.set(ValueLayout.JAVA_INT, GEOJSON_TYPE_OFFSET, GEOJSON_GEOMETRY)
        segment.set(ValueLayout.ADDRESS, GEOJSON_DATA_OFFSET, geometry(arena, value.geometry, 0))
      }
      is GeoJson.FeatureValue -> {
        segment.set(ValueLayout.JAVA_INT, GEOJSON_TYPE_OFFSET, GEOJSON_FEATURE)
        segment.set(ValueLayout.ADDRESS, GEOJSON_DATA_OFFSET, feature(arena, value.feature, 0))
      }
      is GeoJson.FeatureCollection -> {
        segment.set(ValueLayout.JAVA_INT, GEOJSON_TYPE_OFFSET, GEOJSON_FEATURE_COLLECTION)
        val nativeFeatures =
          if (value.features.isEmpty()) MemorySegment.NULL
          else arena.allocate(FEATURE_SIZE * value.features.size)
        value.features.forEachIndexed { index, feature ->
          writeFeature(
            nativeFeatures.asSlice(FEATURE_SIZE * index, FEATURE_SIZE),
            feature,
            arena,
            1,
          )
        }
        segment.set(ValueLayout.ADDRESS, GEOJSON_DATA_OFFSET, nativeFeatures)
        segment.set(
          ValueLayout.JAVA_LONG,
          GEOJSON_DATA_OFFSET + Long.SIZE_BYTES,
          value.features.size.toLong(),
        )
      }
    }
    return segment
  }

  private fun premultipliedRgba8Image(arena: Arena, value: PremultipliedRgba8Image): MemorySegment {
    val pixels = value.pixels
    val segment = MapLibreNativeC.mln_premultiplied_rgba8_image_default(arena)
    mln_premultiplied_rgba8_image.width(segment, value.width)
    mln_premultiplied_rgba8_image.height(segment, value.height)
    mln_premultiplied_rgba8_image.stride(segment, value.stride)
    mln_premultiplied_rgba8_image.pixels(segment, nativeBytes(arena, pixels))
    mln_premultiplied_rgba8_image.byte_length(segment, pixels.size.toLong())
    return segment
  }

  private fun styleImageOptions(arena: Arena, value: StyleImageOptions): MemorySegment {
    val segment = arena.allocate(STYLE_IMAGE_OPTIONS_SIZE)
    var fields = 0
    segment.set(
      ValueLayout.JAVA_INT,
      STYLE_IMAGE_OPTIONS_SIZE_OFFSET,
      STYLE_IMAGE_OPTIONS_SIZE.toInt(),
    )
    segment.set(ValueLayout.JAVA_FLOAT, STYLE_IMAGE_OPTIONS_PIXEL_RATIO_OFFSET, DEFAULT_PIXEL_RATIO)
    segment.set(ValueLayout.JAVA_BOOLEAN, STYLE_IMAGE_OPTIONS_SDF_OFFSET, false)
    value.pixelRatio?.let {
      fields = fields or STYLE_IMAGE_OPTION_PIXEL_RATIO
      segment.set(ValueLayout.JAVA_FLOAT, STYLE_IMAGE_OPTIONS_PIXEL_RATIO_OFFSET, it)
    }
    value.sdf?.let {
      fields = fields or STYLE_IMAGE_OPTION_SDF
      segment.set(ValueLayout.JAVA_BOOLEAN, STYLE_IMAGE_OPTIONS_SDF_OFFSET, it)
    }
    segment.set(ValueLayout.JAVA_INT, STYLE_IMAGE_OPTIONS_FIELDS_OFFSET, fields)
    return segment
  }

  private fun styleImageInfoDefault(arena: Arena): MemorySegment {
    val segment = arena.allocate(STYLE_IMAGE_INFO_SIZE)
    segment.set(ValueLayout.JAVA_INT, STYLE_IMAGE_INFO_SIZE_OFFSET, STYLE_IMAGE_INFO_SIZE.toInt())
    segment.set(ValueLayout.JAVA_FLOAT, STYLE_IMAGE_INFO_PIXEL_RATIO_OFFSET, DEFAULT_PIXEL_RATIO)
    return segment
  }

  private fun styleImageInfo(segment: MemorySegment): StyleImageInfo =
    StyleImageInfo(
      segment.get(ValueLayout.JAVA_INT, STYLE_IMAGE_INFO_WIDTH_OFFSET),
      segment.get(ValueLayout.JAVA_INT, STYLE_IMAGE_INFO_HEIGHT_OFFSET),
      segment.get(ValueLayout.JAVA_INT, STYLE_IMAGE_INFO_STRIDE_OFFSET),
      segment.get(ValueLayout.JAVA_LONG, STYLE_IMAGE_INFO_BYTE_LENGTH_OFFSET),
      segment.get(ValueLayout.JAVA_FLOAT, STYLE_IMAGE_INFO_PIXEL_RATIO_OFFSET),
      segment.get(ValueLayout.JAVA_BOOLEAN, STYLE_IMAGE_INFO_SDF_OFFSET),
    )

  private fun debugOptions(mask: Int): Set<DebugOption> =
    DebugOption.entries.filterTo(mutableSetOf()) { option -> (mask and option.nativeMask) != 0 }

  private fun jsonMembers(
    arena: Arena,
    members: List<JsonValue.Member>,
    depth: Int,
  ): MemorySegment {
    val nativeMembers =
      if (members.isEmpty()) MemorySegment.NULL else arena.allocate(JSON_MEMBER_SIZE * members.size)
    members.forEachIndexed { index, member ->
      val memberSegment = nativeMembers.asSlice(index * JSON_MEMBER_SIZE, JSON_MEMBER_SIZE)
      memberSegment
        .asSlice(JSON_MEMBER_KEY_OFFSET, STRING_VIEW_SIZE)
        .copyFrom(stringView(arena, member.key))
      val nativeValue = arena.allocate(JSON_VALUE_SIZE)
      writeJson(nativeValue, member.value, arena, depth)
      memberSegment.set(ValueLayout.ADDRESS, JSON_MEMBER_VALUE_OFFSET, nativeValue)
    }
    return nativeMembers
  }

  private fun writeJson(segment: MemorySegment, value: JsonValue, arena: Arena, depth: Int) {
    Status.requireArgument(depth <= JsonValue.MAX_DESCRIPTOR_DEPTH) {
      "JSON descriptor depth exceeds ${JsonValue.MAX_DESCRIPTOR_DEPTH}"
    }
    segment.set(ValueLayout.JAVA_INT, JSON_VALUE_SIZE_OFFSET, JSON_VALUE_SIZE.toInt())
    when (value) {
      JsonValue.Null -> segment.set(ValueLayout.JAVA_INT, JSON_VALUE_TYPE_OFFSET, JSON_NULL)
      is JsonValue.Bool -> {
        segment.set(ValueLayout.JAVA_INT, JSON_VALUE_TYPE_OFFSET, JSON_BOOL)
        segment.set(ValueLayout.JAVA_BOOLEAN, JSON_VALUE_DATA_OFFSET, value.value)
      }
      is JsonValue.UInt -> {
        segment.set(ValueLayout.JAVA_INT, JSON_VALUE_TYPE_OFFSET, JSON_UINT)
        segment.set(ValueLayout.JAVA_LONG, JSON_VALUE_DATA_OFFSET, value.value)
      }
      is JsonValue.Int -> {
        segment.set(ValueLayout.JAVA_INT, JSON_VALUE_TYPE_OFFSET, JSON_INT)
        segment.set(ValueLayout.JAVA_LONG, JSON_VALUE_DATA_OFFSET, value.value)
      }
      is JsonValue.DoubleValue -> {
        segment.set(ValueLayout.JAVA_INT, JSON_VALUE_TYPE_OFFSET, JSON_DOUBLE)
        segment.set(ValueLayout.JAVA_DOUBLE, JSON_VALUE_DATA_OFFSET, value.value)
      }
      is JsonValue.StringValue -> {
        segment.set(ValueLayout.JAVA_INT, JSON_VALUE_TYPE_OFFSET, JSON_STRING)
        segment
          .asSlice(JSON_VALUE_DATA_OFFSET, STRING_VIEW_SIZE)
          .copyFrom(stringView(arena, value.value))
      }
      is JsonValue.Array -> {
        segment.set(ValueLayout.JAVA_INT, JSON_VALUE_TYPE_OFFSET, JSON_ARRAY)
        val nativeValues =
          if (value.values.isEmpty()) MemorySegment.NULL
          else arena.allocate(JSON_VALUE_SIZE * value.values.size)
        value.values.forEachIndexed { index, child ->
          writeJson(
            nativeValues.asSlice(index * JSON_VALUE_SIZE, JSON_VALUE_SIZE),
            child,
            arena,
            depth + 1,
          )
        }
        segment.set(ValueLayout.ADDRESS, JSON_VALUE_DATA_OFFSET, nativeValues)
        segment.set(
          ValueLayout.JAVA_LONG,
          JSON_VALUE_DATA_OFFSET + Long.SIZE_BYTES,
          value.values.size.toLong(),
        )
      }
      is JsonValue.ObjectValue -> {
        segment.set(ValueLayout.JAVA_INT, JSON_VALUE_TYPE_OFFSET, JSON_OBJECT)
        val nativeMembers = jsonMembers(arena, value.members, depth + 1)
        segment.set(ValueLayout.ADDRESS, JSON_VALUE_DATA_OFFSET, nativeMembers)
        segment.set(
          ValueLayout.JAVA_LONG,
          JSON_VALUE_DATA_OFFSET + Long.SIZE_BYTES,
          value.members.size.toLong(),
        )
      }
      is JsonValue.Unknown ->
        throw Status.invalidArgument("unknown JSON values cannot be used as input")
    }
  }

  private fun jsonSnapshot(snapshot: MemorySegment): JsonValue? {
    if (snapshot == MemorySegment.NULL) {
      return null
    }
    return try {
      Arena.ofConfined().use { arena ->
        val outValue = arena.allocate(ValueLayout.ADDRESS)
        outValue.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
        Status.check(jsonSnapshotGetFunction().invokeWithArguments(snapshot, outValue) as Int)
        val value = outValue.get(ValueLayout.ADDRESS, 0)
        if (value == MemorySegment.NULL) null else readJson(value.reinterpret(JSON_VALUE_SIZE), 0)
      }
    } finally {
      jsonSnapshotDestroyFunction().invokeWithArguments(snapshot)
    }
  }

  private fun featureQueryResult(result: MemorySegment): List<QueriedFeature> =
    try {
      Arena.ofConfined().use { arena ->
        val outCount = arena.allocate(ValueLayout.JAVA_LONG)
        Status.check(featureQueryResultCountFunction().invokeWithArguments(result, outCount) as Int)
        val count = checkedInt(outCount.get(ValueLayout.JAVA_LONG, 0))
        List(count) { index ->
          val outFeature = arena.allocate(QUERIED_FEATURE_SIZE)
          outFeature.set(
            ValueLayout.JAVA_INT,
            QUERIED_FEATURE_SIZE_OFFSET,
            QUERIED_FEATURE_SIZE.toInt(),
          )
          Status.check(
            featureQueryResultGetFunction().invokeWithArguments(result, index.toLong(), outFeature)
              as Int
          )
          queriedFeature(outFeature)
        }
      }
    } finally {
      featureQueryResultDestroyFunction().invokeWithArguments(result)
    }

  private fun queriedFeature(segment: MemorySegment): QueriedFeature {
    val fields = segment.get(ValueLayout.JAVA_INT, QUERIED_FEATURE_FIELDS_OFFSET)
    val sourceId =
      if ((fields and QUERIED_FEATURE_SOURCE_ID) != 0)
        stringView(segment.asSlice(QUERIED_FEATURE_SOURCE_ID_OFFSET, STRING_VIEW_SIZE))
      else null
    val sourceLayerId =
      if ((fields and QUERIED_FEATURE_SOURCE_LAYER_ID) != 0)
        stringView(segment.asSlice(QUERIED_FEATURE_SOURCE_LAYER_ID_OFFSET, STRING_VIEW_SIZE))
      else null
    val state =
      if ((fields and QUERIED_FEATURE_STATE) != 0) {
        val value = segment.get(ValueLayout.ADDRESS, QUERIED_FEATURE_STATE_OFFSET)
        if (value == MemorySegment.NULL) null else readJson(value.reinterpret(JSON_VALUE_SIZE), 0)
      } else {
        null
      }
    return QueriedFeature(
      feature(segment.asSlice(QUERIED_FEATURE_FEATURE_OFFSET, FEATURE_SIZE)),
      sourceId,
      sourceLayerId,
      state,
    )
  }

  private fun featureExtensionResult(result: MemorySegment): FeatureExtensionResult =
    try {
      Arena.ofConfined().use { arena ->
        val info = arena.allocate(FEATURE_EXTENSION_RESULT_INFO_SIZE)
        info.set(
          ValueLayout.JAVA_INT,
          FEATURE_EXTENSION_RESULT_INFO_SIZE_OFFSET,
          FEATURE_EXTENSION_RESULT_INFO_SIZE.toInt(),
        )
        Status.check(featureExtensionResultGetFunction().invokeWithArguments(result, info) as Int)
        when (
          val type = info.get(ValueLayout.JAVA_INT, FEATURE_EXTENSION_RESULT_INFO_TYPE_OFFSET)
        ) {
          FEATURE_EXTENSION_RESULT_VALUE ->
            FeatureExtensionResult.Value(
              readJson(
                info
                  .get(ValueLayout.ADDRESS, FEATURE_EXTENSION_RESULT_INFO_DATA_OFFSET)
                  .reinterpret(JSON_VALUE_SIZE),
                0,
              )
            )
          FEATURE_EXTENSION_RESULT_FEATURE_COLLECTION ->
            FeatureExtensionResult.FeatureCollection(
              featureCollection(
                info.asSlice(FEATURE_EXTENSION_RESULT_INFO_DATA_OFFSET, FEATURE_COLLECTION_SIZE)
              )
            )
          else -> FeatureExtensionResult.Unknown(type)
        }
      }
    } finally {
      featureExtensionResultDestroyFunction().invokeWithArguments(result)
    }

  private fun featureCollection(segment: MemorySegment): List<Feature> {
    val count =
      checkedInt(segment.get(ValueLayout.JAVA_LONG, FEATURE_COLLECTION_FEATURE_COUNT_OFFSET))
    val features = segment.get(ValueLayout.ADDRESS, FEATURE_COLLECTION_FEATURES_OFFSET)
    return List(count) { index ->
      feature(
        features.reinterpret(FEATURE_SIZE * count).asSlice(index * FEATURE_SIZE, FEATURE_SIZE)
      )
    }
  }

  private fun readJson(segment: MemorySegment, depth: Int): JsonValue {
    Status.requireArgument(depth <= JsonValue.MAX_DESCRIPTOR_DEPTH) {
      "JSON descriptor depth exceeds ${JsonValue.MAX_DESCRIPTOR_DEPTH}"
    }
    return when (val type = segment.get(ValueLayout.JAVA_INT, JSON_VALUE_TYPE_OFFSET)) {
      JSON_NULL -> JsonValue.Null
      JSON_BOOL -> JsonValue.Bool(segment.get(ValueLayout.JAVA_BOOLEAN, JSON_VALUE_DATA_OFFSET))
      JSON_UINT -> JsonValue.UInt(segment.get(ValueLayout.JAVA_LONG, JSON_VALUE_DATA_OFFSET))
      JSON_INT -> JsonValue.Int(segment.get(ValueLayout.JAVA_LONG, JSON_VALUE_DATA_OFFSET))
      JSON_DOUBLE ->
        JsonValue.DoubleValue(segment.get(ValueLayout.JAVA_DOUBLE, JSON_VALUE_DATA_OFFSET))
      JSON_STRING ->
        JsonValue.StringValue(stringView(segment.asSlice(JSON_VALUE_DATA_OFFSET, STRING_VIEW_SIZE)))
      JSON_ARRAY -> {
        val count =
          Math.toIntExact(
            segment.get(ValueLayout.JAVA_LONG, JSON_VALUE_DATA_OFFSET + Long.SIZE_BYTES)
          )
        val values = segment.get(ValueLayout.ADDRESS, JSON_VALUE_DATA_OFFSET)
        JsonValue.Array(
          List(count) { index ->
            readJson(
              values
                .reinterpret(JSON_VALUE_SIZE * count)
                .asSlice(index * JSON_VALUE_SIZE, JSON_VALUE_SIZE),
              depth + 1,
            )
          }
        )
      }
      JSON_OBJECT -> {
        val count =
          Math.toIntExact(
            segment.get(ValueLayout.JAVA_LONG, JSON_VALUE_DATA_OFFSET + Long.SIZE_BYTES)
          )
        val members = segment.get(ValueLayout.ADDRESS, JSON_VALUE_DATA_OFFSET)
        JsonValue.ObjectValue(
          List(count) { index ->
            val member =
              members
                .reinterpret(JSON_MEMBER_SIZE * count)
                .asSlice(index * JSON_MEMBER_SIZE, JSON_MEMBER_SIZE)
            val value = member.get(ValueLayout.ADDRESS, JSON_MEMBER_VALUE_OFFSET)
            JsonValue.Member(
              stringView(member.asSlice(JSON_MEMBER_KEY_OFFSET, STRING_VIEW_SIZE)),
              readJson(value.reinterpret(JSON_VALUE_SIZE), depth + 1),
            )
          }
        )
      }
      else -> JsonValue.Unknown(type, segment.get(ValueLayout.JAVA_INT, JSON_VALUE_SIZE_OFFSET))
    }
  }

  private fun latLng(value: LatLng, arena: Arena): MemorySegment {
    val segment = arena.allocate(latLngLayout)
    segment.set(ValueLayout.JAVA_DOUBLE, 0, value.latitude)
    segment.set(ValueLayout.JAVA_DOUBLE, Double.SIZE_BYTES.toLong(), value.longitude)
    return segment
  }

  private fun latLng(segment: MemorySegment): LatLng =
    LatLng(
      segment.get(ValueLayout.JAVA_DOUBLE, 0),
      segment.get(ValueLayout.JAVA_DOUBLE, Double.SIZE_BYTES.toLong()),
    )

  private fun latLngArray(arena: Arena, values: List<LatLng>): MemorySegment {
    if (values.isEmpty()) {
      return MemorySegment.NULL
    }
    val array = arena.allocate(latLngLayout.byteSize() * values.size)
    values.forEachIndexed { index, value ->
      array
        .asSlice(latLngLayout.byteSize() * index, latLngLayout.byteSize())
        .copyFrom(latLng(value, arena))
    }
    return array
  }

  private fun latLngArray(segment: MemorySegment, count: Int): List<LatLng> =
    List(count) { index ->
      val coordinate =
        segment
          .reinterpret(latLngLayout.byteSize() * count)
          .asSlice(latLngLayout.byteSize() * index, latLngLayout.byteSize())
      LatLng(
        coordinate.get(ValueLayout.JAVA_DOUBLE, 0),
        coordinate.get(ValueLayout.JAVA_DOUBLE, Double.SIZE_BYTES.toLong()),
      )
    }

  private fun screenPoint(value: ScreenPoint, arena: Arena): MemorySegment {
    val segment = arena.allocate(screenPointLayout)
    segment.set(ValueLayout.JAVA_DOUBLE, 0, value.x)
    segment.set(ValueLayout.JAVA_DOUBLE, Double.SIZE_BYTES.toLong(), value.y)
    return segment
  }

  private fun screenPoint(segment: MemorySegment): ScreenPoint =
    ScreenPoint(
      segment.get(ValueLayout.JAVA_DOUBLE, 0),
      segment.get(ValueLayout.JAVA_DOUBLE, Double.SIZE_BYTES.toLong()),
    )

  private fun screenPointArray(arena: Arena, values: List<ScreenPoint>): MemorySegment {
    if (values.isEmpty()) {
      return MemorySegment.NULL
    }
    val array = arena.allocate(screenPointLayout.byteSize() * values.size)
    values.forEachIndexed { index, value ->
      array
        .asSlice(screenPointLayout.byteSize() * index, screenPointLayout.byteSize())
        .copyFrom(screenPoint(value, arena))
    }
    return array
  }

  private fun screenPointArray(segment: MemorySegment, count: Int): List<ScreenPoint> =
    List(count) { index ->
      val point =
        segment
          .reinterpret(screenPointLayout.byteSize() * count)
          .asSlice(screenPointLayout.byteSize() * index, screenPointLayout.byteSize())
      screenPoint(point)
    }

  private fun screenBox(arena: Arena, value: ScreenBox): MemorySegment {
    val segment = arena.allocate(SCREEN_BOX_SIZE)
    segment
      .asSlice(SCREEN_BOX_MIN_OFFSET, SCREEN_POINT_SIZE)
      .copyFrom(screenPoint(value.min, arena))
    segment
      .asSlice(SCREEN_BOX_MAX_OFFSET, SCREEN_POINT_SIZE)
      .copyFrom(screenPoint(value.max, arena))
    return segment
  }

  private fun screenLineString(arena: Arena, values: List<ScreenPoint>): MemorySegment {
    val segment = arena.allocate(SCREEN_LINE_STRING_SIZE)
    segment.set(
      ValueLayout.ADDRESS,
      SCREEN_LINE_STRING_POINTS_OFFSET,
      screenPointArray(arena, values),
    )
    segment.set(ValueLayout.JAVA_LONG, SCREEN_LINE_STRING_POINT_COUNT_OFFSET, values.size.toLong())
    return segment
  }

  private fun unitBezier(value: UnitBezier, arena: Arena): MemorySegment {
    val segment = arena.allocate(unitBezierLayout)
    segment.set(ValueLayout.JAVA_DOUBLE, UNIT_BEZIER_X1_OFFSET, value.x1)
    segment.set(ValueLayout.JAVA_DOUBLE, UNIT_BEZIER_Y1_OFFSET, value.y1)
    segment.set(ValueLayout.JAVA_DOUBLE, UNIT_BEZIER_X2_OFFSET, value.x2)
    segment.set(ValueLayout.JAVA_DOUBLE, UNIT_BEZIER_Y2_OFFSET, value.y2)
    return segment
  }

  private fun vec3(value: Vec3, arena: Arena): MemorySegment {
    val segment = arena.allocate(vec3Layout)
    segment.set(ValueLayout.JAVA_DOUBLE, VEC3_X_OFFSET, value.x)
    segment.set(ValueLayout.JAVA_DOUBLE, VEC3_Y_OFFSET, value.y)
    segment.set(ValueLayout.JAVA_DOUBLE, VEC3_Z_OFFSET, value.z)
    return segment
  }

  private fun vec3(segment: MemorySegment): Vec3 =
    Vec3(
      segment.get(ValueLayout.JAVA_DOUBLE, VEC3_X_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, VEC3_Y_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, VEC3_Z_OFFSET),
    )

  private fun quaternion(value: Quaternion, arena: Arena): MemorySegment {
    val segment = arena.allocate(quaternionLayout)
    segment.set(ValueLayout.JAVA_DOUBLE, QUATERNION_X_OFFSET, value.x)
    segment.set(ValueLayout.JAVA_DOUBLE, QUATERNION_Y_OFFSET, value.y)
    segment.set(ValueLayout.JAVA_DOUBLE, QUATERNION_Z_OFFSET, value.z)
    segment.set(ValueLayout.JAVA_DOUBLE, QUATERNION_W_OFFSET, value.w)
    return segment
  }

  private fun quaternion(segment: MemorySegment): Quaternion =
    Quaternion(
      segment.get(ValueLayout.JAVA_DOUBLE, QUATERNION_X_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, QUATERNION_Y_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, QUATERNION_Z_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, QUATERNION_W_OFFSET),
    )

  private fun nativeBytes(arena: Arena, bytes: ByteArray): MemorySegment {
    if (bytes.isEmpty()) {
      return MemorySegment.NULL
    }
    val segment = arena.allocate(bytes.size.toLong())
    MemorySegment.copy(bytes, 0, segment, ValueLayout.JAVA_BYTE, 0, bytes.size)
    return segment
  }

  private fun copyBytes(address: MemorySegment, byteCount: Long): ByteArray {
    if (address == MemorySegment.NULL || byteCount == 0L) {
      return ByteArray(0)
    }
    return address.reinterpret(byteCount).toArray(ValueLayout.JAVA_BYTE)
  }

  private fun copyString(address: MemorySegment, byteCount: Long): String =
    String(copyBytes(address, byteCount), StandardCharsets.UTF_8)

  private fun boundedAddress(address: MemorySegment, byteCount: Long): MemorySegment {
    if (address == MemorySegment.NULL || byteCount == 0L) {
      return MemorySegment.NULL
    }
    return address.reinterpret(byteCount)
  }

  private fun checkedInt(value: Long): Int {
    require(value <= Int.MAX_VALUE) { "native count exceeds Int.MAX_VALUE" }
    require(value >= 0L) { "native count must be non-negative" }
    return value.toInt()
  }

  private fun stringView(segment: MemorySegment): String =
    copyString(
      segment.get(ValueLayout.ADDRESS, STRING_VIEW_DATA_OFFSET),
      segment.get(ValueLayout.JAVA_LONG, STRING_VIEW_SIZE_OFFSET),
    )

  internal fun resourceRequest(request: MemorySegment): ResourceRequest =
    ResourceRequest(
      copyCString(request.get(ValueLayout.ADDRESS, RESOURCE_REQUEST_URL_OFFSET)),
      ResourceKind.fromNative(request.get(ValueLayout.JAVA_INT, RESOURCE_REQUEST_KIND_OFFSET)),
      ResourceLoadingMethod.fromNative(
        request.get(ValueLayout.JAVA_INT, RESOURCE_REQUEST_LOADING_METHOD_OFFSET)
      ),
      ResourcePriority.fromNative(
        request.get(ValueLayout.JAVA_INT, RESOURCE_REQUEST_PRIORITY_OFFSET)
      ),
      ResourceUsage.fromNative(request.get(ValueLayout.JAVA_INT, RESOURCE_REQUEST_USAGE_OFFSET)),
      ResourceStoragePolicy.fromNative(
        request.get(ValueLayout.JAVA_INT, RESOURCE_REQUEST_STORAGE_POLICY_OFFSET)
      ),
      if (request.get(ValueLayout.JAVA_BOOLEAN, RESOURCE_REQUEST_HAS_RANGE_OFFSET))
        ResourceRequest.ByteRange(
          request.get(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_RANGE_START_OFFSET),
          request.get(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_RANGE_END_OFFSET),
        )
      else null,
      if (request.get(ValueLayout.JAVA_BOOLEAN, RESOURCE_REQUEST_HAS_PRIOR_MODIFIED_OFFSET))
        request.get(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_PRIOR_MODIFIED_OFFSET)
      else null,
      if (request.get(ValueLayout.JAVA_BOOLEAN, RESOURCE_REQUEST_HAS_PRIOR_EXPIRES_OFFSET))
        request.get(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_PRIOR_EXPIRES_OFFSET)
      else null,
      optionalCString(request.get(ValueLayout.ADDRESS, RESOURCE_REQUEST_PRIOR_ETAG_OFFSET)),
      copyBytes(
        request.get(ValueLayout.ADDRESS, RESOURCE_REQUEST_PRIOR_DATA_OFFSET),
        request.get(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_PRIOR_DATA_SIZE_OFFSET),
      ),
    )

  private fun resourceResponse(response: ResourceResponse, arena: Arena): MemorySegment {
    val segment = arena.allocate(RESOURCE_RESPONSE_SIZE)
    val bytes = response.bytes
    if (!response.errorReason.isKnown) {
      throw Status.invalidArgument(
        "Unknown resource error reason cannot be used as input: ${response.errorReason.nativeValue}"
      )
    }
    segment.set(ValueLayout.JAVA_INT, RESOURCE_RESPONSE_SIZE_OFFSET, RESOURCE_RESPONSE_SIZE.toInt())
    segment.set(ValueLayout.JAVA_INT, RESOURCE_RESPONSE_STATUS_OFFSET, response.status.nativeValue)
    segment.set(
      ValueLayout.JAVA_INT,
      RESOURCE_RESPONSE_ERROR_REASON_OFFSET,
      response.errorReason.nativeValue,
    )
    if (bytes.isNotEmpty()) {
      segment.set(ValueLayout.ADDRESS, RESOURCE_RESPONSE_BYTES_OFFSET, nativeBytes(arena, bytes))
      segment.set(ValueLayout.JAVA_LONG, RESOURCE_RESPONSE_BYTE_COUNT_OFFSET, bytes.size.toLong())
    }
    segment.set(
      ValueLayout.ADDRESS,
      RESOURCE_RESPONSE_ERROR_MESSAGE_OFFSET,
      resourceResponseCString(arena, response.errorMessage, "error message"),
    )
    segment.set(
      ValueLayout.JAVA_BOOLEAN,
      RESOURCE_RESPONSE_MUST_REVALIDATE_OFFSET,
      response.mustRevalidate,
    )
    response.modifiedUnixMs?.let {
      segment.set(ValueLayout.JAVA_BOOLEAN, RESOURCE_RESPONSE_HAS_MODIFIED_OFFSET, true)
      segment.set(ValueLayout.JAVA_LONG, RESOURCE_RESPONSE_MODIFIED_OFFSET, it)
    }
    response.expiresUnixMs?.let {
      segment.set(ValueLayout.JAVA_BOOLEAN, RESOURCE_RESPONSE_HAS_EXPIRES_OFFSET, true)
      segment.set(ValueLayout.JAVA_LONG, RESOURCE_RESPONSE_EXPIRES_OFFSET, it)
    }
    segment.set(
      ValueLayout.ADDRESS,
      RESOURCE_RESPONSE_ETAG_OFFSET,
      resourceResponseCString(arena, response.etag, "ETag"),
    )
    response.retryAfterUnixMs?.let {
      segment.set(ValueLayout.JAVA_BOOLEAN, RESOURCE_RESPONSE_HAS_RETRY_AFTER_OFFSET, true)
      segment.set(ValueLayout.JAVA_LONG, RESOURCE_RESPONSE_RETRY_AFTER_OFFSET, it)
    }
    return segment
  }

  private fun optionalCString(address: MemorySegment): String? =
    if (address == MemorySegment.NULL) null else copyCString(address)

  private fun resourceResponseCString(
    arena: Arena,
    value: String?,
    description: String,
  ): MemorySegment {
    value ?: return MemorySegment.NULL
    if ('\u0000' in value) {
      throw Status.invalidArgument("$description contains embedded NUL")
    }
    return cString(arena, value)
  }

  private fun copyCString(address: MemorySegment): String {
    if (address == MemorySegment.NULL) {
      return ""
    }
    var length = 0L
    while (address.reinterpret(length + 1).get(ValueLayout.JAVA_BYTE, length) != 0.toByte()) {
      length++
    }
    return copyString(address, length)
  }

  private fun mapOptions(options: MapOptions, arena: Arena): MemorySegment {
    val segment = MapLibreNativeC.mln_map_options_default(arena)
    options.width?.let {
      Status.requireArgument(it >= 0) { "width must be non-negative" }
      mln_map_options.width(segment, it)
    }
    options.height?.let {
      Status.requireArgument(it >= 0) { "height must be non-negative" }
      mln_map_options.height(segment, it)
    }
    options.scaleFactor?.let { mln_map_options.scale_factor(segment, it) }
    options.mapMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown map mode cannot be used as input: ${it.nativeValue}"
      }
      mln_map_options.map_mode(segment, it.nativeValue)
    }
    return segment
  }

  private fun offlineRegionStatus(status: MemorySegment): OfflineRegionStatus =
    OfflineRegionStatus(
      OfflineRegionDownloadState.fromNative(
        status.get(ValueLayout.JAVA_INT, OFFLINE_REGION_STATUS_DOWNLOAD_STATE_OFFSET)
      ),
      status.get(ValueLayout.JAVA_LONG, OFFLINE_REGION_STATUS_COMPLETED_RESOURCE_COUNT_OFFSET),
      status.get(ValueLayout.JAVA_LONG, OFFLINE_REGION_STATUS_COMPLETED_RESOURCE_SIZE_OFFSET),
      status.get(ValueLayout.JAVA_LONG, OFFLINE_REGION_STATUS_COMPLETED_TILE_COUNT_OFFSET),
      status.get(ValueLayout.JAVA_LONG, OFFLINE_REGION_STATUS_REQUIRED_TILE_COUNT_OFFSET),
      status.get(ValueLayout.JAVA_LONG, OFFLINE_REGION_STATUS_COMPLETED_TILE_SIZE_OFFSET),
      status.get(ValueLayout.JAVA_LONG, OFFLINE_REGION_STATUS_REQUIRED_RESOURCE_COUNT_OFFSET),
      status.get(
        ValueLayout.JAVA_BOOLEAN,
        OFFLINE_REGION_STATUS_REQUIRED_RESOURCE_COUNT_IS_PRECISE_OFFSET,
      ),
      status.get(ValueLayout.JAVA_BOOLEAN, OFFLINE_REGION_STATUS_COMPLETE_OFFSET),
    )

  private fun offlineRegionDefinition(value: OfflineRegionDefinition, arena: Arena): MemorySegment {
    val segment = arena.allocate(OFFLINE_REGION_DEFINITION_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      OFFLINE_REGION_DEFINITION_SIZE_OFFSET,
      OFFLINE_REGION_DEFINITION_SIZE.toInt(),
    )
    when (value) {
      is OfflineRegionDefinition.TilePyramid -> {
        segment.set(
          ValueLayout.JAVA_INT,
          OFFLINE_REGION_DEFINITION_TYPE_OFFSET,
          OFFLINE_REGION_DEFINITION_TYPE_TILE_PYRAMID,
        )
        offlineTilePyramidDefinition(
          value,
          segment.asSlice(OFFLINE_REGION_DEFINITION_DATA_OFFSET),
          arena,
        )
      }
      is OfflineRegionDefinition.GeometryRegion -> {
        segment.set(
          ValueLayout.JAVA_INT,
          OFFLINE_REGION_DEFINITION_TYPE_OFFSET,
          OFFLINE_REGION_DEFINITION_TYPE_GEOMETRY,
        )
        offlineGeometryDefinition(
          value,
          segment.asSlice(OFFLINE_REGION_DEFINITION_DATA_OFFSET),
          arena,
        )
      }
      is OfflineRegionDefinition.Unknown ->
        throw Status.invalidArgument("unknown offline region definitions cannot be used as input")
    }
    return segment
  }

  private fun offlineTilePyramidDefinition(
    value: OfflineRegionDefinition.TilePyramid,
    segment: MemorySegment,
    arena: Arena,
  ) {
    segment.set(
      ValueLayout.JAVA_INT,
      OFFLINE_TILE_PYRAMID_DEFINITION_SIZE_OFFSET,
      OFFLINE_TILE_PYRAMID_DEFINITION_SIZE.toInt(),
    )
    segment.set(
      ValueLayout.ADDRESS,
      OFFLINE_TILE_PYRAMID_DEFINITION_STYLE_URL_OFFSET,
      cString(arena, value.styleUrl),
    )
    latLngBounds(value.bounds, segment.asSlice(OFFLINE_TILE_PYRAMID_DEFINITION_BOUNDS_OFFSET))
    segment.set(
      ValueLayout.JAVA_DOUBLE,
      OFFLINE_TILE_PYRAMID_DEFINITION_MIN_ZOOM_OFFSET,
      value.minZoom,
    )
    segment.set(
      ValueLayout.JAVA_DOUBLE,
      OFFLINE_TILE_PYRAMID_DEFINITION_MAX_ZOOM_OFFSET,
      value.maxZoom,
    )
    segment.set(
      ValueLayout.JAVA_FLOAT,
      OFFLINE_TILE_PYRAMID_DEFINITION_PIXEL_RATIO_OFFSET,
      value.pixelRatio,
    )
    segment.set(
      ValueLayout.JAVA_BOOLEAN,
      OFFLINE_TILE_PYRAMID_DEFINITION_INCLUDE_IDEOGRAPHS_OFFSET,
      value.includeIdeographs,
    )
  }

  private fun offlineGeometryDefinition(
    value: OfflineRegionDefinition.GeometryRegion,
    segment: MemorySegment,
    arena: Arena,
  ) {
    segment.set(
      ValueLayout.JAVA_INT,
      OFFLINE_GEOMETRY_DEFINITION_SIZE_OFFSET,
      OFFLINE_GEOMETRY_DEFINITION_SIZE.toInt(),
    )
    segment.set(
      ValueLayout.ADDRESS,
      OFFLINE_GEOMETRY_DEFINITION_STYLE_URL_OFFSET,
      cString(arena, value.styleUrl),
    )
    segment.set(
      ValueLayout.ADDRESS,
      OFFLINE_GEOMETRY_DEFINITION_GEOMETRY_OFFSET,
      geometry(arena, value.geometry, 0),
    )
    segment.set(ValueLayout.JAVA_DOUBLE, OFFLINE_GEOMETRY_DEFINITION_MIN_ZOOM_OFFSET, value.minZoom)
    segment.set(ValueLayout.JAVA_DOUBLE, OFFLINE_GEOMETRY_DEFINITION_MAX_ZOOM_OFFSET, value.maxZoom)
    segment.set(
      ValueLayout.JAVA_FLOAT,
      OFFLINE_GEOMETRY_DEFINITION_PIXEL_RATIO_OFFSET,
      value.pixelRatio,
    )
    segment.set(
      ValueLayout.JAVA_BOOLEAN,
      OFFLINE_GEOMETRY_DEFINITION_INCLUDE_IDEOGRAPHS_OFFSET,
      value.includeIdeographs,
    )
  }

  private fun latLngBounds(bounds: LatLngBounds, segment: MemorySegment) {
    segment.set(
      ValueLayout.JAVA_DOUBLE,
      LAT_LNG_BOUNDS_SOUTHWEST_LATITUDE_OFFSET,
      bounds.southwest.latitude,
    )
    segment.set(
      ValueLayout.JAVA_DOUBLE,
      LAT_LNG_BOUNDS_SOUTHWEST_LONGITUDE_OFFSET,
      bounds.southwest.longitude,
    )
    segment.set(
      ValueLayout.JAVA_DOUBLE,
      LAT_LNG_BOUNDS_NORTHEAST_LATITUDE_OFFSET,
      bounds.northeast.latitude,
    )
    segment.set(
      ValueLayout.JAVA_DOUBLE,
      LAT_LNG_BOUNDS_NORTHEAST_LONGITUDE_OFFSET,
      bounds.northeast.longitude,
    )
  }

  private fun latLngBounds(arena: Arena, bounds: LatLngBounds): MemorySegment {
    val segment = arena.allocate(latLngBoundsLayout)
    latLngBounds(bounds, segment)
    return segment
  }

  private fun latLngBounds(segment: MemorySegment): LatLngBounds =
    LatLngBounds(
      LatLng(
        segment.get(ValueLayout.JAVA_DOUBLE, LAT_LNG_BOUNDS_SOUTHWEST_LATITUDE_OFFSET),
        segment.get(ValueLayout.JAVA_DOUBLE, LAT_LNG_BOUNDS_SOUTHWEST_LONGITUDE_OFFSET),
      ),
      LatLng(
        segment.get(ValueLayout.JAVA_DOUBLE, LAT_LNG_BOUNDS_NORTHEAST_LATITUDE_OFFSET),
        segment.get(ValueLayout.JAVA_DOUBLE, LAT_LNG_BOUNDS_NORTHEAST_LONGITUDE_OFFSET),
      ),
    )

  private fun offlineRegionSnapshot(snapshot: MemorySegment): OfflineRegionInfo =
    try {
      Arena.ofConfined().use { arena ->
        val info = arena.allocate(OFFLINE_REGION_INFO_SIZE)
        info.set(
          ValueLayout.JAVA_INT,
          OFFLINE_REGION_INFO_SIZE_OFFSET,
          OFFLINE_REGION_INFO_SIZE.toInt(),
        )
        Status.check(offlineRegionSnapshotGetFunction().invokeWithArguments(snapshot, info) as Int)
        offlineRegionInfo(info)
      }
    } finally {
      offlineRegionSnapshotDestroyFunction().invokeWithArguments(snapshot)
    }

  private fun offlineRegionList(list: MemorySegment): List<OfflineRegionInfo> =
    try {
      Arena.ofConfined().use { arena ->
        val outCount = arena.allocate(ValueLayout.JAVA_LONG)
        Status.check(offlineRegionListCountFunction().invokeWithArguments(list, outCount) as Int)
        val count = Math.toIntExact(outCount.get(ValueLayout.JAVA_LONG, 0))
        List(count) { index ->
          val info = arena.allocate(OFFLINE_REGION_INFO_SIZE)
          info.set(
            ValueLayout.JAVA_INT,
            OFFLINE_REGION_INFO_SIZE_OFFSET,
            OFFLINE_REGION_INFO_SIZE.toInt(),
          )
          Status.check(
            offlineRegionListGetFunction().invokeWithArguments(list, index.toLong(), info) as Int
          )
          offlineRegionInfo(info)
        }
      }
    } finally {
      offlineRegionListDestroyFunction().invokeWithArguments(list)
    }

  private fun styleIdList(list: MemorySegment): List<String> =
    try {
      Arena.ofConfined().use { arena ->
        val outCount = arena.allocate(ValueLayout.JAVA_LONG)
        Status.check(styleIdListCountFunction().invokeWithArguments(list, outCount) as Int)
        val count = Math.toIntExact(outCount.get(ValueLayout.JAVA_LONG, 0))
        List(count) { index ->
          val outId = arena.allocate(STRING_VIEW_SIZE)
          Status.check(
            styleIdListGetFunction().invokeWithArguments(list, index.toLong(), outId) as Int
          )
          stringView(outId)
        }
      }
    } finally {
      styleIdListDestroyFunction().invokeWithArguments(list)
    }

  private fun copyStyleSourceAttribution(
    map: MemorySegment,
    sourceId: MemorySegment,
    attributionSize: Long,
    arena: Arena,
  ): String? {
    if (attributionSize == 0L) {
      return ""
    }
    val outAttribution = arena.allocate(attributionSize)
    val outAttributionSize = arena.allocate(ValueLayout.JAVA_LONG)
    val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
    Status.check(
      copyStyleSourceAttributionFunction()
        .invokeWithArguments(
          map,
          sourceId,
          outAttribution,
          attributionSize,
          outAttributionSize,
          outFound,
        ) as Int
    )
    if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
      return null
    }
    return copyString(outAttribution, outAttributionSize.get(ValueLayout.JAVA_LONG, 0))
  }

  private fun takeOfflineRegionSnapshot(
    runtime: MemorySegment,
    operationId: Long,
    function: MethodHandle,
    markTaken: () -> Unit,
  ): OfflineRegionInfo =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.ADDRESS)
      outSnapshot.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(function.invokeWithArguments(runtime, operationId, outSnapshot) as Int)
      try {
        offlineRegionSnapshot(outSnapshot.get(ValueLayout.ADDRESS, 0))
      } finally {
        markTaken()
      }
    }

  private fun takeOfflineRegionList(
    runtime: MemorySegment,
    operationId: Long,
    function: MethodHandle,
    markTaken: () -> Unit,
  ): List<OfflineRegionInfo> =
    Arena.ofConfined().use { arena ->
      val outList = arena.allocate(ValueLayout.ADDRESS)
      outList.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL)
      Status.check(function.invokeWithArguments(runtime, operationId, outList) as Int)
      try {
        offlineRegionList(outList.get(ValueLayout.ADDRESS, 0))
      } finally {
        markTaken()
      }
    }

  private fun offlineRegionInfo(info: MemorySegment): OfflineRegionInfo =
    OfflineRegionInfo(
      info.get(ValueLayout.JAVA_LONG, OFFLINE_REGION_INFO_ID_OFFSET),
      offlineRegionDefinition(info.asSlice(OFFLINE_REGION_INFO_DEFINITION_OFFSET)),
      copyBytes(
        info.get(ValueLayout.ADDRESS, OFFLINE_REGION_INFO_METADATA_OFFSET),
        info.get(ValueLayout.JAVA_LONG, OFFLINE_REGION_INFO_METADATA_SIZE_OFFSET),
      ),
    )

  private fun offlineRegionDefinition(segment: MemorySegment): OfflineRegionDefinition =
    when (val type = segment.get(ValueLayout.JAVA_INT, OFFLINE_REGION_DEFINITION_TYPE_OFFSET)) {
      OFFLINE_REGION_DEFINITION_TYPE_TILE_PYRAMID ->
        offlineTilePyramidDefinition(segment.asSlice(OFFLINE_REGION_DEFINITION_DATA_OFFSET))
      OFFLINE_REGION_DEFINITION_TYPE_GEOMETRY ->
        offlineGeometryDefinition(segment.asSlice(OFFLINE_REGION_DEFINITION_DATA_OFFSET))
      else ->
        OfflineRegionDefinition.Unknown(
          type,
          segment.get(ValueLayout.JAVA_INT, OFFLINE_REGION_DEFINITION_SIZE_OFFSET),
        )
    }

  private fun offlineTilePyramidDefinition(
    segment: MemorySegment
  ): OfflineRegionDefinition.TilePyramid =
    OfflineRegionDefinition.TilePyramid(
      copyString(
        segment.get(ValueLayout.ADDRESS, OFFLINE_TILE_PYRAMID_DEFINITION_STYLE_URL_OFFSET),
        cStringLength(
          segment.get(ValueLayout.ADDRESS, OFFLINE_TILE_PYRAMID_DEFINITION_STYLE_URL_OFFSET)
        ),
      ),
      latLngBounds(segment.asSlice(OFFLINE_TILE_PYRAMID_DEFINITION_BOUNDS_OFFSET)),
      segment.get(ValueLayout.JAVA_DOUBLE, OFFLINE_TILE_PYRAMID_DEFINITION_MIN_ZOOM_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, OFFLINE_TILE_PYRAMID_DEFINITION_MAX_ZOOM_OFFSET),
      segment.get(ValueLayout.JAVA_FLOAT, OFFLINE_TILE_PYRAMID_DEFINITION_PIXEL_RATIO_OFFSET),
      segment.get(
        ValueLayout.JAVA_BOOLEAN,
        OFFLINE_TILE_PYRAMID_DEFINITION_INCLUDE_IDEOGRAPHS_OFFSET,
      ),
    )

  private fun offlineGeometryDefinition(
    segment: MemorySegment
  ): OfflineRegionDefinition.GeometryRegion =
    OfflineRegionDefinition.GeometryRegion(
      copyString(
        segment.get(ValueLayout.ADDRESS, OFFLINE_GEOMETRY_DEFINITION_STYLE_URL_OFFSET),
        cStringLength(
          segment.get(ValueLayout.ADDRESS, OFFLINE_GEOMETRY_DEFINITION_STYLE_URL_OFFSET)
        ),
      ),
      geometry(
        segment
          .get(ValueLayout.ADDRESS, OFFLINE_GEOMETRY_DEFINITION_GEOMETRY_OFFSET)
          .reinterpret(GEOMETRY_SIZE)
      ),
      segment.get(ValueLayout.JAVA_DOUBLE, OFFLINE_GEOMETRY_DEFINITION_MIN_ZOOM_OFFSET),
      segment.get(ValueLayout.JAVA_DOUBLE, OFFLINE_GEOMETRY_DEFINITION_MAX_ZOOM_OFFSET),
      segment.get(ValueLayout.JAVA_FLOAT, OFFLINE_GEOMETRY_DEFINITION_PIXEL_RATIO_OFFSET),
      segment.get(ValueLayout.JAVA_BOOLEAN, OFFLINE_GEOMETRY_DEFINITION_INCLUDE_IDEOGRAPHS_OFFSET),
    )

  private fun cStringLength(address: MemorySegment): Long {
    if (address == MemorySegment.NULL) {
      return 0
    }
    var length = 0L
    while (address.reinterpret(length + 1).get(ValueLayout.JAVA_BYTE, length) != 0.toByte()) {
      length++
    }
    return length
  }

  private fun runtimeEventPayload(
    payloadType: Int,
    payload: MemorySegment,
    payloadSize: Long,
  ): RuntimeEventPayload =
    when (payloadType) {
      PAYLOAD_NONE -> RuntimeEventPayload.None
      PAYLOAD_RENDER_FRAME ->
        if (hasPayloadSize(payload, payloadSize, RUNTIME_EVENT_RENDER_FRAME_SIZE)) {
          renderFramePayload(payload)
        } else unknownPayload(payloadType, payload, payloadSize)
      PAYLOAD_RENDER_MAP ->
        if (hasPayloadSize(payload, payloadSize, RUNTIME_EVENT_RENDER_MAP_SIZE)) {
          renderMapPayload(payload)
        } else unknownPayload(payloadType, payload, payloadSize)
      PAYLOAD_STYLE_IMAGE_MISSING ->
        if (hasPayloadSize(payload, payloadSize, RUNTIME_EVENT_STYLE_IMAGE_MISSING_SIZE)) {
          styleImageMissingPayload(payload)
        } else unknownPayload(payloadType, payload, payloadSize)
      PAYLOAD_TILE_ACTION ->
        if (hasPayloadSize(payload, payloadSize, RUNTIME_EVENT_TILE_ACTION_SIZE)) {
          tileActionPayload(payload)
        } else unknownPayload(payloadType, payload, payloadSize)
      PAYLOAD_OFFLINE_REGION_STATUS ->
        if (hasPayloadSize(payload, payloadSize, RUNTIME_EVENT_OFFLINE_REGION_STATUS_SIZE)) {
          offlineRegionStatusPayload(payload)
        } else unknownPayload(payloadType, payload, payloadSize)
      PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR ->
        if (
          hasPayloadSize(payload, payloadSize, RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR_SIZE)
        ) {
          offlineRegionResponseErrorPayload(payload)
        } else unknownPayload(payloadType, payload, payloadSize)
      PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT ->
        if (
          hasPayloadSize(payload, payloadSize, RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_SIZE)
        ) {
          offlineRegionTileCountLimitPayload(payload)
        } else unknownPayload(payloadType, payload, payloadSize)
      PAYLOAD_OFFLINE_OPERATION_COMPLETED ->
        if (hasPayloadSize(payload, payloadSize, RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_SIZE)) {
          offlineOperationCompletedPayload(payload)
        } else unknownPayload(payloadType, payload, payloadSize)
      else -> unknownPayload(payloadType, payload, payloadSize)
    }

  private fun hasPayloadSize(
    payload: MemorySegment,
    payloadSize: Long,
    requiredSize: Long,
  ): Boolean = payload != MemorySegment.NULL && payloadSize >= requiredSize

  private fun unknownPayload(
    payloadType: Int,
    payload: MemorySegment,
    payloadSize: Long,
  ): RuntimeEventPayload.Unknown =
    RuntimeEventPayload.Unknown(payloadType, payloadSize, copyBytes(payload, payloadSize))

  private fun renderFramePayload(payload: MemorySegment): RuntimeEventPayload.RenderFrame =
    RuntimeEventPayload.RenderFrame(
      RenderMode.fromNative(
        payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_RENDER_FRAME_MODE_OFFSET)
      ),
      payload.get(ValueLayout.JAVA_BOOLEAN, RUNTIME_EVENT_RENDER_FRAME_NEEDS_REPAINT_OFFSET),
      payload.get(ValueLayout.JAVA_BOOLEAN, RUNTIME_EVENT_RENDER_FRAME_PLACEMENT_CHANGED_OFFSET),
      RenderingStats(
        payload.get(ValueLayout.JAVA_DOUBLE, RUNTIME_EVENT_RENDER_FRAME_ENCODING_TIME_OFFSET),
        payload.get(ValueLayout.JAVA_DOUBLE, RUNTIME_EVENT_RENDER_FRAME_RENDERING_TIME_OFFSET),
        payload.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_RENDER_FRAME_FRAME_COUNT_OFFSET),
        payload.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_RENDER_FRAME_DRAW_CALL_COUNT_OFFSET),
        payload.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_RENDER_FRAME_TOTAL_DRAW_CALL_COUNT_OFFSET),
      ),
    )

  private fun renderMapPayload(payload: MemorySegment): RuntimeEventPayload.RenderMap =
    RuntimeEventPayload.RenderMap(
      RenderMode.fromNative(payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_RENDER_MAP_MODE_OFFSET))
    )

  private fun styleImageMissingPayload(
    payload: MemorySegment
  ): RuntimeEventPayload.StyleImageMissing =
    RuntimeEventPayload.StyleImageMissing(
      copyString(
        payload.get(ValueLayout.ADDRESS, RUNTIME_EVENT_STYLE_IMAGE_MISSING_IMAGE_ID_OFFSET),
        payload.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_STYLE_IMAGE_MISSING_IMAGE_ID_SIZE_OFFSET),
      )
    )

  private fun tileActionPayload(payload: MemorySegment): RuntimeEventPayload.TileAction =
    RuntimeEventPayload.TileAction(
      TileOperation.fromNative(
        payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_TILE_ACTION_OPERATION_OFFSET)
      ),
      TileId(
        Integer.toUnsignedLong(
          payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_TILE_ACTION_OVERSCALED_Z_OFFSET)
        ),
        payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_TILE_ACTION_WRAP_OFFSET),
        Integer.toUnsignedLong(
          payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_TILE_ACTION_CANONICAL_Z_OFFSET)
        ),
        Integer.toUnsignedLong(
          payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_TILE_ACTION_CANONICAL_X_OFFSET)
        ),
        Integer.toUnsignedLong(
          payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_TILE_ACTION_CANONICAL_Y_OFFSET)
        ),
      ),
      copyString(
        payload.get(ValueLayout.ADDRESS, RUNTIME_EVENT_TILE_ACTION_SOURCE_ID_OFFSET),
        payload.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_TILE_ACTION_SOURCE_ID_SIZE_OFFSET),
      ),
    )

  private fun offlineRegionStatusPayload(
    payload: MemorySegment
  ): RuntimeEventPayload.OfflineRegionStatusChanged =
    RuntimeEventPayload.OfflineRegionStatusChanged(
      payload.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_OFFLINE_REGION_STATUS_REGION_ID_OFFSET),
      offlineRegionStatus(payload.asSlice(RUNTIME_EVENT_OFFLINE_REGION_STATUS_STATUS_OFFSET)),
    )

  private fun offlineRegionResponseErrorPayload(
    payload: MemorySegment
  ): RuntimeEventPayload.OfflineRegionResponseError =
    RuntimeEventPayload.OfflineRegionResponseError(
      payload.get(
        ValueLayout.JAVA_LONG,
        RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR_REGION_ID_OFFSET,
      ),
      ResourceErrorReason.fromNative(
        payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR_REASON_OFFSET)
      ),
    )

  private fun offlineRegionTileCountLimitPayload(
    payload: MemorySegment
  ): RuntimeEventPayload.OfflineRegionTileCountLimit =
    RuntimeEventPayload.OfflineRegionTileCountLimit(
      payload.get(
        ValueLayout.JAVA_LONG,
        RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_REGION_ID_OFFSET,
      ),
      payload.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_LIMIT_OFFSET),
    )

  private fun offlineOperationCompletedPayload(
    payload: MemorySegment
  ): RuntimeEventPayload.OfflineOperationCompleted =
    RuntimeEventPayload.OfflineOperationCompleted(
      payload.get(
        ValueLayout.JAVA_LONG,
        RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_OPERATION_ID_OFFSET,
      ),
      OfflineOperationKind.fromNative(
        payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_KIND_OFFSET)
      ),
      OfflineOperationResultKind.fromNative(
        payload.get(
          ValueLayout.JAVA_INT,
          RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_RESULT_KIND_OFFSET,
        )
      ),
      payload.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_STATUS_OFFSET),
      payload.get(ValueLayout.JAVA_BOOLEAN, RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_FOUND_OFFSET),
    )

  private fun downcall(name: String): MethodHandle =
    MapLibreNativeC::class.java.getMethod("${name}\$handle").invoke(null) as MethodHandle

  private fun nativeAccessFailure(cause: Throwable): IllegalStateException =
    IllegalStateException(
      "Java FFM native access is not enabled. Run the JVM with " +
        "--enable-native-access=ALL-UNNAMED for this classpath build.",
      cause,
    )

  private fun missingSymbols(cause: Throwable): UnsatisfiedLinkError {
    val missing =
      UnsatisfiedLinkError("Loaded native library does not expose the Maplibre C ABI symbols.")
    missing.addSuppressed(cause)
    return missing
  }

  private fun deepestCause(error: Throwable): Throwable {
    var current = error
    while (current.cause != null) {
      current = current.cause!!
    }
    return current
  }

  private val latLngLayout = mln_lat_lng.layout()
  private val projectedMetersLayout = mln_projected_meters.layout()
  private val screenPointLayout = mln_screen_point.layout()
  private val edgeInsetsLayout = mln_edge_insets.layout()
  private val latLngBoundsLayout = mln_lat_lng_bounds.layout()
  private val canonicalTileIdLayout = mln_canonical_tile_id.layout()
  private val unitBezierLayout = mln_unit_bezier.layout()
  private val vec3Layout = mln_vec3.layout()
  private val quaternionLayout = mln_quaternion.layout()
  private val stringViewLayout = mln_string_view.layout()

  private val STRING_VIEW_SIZE: Long = mln_string_view.sizeof()
  private val STRING_VIEW_DATA_OFFSET: Long = mln_string_view.`data$offset`()
  private val STRING_VIEW_SIZE_OFFSET: Long = mln_string_view.`size$offset`()

  private val SCREEN_POINT_SIZE: Long = mln_screen_point.sizeof()
  private val SCREEN_BOX_SIZE: Long = mln_screen_box.sizeof()
  private val SCREEN_BOX_MIN_OFFSET: Long = mln_screen_box.`min$offset`()
  private val SCREEN_BOX_MAX_OFFSET: Long = mln_screen_box.`max$offset`()

  private val SCREEN_LINE_STRING_SIZE: Long = mln_screen_line_string.sizeof()
  private val SCREEN_LINE_STRING_POINTS_OFFSET: Long = mln_screen_line_string.`points$offset`()
  private val SCREEN_LINE_STRING_POINT_COUNT_OFFSET: Long =
    mln_screen_line_string.`point_count$offset`()

  private val UNIT_BEZIER_SIZE: Long = mln_unit_bezier.sizeof()
  private val UNIT_BEZIER_X1_OFFSET: Long = mln_unit_bezier.`x1$offset`()
  private val UNIT_BEZIER_Y1_OFFSET: Long = mln_unit_bezier.`y1$offset`()
  private val UNIT_BEZIER_X2_OFFSET: Long = mln_unit_bezier.`x2$offset`()
  private val UNIT_BEZIER_Y2_OFFSET: Long = mln_unit_bezier.`y2$offset`()

  private val VEC3_SIZE: Long = mln_vec3.sizeof()
  private val VEC3_X_OFFSET: Long = mln_vec3.`x$offset`()
  private val VEC3_Y_OFFSET: Long = mln_vec3.`y$offset`()
  private val VEC3_Z_OFFSET: Long = mln_vec3.`z$offset`()

  private val QUATERNION_SIZE: Long = mln_quaternion.sizeof()
  private val QUATERNION_X_OFFSET: Long = mln_quaternion.`x$offset`()
  private val QUATERNION_Y_OFFSET: Long = mln_quaternion.`y$offset`()
  private val QUATERNION_Z_OFFSET: Long = mln_quaternion.`z$offset`()
  private val QUATERNION_W_OFFSET: Long = mln_quaternion.`w$offset`()

  private val LAT_LNG_BOUNDS_SIZE: Long = mln_lat_lng_bounds.sizeof()

  private const val GEOMETRY_EMPTY: Int = 0
  private const val GEOMETRY_POINT: Int = 1
  private const val GEOMETRY_LINE_STRING: Int = 2
  private const val GEOMETRY_POLYGON: Int = 3
  private const val GEOMETRY_MULTI_POINT: Int = 4
  private const val GEOMETRY_MULTI_LINE_STRING: Int = 5
  private const val GEOMETRY_MULTI_POLYGON: Int = 6
  private const val GEOMETRY_COLLECTION: Int = 7

  private val COORDINATE_SPAN_SIZE: Long = mln_coordinate_span.sizeof()
  private val COORDINATE_SPAN_COORDINATES_OFFSET: Long = mln_coordinate_span.`coordinates$offset`()
  private val COORDINATE_SPAN_COUNT_OFFSET: Long = mln_coordinate_span.`coordinate_count$offset`()

  private val POLYGON_GEOMETRY_SIZE: Long = mln_polygon_geometry.sizeof()
  private val POLYGON_GEOMETRY_RINGS_OFFSET: Long = mln_polygon_geometry.`rings$offset`()
  private val POLYGON_GEOMETRY_RING_COUNT_OFFSET: Long = mln_polygon_geometry.`ring_count$offset`()

  private val MULTI_LINE_GEOMETRY_SIZE: Long = mln_multi_line_geometry.sizeof()
  private val MULTI_LINE_GEOMETRY_LINES_OFFSET: Long = mln_multi_line_geometry.`lines$offset`()
  private val MULTI_LINE_GEOMETRY_LINE_COUNT_OFFSET: Long =
    mln_multi_line_geometry.`line_count$offset`()

  private val MULTI_POLYGON_GEOMETRY_SIZE: Long = mln_multi_polygon_geometry.sizeof()
  private val MULTI_POLYGON_GEOMETRY_POLYGONS_OFFSET: Long =
    mln_multi_polygon_geometry.`polygons$offset`()
  private val MULTI_POLYGON_GEOMETRY_POLYGON_COUNT_OFFSET: Long =
    mln_multi_polygon_geometry.`polygon_count$offset`()

  private val GEOMETRY_COLLECTION_SIZE: Long = mln_geometry_collection.sizeof()
  private val GEOMETRY_COLLECTION_GEOMETRIES_OFFSET: Long =
    mln_geometry_collection.`geometries$offset`()
  private val GEOMETRY_COLLECTION_GEOMETRY_COUNT_OFFSET: Long =
    mln_geometry_collection.`geometry_count$offset`()

  private val GEOMETRY_SIZE: Long = mln_geometry.sizeof()
  private val GEOMETRY_SIZE_OFFSET: Long = mln_geometry.`size$offset`()
  private val GEOMETRY_TYPE_OFFSET: Long = mln_geometry.`type$offset`()
  private val GEOMETRY_DATA_OFFSET: Long = mln_geometry.`data$offset`()

  private const val FEATURE_IDENTIFIER_NULL: Int = 0
  private const val FEATURE_IDENTIFIER_UINT: Int = 1
  private const val FEATURE_IDENTIFIER_INT: Int = 2
  private const val FEATURE_IDENTIFIER_DOUBLE: Int = 3
  private const val FEATURE_IDENTIFIER_STRING: Int = 4

  private val FEATURE_SIZE: Long = mln_feature.sizeof()
  private val FEATURE_SIZE_OFFSET: Long = mln_feature.`size$offset`()
  private val FEATURE_GEOMETRY_OFFSET: Long = mln_feature.`geometry$offset`()
  private val FEATURE_PROPERTIES_OFFSET: Long = mln_feature.`properties$offset`()
  private val FEATURE_PROPERTY_COUNT_OFFSET: Long = mln_feature.`property_count$offset`()
  private val FEATURE_IDENTIFIER_TYPE_OFFSET: Long = mln_feature.`identifier_type$offset`()
  private val FEATURE_IDENTIFIER_OFFSET: Long = mln_feature.`identifier$offset`()

  private const val GEOJSON_GEOMETRY: Int = 1
  private const val GEOJSON_FEATURE: Int = 2
  private const val GEOJSON_FEATURE_COLLECTION: Int = 3

  private val GEOJSON_SIZE: Long = mln_geojson.sizeof()
  private val GEOJSON_SIZE_OFFSET: Long = mln_geojson.`size$offset`()
  private val GEOJSON_TYPE_OFFSET: Long = mln_geojson.`type$offset`()
  private val GEOJSON_DATA_OFFSET: Long = mln_geojson.`data$offset`()

  private val CANONICAL_TILE_ID_SIZE: Long = mln_canonical_tile_id.sizeof()
  private val CANONICAL_TILE_ID_Z_OFFSET: Long = mln_canonical_tile_id.`z$offset`()
  private val CANONICAL_TILE_ID_X_OFFSET: Long = mln_canonical_tile_id.`x$offset`()
  private val CANONICAL_TILE_ID_Y_OFFSET: Long = mln_canonical_tile_id.`y$offset`()

  private val JSON_VALUE_SIZE: Long = mln_json_value.sizeof()
  private val JSON_VALUE_SIZE_OFFSET: Long = mln_json_value.`size$offset`()
  private val JSON_VALUE_TYPE_OFFSET: Long = mln_json_value.`type$offset`()
  private val JSON_VALUE_DATA_OFFSET: Long = mln_json_value.`data$offset`()
  private val JSON_MEMBER_SIZE: Long = mln_json_member.sizeof()
  private val JSON_MEMBER_KEY_OFFSET: Long = mln_json_member.`key$offset`()
  private val JSON_MEMBER_VALUE_OFFSET: Long = mln_json_member.`value$offset`()
  private const val JSON_NULL: Int = 0
  private const val JSON_BOOL: Int = 1
  private const val JSON_UINT: Int = 2
  private const val JSON_INT: Int = 3
  private const val JSON_DOUBLE: Int = 4
  private const val JSON_STRING: Int = 5
  private const val JSON_ARRAY: Int = 6
  private const val JSON_OBJECT: Int = 7

  private const val QUERY_GEOMETRY_POINT: Int = 1
  private const val QUERY_GEOMETRY_BOX: Int = 2
  private const val QUERY_GEOMETRY_LINE_STRING: Int = 3

  private val RENDERED_QUERY_GEOMETRY_SIZE: Long = mln_rendered_query_geometry.sizeof()
  private val RENDERED_QUERY_GEOMETRY_SIZE_OFFSET: Long =
    mln_rendered_query_geometry.`size$offset`()
  private val RENDERED_QUERY_GEOMETRY_TYPE_OFFSET: Long =
    mln_rendered_query_geometry.`type$offset`()
  private val RENDERED_QUERY_GEOMETRY_DATA_OFFSET: Long =
    mln_rendered_query_geometry.`data$offset`()

  private const val RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS: Int = 1 shl 0

  private val RENDERED_FEATURE_QUERY_OPTIONS_SIZE: Long =
    mln_rendered_feature_query_options.sizeof()
  private val RENDERED_FEATURE_QUERY_OPTIONS_SIZE_OFFSET: Long =
    mln_rendered_feature_query_options.`size$offset`()
  private val RENDERED_FEATURE_QUERY_OPTIONS_FIELDS_OFFSET: Long =
    mln_rendered_feature_query_options.`fields$offset`()
  private val RENDERED_FEATURE_QUERY_OPTIONS_LAYER_IDS_OFFSET: Long =
    mln_rendered_feature_query_options.`layer_ids$offset`()
  private val RENDERED_FEATURE_QUERY_OPTIONS_LAYER_ID_COUNT_OFFSET: Long =
    mln_rendered_feature_query_options.`layer_id_count$offset`()
  private val RENDERED_FEATURE_QUERY_OPTIONS_FILTER_OFFSET: Long =
    mln_rendered_feature_query_options.`filter$offset`()

  private const val SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS: Int = 1 shl 0

  private val SOURCE_FEATURE_QUERY_OPTIONS_SIZE: Long = mln_source_feature_query_options.sizeof()
  private val SOURCE_FEATURE_QUERY_OPTIONS_SIZE_OFFSET: Long =
    mln_source_feature_query_options.`size$offset`()
  private val SOURCE_FEATURE_QUERY_OPTIONS_FIELDS_OFFSET: Long =
    mln_source_feature_query_options.`fields$offset`()
  private val SOURCE_FEATURE_QUERY_OPTIONS_SOURCE_LAYER_IDS_OFFSET: Long =
    mln_source_feature_query_options.`source_layer_ids$offset`()
  private val SOURCE_FEATURE_QUERY_OPTIONS_SOURCE_LAYER_ID_COUNT_OFFSET: Long =
    mln_source_feature_query_options.`source_layer_id_count$offset`()
  private val SOURCE_FEATURE_QUERY_OPTIONS_FILTER_OFFSET: Long =
    mln_source_feature_query_options.`filter$offset`()

  private const val QUERIED_FEATURE_SOURCE_ID: Int = 1 shl 0
  private const val QUERIED_FEATURE_SOURCE_LAYER_ID: Int = 1 shl 1
  private const val QUERIED_FEATURE_STATE: Int = 1 shl 2

  private val QUERIED_FEATURE_SIZE: Long = mln_queried_feature.sizeof()
  private val QUERIED_FEATURE_SIZE_OFFSET: Long = mln_queried_feature.`size$offset`()
  private val QUERIED_FEATURE_FIELDS_OFFSET: Long = mln_queried_feature.`fields$offset`()
  private val QUERIED_FEATURE_FEATURE_OFFSET: Long = mln_queried_feature.`feature$offset`()
  private val QUERIED_FEATURE_SOURCE_ID_OFFSET: Long = mln_queried_feature.`source_id$offset`()
  private val QUERIED_FEATURE_SOURCE_LAYER_ID_OFFSET: Long =
    mln_queried_feature.`source_layer_id$offset`()
  private val QUERIED_FEATURE_STATE_OFFSET: Long = mln_queried_feature.`state$offset`()

  private const val FEATURE_EXTENSION_RESULT_VALUE: Int = 1
  private const val FEATURE_EXTENSION_RESULT_FEATURE_COLLECTION: Int = 2

  private val FEATURE_EXTENSION_RESULT_INFO_SIZE: Long = mln_feature_extension_result_info.sizeof()
  private val FEATURE_EXTENSION_RESULT_INFO_SIZE_OFFSET: Long =
    mln_feature_extension_result_info.`size$offset`()
  private val FEATURE_EXTENSION_RESULT_INFO_TYPE_OFFSET: Long =
    mln_feature_extension_result_info.`type$offset`()
  private val FEATURE_EXTENSION_RESULT_INFO_DATA_OFFSET: Long =
    mln_feature_extension_result_info.`data$offset`()

  private val FEATURE_COLLECTION_SIZE: Long = mln_feature_collection.sizeof()
  private val FEATURE_COLLECTION_FEATURES_OFFSET: Long = mln_feature_collection.`features$offset`()
  private val FEATURE_COLLECTION_FEATURE_COUNT_OFFSET: Long =
    mln_feature_collection.`feature_count$offset`()

  private val STYLE_SOURCE_INFO_SIZE: Long = mln_style_source_info.sizeof()
  private val STYLE_SOURCE_INFO_SIZE_OFFSET: Long = mln_style_source_info.`size$offset`()
  private val STYLE_SOURCE_INFO_TYPE_OFFSET: Long = mln_style_source_info.`type$offset`()
  private val STYLE_SOURCE_INFO_IS_VOLATILE_OFFSET: Long =
    mln_style_source_info.`is_volatile$offset`()
  private val STYLE_SOURCE_INFO_HAS_ATTRIBUTION_OFFSET: Long =
    mln_style_source_info.`has_attribution$offset`()
  private val STYLE_SOURCE_INFO_ATTRIBUTION_SIZE_OFFSET: Long =
    mln_style_source_info.`attribution_size$offset`()

  private const val IMAGE_SOURCE_COORDINATE_COUNT: Int = 4

  private val EDGE_INSETS_SIZE: Long = mln_edge_insets.sizeof()
  private val EDGE_INSETS_TOP_OFFSET: Long = mln_edge_insets.`top$offset`()
  private val EDGE_INSETS_LEFT_OFFSET: Long = mln_edge_insets.`left$offset`()
  private val EDGE_INSETS_BOTTOM_OFFSET: Long = mln_edge_insets.`bottom$offset`()
  private val EDGE_INSETS_RIGHT_OFFSET: Long = mln_edge_insets.`right$offset`()

  private const val TILE_SOURCE_OPTION_MIN_ZOOM: Int = 1 shl 0
  private const val TILE_SOURCE_OPTION_MAX_ZOOM: Int = 1 shl 1
  private const val TILE_SOURCE_OPTION_ATTRIBUTION: Int = 1 shl 2
  private const val TILE_SOURCE_OPTION_SCHEME: Int = 1 shl 3
  private const val TILE_SOURCE_OPTION_BOUNDS: Int = 1 shl 4
  private const val TILE_SOURCE_OPTION_TILE_SIZE: Int = 1 shl 5
  private const val TILE_SOURCE_OPTION_VECTOR_ENCODING: Int = 1 shl 6
  private const val TILE_SOURCE_OPTION_RASTER_ENCODING: Int = 1 shl 7

  private val TILE_SOURCE_OPTIONS_SIZE: Long = mln_style_tile_source_options.sizeof()
  private val TILE_SOURCE_OPTIONS_SIZE_OFFSET: Long = mln_style_tile_source_options.`size$offset`()
  private val TILE_SOURCE_OPTIONS_FIELDS_OFFSET: Long =
    mln_style_tile_source_options.`fields$offset`()
  private val TILE_SOURCE_OPTIONS_MIN_ZOOM_OFFSET: Long =
    mln_style_tile_source_options.`min_zoom$offset`()
  private val TILE_SOURCE_OPTIONS_MAX_ZOOM_OFFSET: Long =
    mln_style_tile_source_options.`max_zoom$offset`()
  private val TILE_SOURCE_OPTIONS_ATTRIBUTION_OFFSET: Long =
    mln_style_tile_source_options.`attribution$offset`()
  private val TILE_SOURCE_OPTIONS_SCHEME_OFFSET: Long =
    mln_style_tile_source_options.`scheme$offset`()
  private val TILE_SOURCE_OPTIONS_BOUNDS_OFFSET: Long =
    mln_style_tile_source_options.`bounds$offset`()
  private val TILE_SOURCE_OPTIONS_TILE_SIZE_OFFSET: Long =
    mln_style_tile_source_options.`tile_size$offset`()
  private val TILE_SOURCE_OPTIONS_VECTOR_ENCODING_OFFSET: Long =
    mln_style_tile_source_options.`vector_encoding$offset`()
  private val TILE_SOURCE_OPTIONS_RASTER_ENCODING_OFFSET: Long =
    mln_style_tile_source_options.`raster_encoding$offset`()

  private const val CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM: Int = 1 shl 0
  private const val CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM: Int = 1 shl 1
  private const val CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE: Int = 1 shl 2
  private const val CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE: Int = 1 shl 3
  private const val CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER: Int = 1 shl 4
  private const val CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP: Int = 1 shl 5
  private const val CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP: Int = 1 shl 6

  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZE: Long =
    mln_custom_geometry_source_options.sizeof()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZE_OFFSET: Long =
    mln_custom_geometry_source_options.`size$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_FIELDS_OFFSET: Long =
    mln_custom_geometry_source_options.`fields$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE_OFFSET: Long =
    mln_custom_geometry_source_options.`fetch_tile$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_CANCEL_TILE_OFFSET: Long =
    mln_custom_geometry_source_options.`cancel_tile$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_USER_DATA_OFFSET: Long =
    mln_custom_geometry_source_options.`user_data$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_MIN_ZOOM_OFFSET: Long =
    mln_custom_geometry_source_options.`min_zoom$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_MAX_ZOOM_OFFSET: Long =
    mln_custom_geometry_source_options.`max_zoom$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_TOLERANCE_OFFSET: Long =
    mln_custom_geometry_source_options.`tolerance$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_TILE_SIZE_OFFSET: Long =
    mln_custom_geometry_source_options.`tile_size$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_BUFFER_OFFSET: Long =
    mln_custom_geometry_source_options.`buffer$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_CLIP_OFFSET: Long =
    mln_custom_geometry_source_options.`clip$offset`()
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_WRAP_OFFSET: Long =
    mln_custom_geometry_source_options.`wrap$offset`()

  private const val FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID: Int = 1 shl 0
  private const val FEATURE_STATE_SELECTOR_FEATURE_ID: Int = 1 shl 1
  private const val FEATURE_STATE_SELECTOR_STATE_KEY: Int = 1 shl 2

  private val FEATURE_STATE_SELECTOR_SIZE: Long = mln_feature_state_selector.sizeof()
  private val FEATURE_STATE_SELECTOR_SIZE_OFFSET: Long = mln_feature_state_selector.`size$offset`()
  private val FEATURE_STATE_SELECTOR_FIELDS_OFFSET: Long =
    mln_feature_state_selector.`fields$offset`()
  private val FEATURE_STATE_SELECTOR_SOURCE_ID_OFFSET: Long =
    mln_feature_state_selector.`source_id$offset`()
  private val FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID_OFFSET: Long =
    mln_feature_state_selector.`source_layer_id$offset`()
  private val FEATURE_STATE_SELECTOR_FEATURE_ID_OFFSET: Long =
    mln_feature_state_selector.`feature_id$offset`()
  private val FEATURE_STATE_SELECTOR_STATE_KEY_OFFSET: Long =
    mln_feature_state_selector.`state_key$offset`()

  private const val STYLE_IMAGE_OPTION_PIXEL_RATIO: Int = 1 shl 0
  private const val STYLE_IMAGE_OPTION_SDF: Int = 1 shl 1

  private const val DEFAULT_PIXEL_RATIO: Float = 1.0f

  private val STYLE_IMAGE_OPTIONS_SIZE: Long = mln_style_image_options.sizeof()
  private val STYLE_IMAGE_OPTIONS_SIZE_OFFSET: Long = mln_style_image_options.`size$offset`()
  private val STYLE_IMAGE_OPTIONS_FIELDS_OFFSET: Long = mln_style_image_options.`fields$offset`()
  private val STYLE_IMAGE_OPTIONS_PIXEL_RATIO_OFFSET: Long =
    mln_style_image_options.`pixel_ratio$offset`()
  private val STYLE_IMAGE_OPTIONS_SDF_OFFSET: Long = mln_style_image_options.`sdf$offset`()

  private val STYLE_IMAGE_INFO_SIZE: Long = mln_style_image_info.sizeof()
  private val STYLE_IMAGE_INFO_SIZE_OFFSET: Long = mln_style_image_info.`size$offset`()
  private val STYLE_IMAGE_INFO_WIDTH_OFFSET: Long = mln_style_image_info.`width$offset`()
  private val STYLE_IMAGE_INFO_HEIGHT_OFFSET: Long = mln_style_image_info.`height$offset`()
  private val STYLE_IMAGE_INFO_STRIDE_OFFSET: Long = mln_style_image_info.`stride$offset`()
  private val STYLE_IMAGE_INFO_BYTE_LENGTH_OFFSET: Long =
    mln_style_image_info.`byte_length$offset`()
  private val STYLE_IMAGE_INFO_PIXEL_RATIO_OFFSET: Long =
    mln_style_image_info.`pixel_ratio$offset`()
  private val STYLE_IMAGE_INFO_SDF_OFFSET: Long = mln_style_image_info.`sdf$offset`()

  private val RUNTIME_EVENT_SIZE: Long = mln_runtime_event.sizeof()
  private val RUNTIME_EVENT_SIZE_OFFSET: Long = mln_runtime_event.`size$offset`()
  private val RUNTIME_EVENT_TYPE_OFFSET: Long = mln_runtime_event.`type$offset`()
  private val RUNTIME_EVENT_SOURCE_TYPE_OFFSET: Long = mln_runtime_event.`source_type$offset`()
  private val RUNTIME_EVENT_SOURCE_OFFSET: Long = mln_runtime_event.`source$offset`()
  private val RUNTIME_EVENT_CODE_OFFSET: Long = mln_runtime_event.`code$offset`()
  private val RUNTIME_EVENT_PAYLOAD_TYPE_OFFSET: Long = mln_runtime_event.`payload_type$offset`()
  private val RUNTIME_EVENT_PAYLOAD_OFFSET: Long = mln_runtime_event.`payload$offset`()
  private val RUNTIME_EVENT_PAYLOAD_SIZE_OFFSET: Long = mln_runtime_event.`payload_size$offset`()
  private val RUNTIME_EVENT_MESSAGE_OFFSET: Long = mln_runtime_event.`message$offset`()
  private val RUNTIME_EVENT_MESSAGE_SIZE_OFFSET: Long = mln_runtime_event.`message_size$offset`()

  private val RESOURCE_REQUEST_URL_OFFSET: Long = mln_resource_request.`url$offset`()
  private val RESOURCE_REQUEST_KIND_OFFSET: Long = mln_resource_request.`kind$offset`()
  private val RESOURCE_REQUEST_LOADING_METHOD_OFFSET: Long =
    mln_resource_request.`loading_method$offset`()
  private val RESOURCE_REQUEST_PRIORITY_OFFSET: Long = mln_resource_request.`priority$offset`()
  private val RESOURCE_REQUEST_USAGE_OFFSET: Long = mln_resource_request.`usage$offset`()
  private val RESOURCE_REQUEST_STORAGE_POLICY_OFFSET: Long =
    mln_resource_request.`storage_policy$offset`()
  private val RESOURCE_REQUEST_HAS_RANGE_OFFSET: Long = mln_resource_request.`has_range$offset`()
  private val RESOURCE_REQUEST_RANGE_START_OFFSET: Long =
    mln_resource_request.`range_start$offset`()
  private val RESOURCE_REQUEST_RANGE_END_OFFSET: Long = mln_resource_request.`range_end$offset`()
  private val RESOURCE_REQUEST_HAS_PRIOR_MODIFIED_OFFSET: Long =
    mln_resource_request.`has_prior_modified$offset`()
  private val RESOURCE_REQUEST_PRIOR_MODIFIED_OFFSET: Long =
    mln_resource_request.`prior_modified_unix_ms$offset`()
  private val RESOURCE_REQUEST_HAS_PRIOR_EXPIRES_OFFSET: Long =
    mln_resource_request.`has_prior_expires$offset`()
  private val RESOURCE_REQUEST_PRIOR_EXPIRES_OFFSET: Long =
    mln_resource_request.`prior_expires_unix_ms$offset`()
  private val RESOURCE_REQUEST_PRIOR_ETAG_OFFSET: Long = mln_resource_request.`prior_etag$offset`()
  private val RESOURCE_REQUEST_PRIOR_DATA_OFFSET: Long = mln_resource_request.`prior_data$offset`()
  private val RESOURCE_REQUEST_PRIOR_DATA_SIZE_OFFSET: Long =
    mln_resource_request.`prior_data_size$offset`()

  private val RESOURCE_RESPONSE_SIZE: Long = mln_resource_response.sizeof()
  private val RESOURCE_RESPONSE_SIZE_OFFSET: Long = mln_resource_response.`size$offset`()
  private val RESOURCE_RESPONSE_STATUS_OFFSET: Long = mln_resource_response.`status$offset`()
  private val RESOURCE_RESPONSE_ERROR_REASON_OFFSET: Long =
    mln_resource_response.`error_reason$offset`()
  private val RESOURCE_RESPONSE_BYTES_OFFSET: Long = mln_resource_response.`bytes$offset`()
  private val RESOURCE_RESPONSE_BYTE_COUNT_OFFSET: Long =
    mln_resource_response.`byte_count$offset`()
  private val RESOURCE_RESPONSE_ERROR_MESSAGE_OFFSET: Long =
    mln_resource_response.`error_message$offset`()
  private val RESOURCE_RESPONSE_MUST_REVALIDATE_OFFSET: Long =
    mln_resource_response.`must_revalidate$offset`()
  private val RESOURCE_RESPONSE_HAS_MODIFIED_OFFSET: Long =
    mln_resource_response.`has_modified$offset`()
  private val RESOURCE_RESPONSE_MODIFIED_OFFSET: Long =
    mln_resource_response.`modified_unix_ms$offset`()
  private val RESOURCE_RESPONSE_HAS_EXPIRES_OFFSET: Long =
    mln_resource_response.`has_expires$offset`()
  private val RESOURCE_RESPONSE_EXPIRES_OFFSET: Long =
    mln_resource_response.`expires_unix_ms$offset`()
  private val RESOURCE_RESPONSE_ETAG_OFFSET: Long = mln_resource_response.`etag$offset`()
  private val RESOURCE_RESPONSE_HAS_RETRY_AFTER_OFFSET: Long =
    mln_resource_response.`has_retry_after$offset`()
  private val RESOURCE_RESPONSE_RETRY_AFTER_OFFSET: Long =
    mln_resource_response.`retry_after_unix_ms$offset`()

  private const val PAYLOAD_NONE: Int = 0
  private const val PAYLOAD_RENDER_FRAME: Int = 1
  private const val PAYLOAD_RENDER_MAP: Int = 2
  private const val PAYLOAD_STYLE_IMAGE_MISSING: Int = 3
  private const val PAYLOAD_TILE_ACTION: Int = 4
  private const val PAYLOAD_OFFLINE_REGION_STATUS: Int = 5
  private const val PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR: Int = 6
  private const val PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT: Int = 7
  private const val PAYLOAD_OFFLINE_OPERATION_COMPLETED: Int = 8

  private val OFFLINE_REGION_STATUS_SIZE: Long = mln_offline_region_status.sizeof()
  private val OFFLINE_REGION_STATUS_SIZE_OFFSET: Long = mln_offline_region_status.`size$offset`()
  private val OFFLINE_REGION_STATUS_DOWNLOAD_STATE_OFFSET: Long =
    mln_offline_region_status.`download_state$offset`()
  private val OFFLINE_REGION_STATUS_COMPLETED_RESOURCE_COUNT_OFFSET: Long =
    mln_offline_region_status.`completed_resource_count$offset`()
  private val OFFLINE_REGION_STATUS_COMPLETED_RESOURCE_SIZE_OFFSET: Long =
    mln_offline_region_status.`completed_resource_size$offset`()
  private val OFFLINE_REGION_STATUS_COMPLETED_TILE_COUNT_OFFSET: Long =
    mln_offline_region_status.`completed_tile_count$offset`()
  private val OFFLINE_REGION_STATUS_REQUIRED_TILE_COUNT_OFFSET: Long =
    mln_offline_region_status.`required_tile_count$offset`()
  private val OFFLINE_REGION_STATUS_COMPLETED_TILE_SIZE_OFFSET: Long =
    mln_offline_region_status.`completed_tile_size$offset`()
  private val OFFLINE_REGION_STATUS_REQUIRED_RESOURCE_COUNT_OFFSET: Long =
    mln_offline_region_status.`required_resource_count$offset`()
  private val OFFLINE_REGION_STATUS_REQUIRED_RESOURCE_COUNT_IS_PRECISE_OFFSET: Long =
    mln_offline_region_status.`required_resource_count_is_precise$offset`()
  private val OFFLINE_REGION_STATUS_COMPLETE_OFFSET: Long =
    mln_offline_region_status.`complete$offset`()

  private val RUNTIME_EVENT_RENDER_FRAME_SIZE: Long = mln_runtime_event_render_frame.sizeof()
  private val RUNTIME_EVENT_RENDER_FRAME_MODE_OFFSET: Long =
    mln_runtime_event_render_frame.`mode$offset`()
  private val RUNTIME_EVENT_RENDER_FRAME_NEEDS_REPAINT_OFFSET: Long =
    mln_runtime_event_render_frame.`needs_repaint$offset`()
  private val RUNTIME_EVENT_RENDER_FRAME_PLACEMENT_CHANGED_OFFSET: Long =
    mln_runtime_event_render_frame.`placement_changed$offset`()
  private val RUNTIME_EVENT_RENDER_FRAME_ENCODING_TIME_OFFSET: Long =
    mln_runtime_event_render_frame.`stats$offset`() + mln_rendering_stats.`encoding_time$offset`()
  private val RUNTIME_EVENT_RENDER_FRAME_RENDERING_TIME_OFFSET: Long =
    mln_runtime_event_render_frame.`stats$offset`() + mln_rendering_stats.`rendering_time$offset`()
  private val RUNTIME_EVENT_RENDER_FRAME_FRAME_COUNT_OFFSET: Long =
    mln_runtime_event_render_frame.`stats$offset`() + mln_rendering_stats.`frame_count$offset`()
  private val RUNTIME_EVENT_RENDER_FRAME_DRAW_CALL_COUNT_OFFSET: Long =
    mln_runtime_event_render_frame.`stats$offset`() + mln_rendering_stats.`draw_call_count$offset`()
  private val RUNTIME_EVENT_RENDER_FRAME_TOTAL_DRAW_CALL_COUNT_OFFSET: Long =
    mln_runtime_event_render_frame.`stats$offset`() +
      mln_rendering_stats.`total_draw_call_count$offset`()

  private val RUNTIME_EVENT_RENDER_MAP_SIZE: Long = mln_runtime_event_render_map.sizeof()
  private val RUNTIME_EVENT_RENDER_MAP_MODE_OFFSET: Long =
    mln_runtime_event_render_map.`mode$offset`()

  private val RUNTIME_EVENT_STYLE_IMAGE_MISSING_SIZE: Long =
    mln_runtime_event_style_image_missing.sizeof()
  private val RUNTIME_EVENT_STYLE_IMAGE_MISSING_IMAGE_ID_OFFSET: Long =
    mln_runtime_event_style_image_missing.`image_id$offset`()
  private val RUNTIME_EVENT_STYLE_IMAGE_MISSING_IMAGE_ID_SIZE_OFFSET: Long =
    mln_runtime_event_style_image_missing.`image_id_size$offset`()

  private val RUNTIME_EVENT_TILE_ACTION_SIZE: Long = mln_runtime_event_tile_action.sizeof()
  private val RUNTIME_EVENT_TILE_ACTION_OPERATION_OFFSET: Long =
    mln_runtime_event_tile_action.`operation$offset`()
  private val RUNTIME_EVENT_TILE_ACTION_OVERSCALED_Z_OFFSET: Long =
    mln_runtime_event_tile_action.`tile_id$offset`() + mln_tile_id.`overscaled_z$offset`()
  private val RUNTIME_EVENT_TILE_ACTION_WRAP_OFFSET: Long =
    mln_runtime_event_tile_action.`tile_id$offset`() + mln_tile_id.`wrap$offset`()
  private val RUNTIME_EVENT_TILE_ACTION_CANONICAL_Z_OFFSET: Long =
    mln_runtime_event_tile_action.`tile_id$offset`() + mln_tile_id.`canonical_z$offset`()
  private val RUNTIME_EVENT_TILE_ACTION_CANONICAL_X_OFFSET: Long =
    mln_runtime_event_tile_action.`tile_id$offset`() + mln_tile_id.`canonical_x$offset`()
  private val RUNTIME_EVENT_TILE_ACTION_CANONICAL_Y_OFFSET: Long =
    mln_runtime_event_tile_action.`tile_id$offset`() + mln_tile_id.`canonical_y$offset`()
  private val RUNTIME_EVENT_TILE_ACTION_SOURCE_ID_OFFSET: Long =
    mln_runtime_event_tile_action.`source_id$offset`()
  private val RUNTIME_EVENT_TILE_ACTION_SOURCE_ID_SIZE_OFFSET: Long =
    mln_runtime_event_tile_action.`source_id_size$offset`()

  private val RUNTIME_EVENT_OFFLINE_REGION_STATUS_SIZE: Long =
    mln_runtime_event_offline_region_status.sizeof()
  private val RUNTIME_EVENT_OFFLINE_REGION_STATUS_REGION_ID_OFFSET: Long =
    mln_runtime_event_offline_region_status.`region_id$offset`()
  private val RUNTIME_EVENT_OFFLINE_REGION_STATUS_STATUS_OFFSET: Long =
    mln_runtime_event_offline_region_status.`status$offset`()

  private val RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR_SIZE: Long =
    mln_runtime_event_offline_region_response_error.sizeof()
  private val RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR_REGION_ID_OFFSET: Long =
    mln_runtime_event_offline_region_response_error.`region_id$offset`()
  private val RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR_REASON_OFFSET: Long =
    mln_runtime_event_offline_region_response_error.`reason$offset`()

  private val RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_SIZE: Long =
    mln_runtime_event_offline_region_tile_count_limit.sizeof()
  private val RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_REGION_ID_OFFSET: Long =
    mln_runtime_event_offline_region_tile_count_limit.`region_id$offset`()
  private val RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_LIMIT_OFFSET: Long =
    mln_runtime_event_offline_region_tile_count_limit.`limit$offset`()

  private val RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_SIZE: Long =
    mln_runtime_event_offline_operation_completed.sizeof()
  private val RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_OPERATION_ID_OFFSET: Long =
    mln_runtime_event_offline_operation_completed.`operation_id$offset`()
  private val RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_KIND_OFFSET: Long =
    mln_runtime_event_offline_operation_completed.`operation_kind$offset`()
  private val RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_RESULT_KIND_OFFSET: Long =
    mln_runtime_event_offline_operation_completed.`result_kind$offset`()
  private val RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_STATUS_OFFSET: Long =
    mln_runtime_event_offline_operation_completed.`result_status$offset`()
  private val RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED_FOUND_OFFSET: Long =
    mln_runtime_event_offline_operation_completed.`found$offset`()

  private const val OFFLINE_REGION_DEFINITION_TYPE_TILE_PYRAMID: Int = 1
  private const val OFFLINE_REGION_DEFINITION_TYPE_GEOMETRY: Int = 2

  private val LAT_LNG_BOUNDS_SOUTHWEST_LATITUDE_OFFSET: Long =
    mln_lat_lng_bounds.`southwest$offset`() + mln_lat_lng.`latitude$offset`()
  private val LAT_LNG_BOUNDS_SOUTHWEST_LONGITUDE_OFFSET: Long =
    mln_lat_lng_bounds.`southwest$offset`() + mln_lat_lng.`longitude$offset`()
  private val LAT_LNG_BOUNDS_NORTHEAST_LATITUDE_OFFSET: Long =
    mln_lat_lng_bounds.`northeast$offset`() + mln_lat_lng.`latitude$offset`()
  private val LAT_LNG_BOUNDS_NORTHEAST_LONGITUDE_OFFSET: Long =
    mln_lat_lng_bounds.`northeast$offset`() + mln_lat_lng.`longitude$offset`()

  private val OFFLINE_TILE_PYRAMID_DEFINITION_SIZE: Long =
    mln_offline_tile_pyramid_region_definition.sizeof()
  private val OFFLINE_TILE_PYRAMID_DEFINITION_SIZE_OFFSET: Long =
    mln_offline_tile_pyramid_region_definition.`size$offset`()
  private val OFFLINE_TILE_PYRAMID_DEFINITION_STYLE_URL_OFFSET: Long =
    mln_offline_tile_pyramid_region_definition.`style_url$offset`()
  private val OFFLINE_TILE_PYRAMID_DEFINITION_BOUNDS_OFFSET: Long =
    mln_offline_tile_pyramid_region_definition.`bounds$offset`()
  private val OFFLINE_TILE_PYRAMID_DEFINITION_MIN_ZOOM_OFFSET: Long =
    mln_offline_tile_pyramid_region_definition.`min_zoom$offset`()
  private val OFFLINE_TILE_PYRAMID_DEFINITION_MAX_ZOOM_OFFSET: Long =
    mln_offline_tile_pyramid_region_definition.`max_zoom$offset`()
  private val OFFLINE_TILE_PYRAMID_DEFINITION_PIXEL_RATIO_OFFSET: Long =
    mln_offline_tile_pyramid_region_definition.`pixel_ratio$offset`()
  private val OFFLINE_TILE_PYRAMID_DEFINITION_INCLUDE_IDEOGRAPHS_OFFSET: Long =
    mln_offline_tile_pyramid_region_definition.`include_ideographs$offset`()

  private val OFFLINE_GEOMETRY_DEFINITION_SIZE: Long =
    mln_offline_geometry_region_definition.sizeof()
  private val OFFLINE_GEOMETRY_DEFINITION_SIZE_OFFSET: Long =
    mln_offline_geometry_region_definition.`size$offset`()
  private val OFFLINE_GEOMETRY_DEFINITION_STYLE_URL_OFFSET: Long =
    mln_offline_geometry_region_definition.`style_url$offset`()
  private val OFFLINE_GEOMETRY_DEFINITION_GEOMETRY_OFFSET: Long =
    mln_offline_geometry_region_definition.`geometry$offset`()
  private val OFFLINE_GEOMETRY_DEFINITION_MIN_ZOOM_OFFSET: Long =
    mln_offline_geometry_region_definition.`min_zoom$offset`()
  private val OFFLINE_GEOMETRY_DEFINITION_MAX_ZOOM_OFFSET: Long =
    mln_offline_geometry_region_definition.`max_zoom$offset`()
  private val OFFLINE_GEOMETRY_DEFINITION_PIXEL_RATIO_OFFSET: Long =
    mln_offline_geometry_region_definition.`pixel_ratio$offset`()
  private val OFFLINE_GEOMETRY_DEFINITION_INCLUDE_IDEOGRAPHS_OFFSET: Long =
    mln_offline_geometry_region_definition.`include_ideographs$offset`()

  private val OFFLINE_REGION_DEFINITION_SIZE: Long = mln_offline_region_definition.sizeof()
  private val OFFLINE_REGION_DEFINITION_SIZE_OFFSET: Long =
    mln_offline_region_definition.`size$offset`()
  private val OFFLINE_REGION_DEFINITION_TYPE_OFFSET: Long =
    mln_offline_region_definition.`type$offset`()
  private val OFFLINE_REGION_DEFINITION_DATA_OFFSET: Long =
    mln_offline_region_definition.`data$offset`()

  private val OFFLINE_REGION_INFO_SIZE: Long = mln_offline_region_info.sizeof()
  private val OFFLINE_REGION_INFO_SIZE_OFFSET: Long = mln_offline_region_info.`size$offset`()
  private val OFFLINE_REGION_INFO_ID_OFFSET: Long = mln_offline_region_info.`id$offset`()
  private val OFFLINE_REGION_INFO_DEFINITION_OFFSET: Long =
    mln_offline_region_info.`definition$offset`()
  private val OFFLINE_REGION_INFO_METADATA_OFFSET: Long =
    mln_offline_region_info.`metadata$offset`()
  private val OFFLINE_REGION_INFO_METADATA_SIZE_OFFSET: Long =
    mln_offline_region_info.`metadata_size$offset`()

  internal data class NativeRuntimeEvent(
    val type: Int,
    val sourceType: Int,
    val sourceAddress: Long,
    val code: Int,
    val payload: RuntimeEventPayload,
    val message: String,
  )

  internal class OwnedTextureFrameSegment(val segment: MemorySegment, private val arena: Arena) :
    AutoCloseable {
    override fun close() {
      arena.close()
    }
  }
}
