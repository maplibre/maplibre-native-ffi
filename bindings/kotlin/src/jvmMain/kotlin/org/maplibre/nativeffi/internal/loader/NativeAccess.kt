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
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.camera.UnitBezier
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.CanonicalTileId
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
import org.maplibre.nativeffi.internal.c.mln_buffer_view
import org.maplibre.nativeffi.internal.c.mln_camera_fit_options
import org.maplibre.nativeffi.internal.c.mln_camera_options
import org.maplibre.nativeffi.internal.c.mln_canonical_tile_id
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_options
import org.maplibre.nativeffi.internal.c.mln_custom_mvt_vector_source_options
import org.maplibre.nativeffi.internal.c.mln_edge_insets
import org.maplibre.nativeffi.internal.c.mln_egl_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_feature_state_selector
import org.maplibre.nativeffi.internal.c.mln_free_camera_options
import org.maplibre.nativeffi.internal.c.mln_geojson_source_options
import org.maplibre.nativeffi.internal.c.mln_image_content
import org.maplibre.nativeffi.internal.c.mln_image_stretch
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
import org.maplibre.nativeffi.internal.c.mln_runtime_event_batch
import org.maplibre.nativeffi.internal.c.mln_runtime_event_camera_transition_finished
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_operation_completed
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_response_error
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_status
import org.maplibre.nativeffi.internal.c.mln_runtime_event_offline_region_tile_count_limit
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_frame
import org.maplibre.nativeffi.internal.c.mln_runtime_event_render_map
import org.maplibre.nativeffi.internal.c.mln_runtime_event_tile_action
import org.maplibre.nativeffi.internal.c.mln_runtime_options
import org.maplibre.nativeffi.internal.c.mln_screen_box
import org.maplibre.nativeffi.internal.c.mln_screen_line_string
import org.maplibre.nativeffi.internal.c.mln_screen_point
import org.maplibre.nativeffi.internal.c.mln_source_feature_query_options
import org.maplibre.nativeffi.internal.c.mln_style_image_info
import org.maplibre.nativeffi.internal.c.mln_style_image_options
import org.maplibre.nativeffi.internal.c.mln_style_source_info
import org.maplibre.nativeffi.internal.c.mln_style_tile_source_options
import org.maplibre.nativeffi.internal.c.mln_style_transition_options
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
import org.maplibre.nativeffi.internal.lifecycle.NativeGeoJsonSourceData
import org.maplibre.nativeffi.internal.lifecycle.NativeHandle
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.internal.lifecycle.NativeMapProjection
import org.maplibre.nativeffi.internal.lifecycle.NativeOfflineRegionList
import org.maplibre.nativeffi.internal.lifecycle.NativeOfflineRegionSnapshot
import org.maplibre.nativeffi.internal.lifecycle.NativeOwnedBuffer
import org.maplibre.nativeffi.internal.lifecycle.NativeQueriedFeatureList
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.lifecycle.NativeRuntime
import org.maplibre.nativeffi.internal.lifecycle.NativeStyleIdList
import org.maplibre.nativeffi.internal.lifecycle.NativeStyleStringList
import org.maplibre.nativeffi.internal.lifecycle.NativeWakeSource
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.map.ConstrainMode
import org.maplibre.nativeffi.map.DebugOption
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.MapSize
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
import org.maplibre.nativeffi.render.RenderResult
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.RenderUpdate
import org.maplibre.nativeffi.render.TextureImageInfo
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanHandle
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
import org.maplibre.nativeffi.style.CustomMvtVectorSourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.ImageContent
import org.maplibre.nativeffi.style.ImageStretch
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.RasterDemEncoding
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleImageTextFit
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileJson
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding

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
    intFunction("mln_c_version").invokeNative().let { Integer.toUnsignedLong(it as Int) }

  internal fun supportedRenderBackendMask(): Int =
    intFunction("mln_supported_render_backend_mask").invokeNative() as Int

  internal fun supportedOpenGLContextProviderMask(): Int =
    intFunction("mln_opengl_supported_context_provider_mask").invokeNative() as Int

  internal fun renderTargetExtentPhysicalSize(extent: RenderTargetExtent): Pair<Int, Int> =
    Arena.ofConfined().use { arena ->
      val nativeExtent = mln_render_target_extent.allocate(arena)
      fillRenderTargetExtent(nativeExtent, extent)
      val outWidth = arena.allocate(ValueLayout.JAVA_INT)
      val outHeight = arena.allocate(ValueLayout.JAVA_INT)
      Status.check(
        statusOutFunction("mln_render_target_extent_physical_size")
          .invokeNative(nativeExtent, outWidth, outHeight) as Int
      )
      outWidth.get(ValueLayout.JAVA_INT, 0) to outHeight.get(ValueLayout.JAVA_INT, 0)
    }

  internal fun networkStatus(): Int =
    Arena.ofConfined().use { arena ->
      val outStatus = arena.allocate(ValueLayout.JAVA_INT)
      Status.check(statusOutFunction("mln_network_status_get").invokeNative(outStatus) as Int)
      outStatus.get(ValueLayout.JAVA_INT, 0)
    }

  internal fun setNetworkStatus(status: Int) {
    Status.check(statusInFunction("mln_network_status_set").invokeNative(status) as Int)
  }

  internal fun setAsyncLogSeverityMask(mask: Int) {
    Status.check(statusInFunction("mln_log_set_async_severity_mask").invokeNative(mask) as Int)
  }

  internal fun setLogCallback(callback: MemorySegment): Int =
    logSetCallbackFunction().invokeNative(callback, MemorySegment.NULL) as Int

  internal fun clearLogCallback(): Int = intFunction("mln_log_clear_callback").invokeNative() as Int

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
        projectedMetersForLatLngFunction().invokeNative(nativeCoordinate, outMeters) as Int
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
        latLngForProjectedMetersFunction().invokeNative(nativeMeters, outCoordinate) as Int
      )
      LatLng(
        outCoordinate.get(ValueLayout.JAVA_DOUBLE, 0),
        outCoordinate.get(ValueLayout.JAVA_DOUBLE, Double.SIZE_BYTES.toLong()),
      )
    }

  internal fun createRuntime(options: RuntimeOptions): NativeRuntime =
    Arena.ofConfined().use { arena ->
      val nativeOptions = runtimeOptions(options, arena)
      val outRuntime = arena.allocate(ValueLayout.JAVA_LONG)
      outRuntime.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(runtimeCreateFunction().invokeNative(nativeOptions, outRuntime) as Int)
      NativeRuntime(outRuntime.get(ValueLayout.JAVA_LONG, 0)).also { runtime ->
        require(!runtime.isNull) { "mln_runtime_create returned the null handle" }
      }
    }

  internal fun pumpRuntime(runtime: NativeRuntime, timeoutMillis: Long, budgetMillis: Long) {
    Status.check(runtimePumpFunction().invokeNative(runtime, timeoutMillis, budgetMillis) as Int)
  }

  internal fun acquireWakeSource(runtime: NativeRuntime): NativeWakeSource =
    Arena.ofConfined().use { arena ->
      val outSource = arena.allocate(ValueLayout.JAVA_LONG)
      outSource.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(runtimeWakeSourceAcquireFunction().invokeNative(runtime, outSource) as Int)
      NativeWakeSource(outSource.get(ValueLayout.JAVA_LONG, 0)).also { source ->
        require(!source.isNull) { "mln_runtime_wake_source_acquire returned the null handle" }
      }
    }

  internal fun signalWakeSource(source: NativeWakeSource) {
    Status.check(wakeSourceSignalFunction().invokeNative(source) as Int)
  }

  internal fun destroyWakeSource(source: NativeWakeSource) {
    wakeSourceDestroyFunction().invokeNative(source)
  }

  internal fun destroyRuntime(runtime: NativeRuntime): Int =
    runtimeStatusFunction("mln_runtime_destroy").invokeNative(runtime) as Int

  internal fun setResourceProvider(runtime: NativeRuntime, provider: MemorySegment): Int =
    runtimeSetResourceProviderFunction().invokeNative(runtime, provider) as Int

  internal fun clearResourceProvider(runtime: NativeRuntime): Int =
    runtimeClearResourceProviderFunction().invokeNative(runtime) as Int

  internal fun startAmbientCacheOperation(runtime: NativeRuntime, operation: Int): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeAmbientCacheOperationStartFunction().invokeNative(runtime, operation, outOperationId)
          as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startSetMaximumAmbientCacheSize(runtime: NativeRuntime, size: Long): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeSetMaximumAmbientCacheSizeStartFunction().invokeNative(runtime, size, outOperationId)
          as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startCreateOfflineRegion(
    runtime: NativeRuntime,
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeOfflineRegionCreateStartFunction()
          .invokeNative(
            runtime,
            offlineRegionDefinition(definition, arena),
            nativeBytes(arena, metadata),
            metadata.size.toLong(),
            outOperationId,
          ) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startOfflineRegion(runtime: NativeRuntime, regionId: Long): Long =
    startRuntimeLongOperation("mln_runtime_offline_region_get_start", runtime, regionId)

  internal fun startOfflineRegions(runtime: NativeRuntime): Long =
    startRuntimeOperation("mln_runtime_offline_regions_list_start", runtime)

  internal fun startMergeOfflineRegionsDatabase(runtime: NativeRuntime, path: String): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeAddressOperationStartFunction("mln_runtime_offline_regions_merge_database_start")
          .invokeNative(runtime, cString(arena, path), outOperationId) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startUpdateOfflineRegionMetadata(
    runtime: NativeRuntime,
    regionId: Long,
    metadata: ByteArray,
  ): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeLongAddressLongOperationStartFunction(
            "mln_runtime_offline_region_update_metadata_start"
          )
          .invokeNative(
            runtime,
            regionId,
            nativeBytes(arena, metadata),
            metadata.size.toLong(),
            outOperationId,
          ) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startOfflineRegionStatus(runtime: NativeRuntime, regionId: Long): Long =
    startRuntimeLongOperation("mln_runtime_offline_region_get_status_start", runtime, regionId)

  internal fun startSetOfflineRegionObserved(
    runtime: NativeRuntime,
    regionId: Long,
    observed: Boolean,
  ): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeLongBooleanOperationStartFunction("mln_runtime_offline_region_set_observed_start")
          .invokeNative(runtime, regionId, observed, outOperationId) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startSetOfflineRegionDownloadState(
    runtime: NativeRuntime,
    regionId: Long,
    downloadState: Int,
  ): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeLongIntOperationStartFunction("mln_runtime_offline_region_set_download_state_start")
          .invokeNative(runtime, regionId, downloadState, outOperationId) as Int
      )
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun startInvalidateOfflineRegion(runtime: NativeRuntime, regionId: Long): Long =
    startRuntimeLongOperation("mln_runtime_offline_region_invalidate_start", runtime, regionId)

  internal fun startDeleteOfflineRegion(runtime: NativeRuntime, regionId: Long): Long =
    startRuntimeLongOperation("mln_runtime_offline_region_delete_start", runtime, regionId)

  internal fun discardOfflineOperation(runtime: NativeRuntime, operationId: Long): Int =
    runtimeOfflineOperationDiscardFunction().invokeNative(runtime, operationId) as Int

  internal fun setResourceTransform(runtime: NativeRuntime, descriptor: MemorySegment): Int =
    runtimeSetResourceTransformFunction().invokeNative(runtime, descriptor) as Int

  internal fun clearResourceTransform(runtime: NativeRuntime): Int =
    runtimeClearResourceTransformFunction().invokeNative(runtime) as Int

  internal fun setHttpHeaderTransform(runtime: NativeRuntime, descriptor: MemorySegment): Int =
    runtimeSetHttpHeaderTransformFunction().invokeNative(runtime, descriptor) as Int

  internal fun clearHttpHeaderTransform(runtime: NativeRuntime): Int =
    runtimeClearHttpHeaderTransformFunction().invokeNative(runtime) as Int

  internal fun completeResourceRequest(
    handle: NativeResourceRequest,
    response: ResourceResponse,
  ): Int =
    Arena.ofConfined().use { arena ->
      resourceRequestCompleteFunction().invokeNative(handle, resourceResponse(response, arena))
        as Int
    }

  internal fun isResourceRequestCancelled(handle: NativeResourceRequest): Boolean =
    Arena.ofConfined().use { arena ->
      val outCancelled = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(resourceRequestCancelledFunction().invokeNative(handle, outCancelled) as Int)
      outCancelled.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun releaseResourceRequest(handle: NativeResourceRequest) {
    resourceRequestReleaseFunction().invokeNative(handle)
  }

  internal fun createMap(runtime: NativeRuntime, options: MapOptions): NativeMap =
    Arena.ofConfined().use { arena ->
      val outMap = arena.allocate(ValueLayout.JAVA_LONG)
      outMap.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        mapCreateFunction().invokeNative(runtime, mapOptions(options, arena), outMap) as Int
      )
      NativeMap(outMap.get(ValueLayout.JAVA_LONG, 0)).also { map ->
        require(!map.isNull) { "mln_map_create returned the null handle" }
      }
    }

  internal fun destroyMap(map: NativeMap): Int =
    mapStatusFunction("mln_map_destroy").invokeNative(map) as Int

  internal fun setMapStyleUrl(map: NativeMap, url: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_style_url").invokeNative(map, cString(arena, url))
          as Int
      )
    }
  }

  internal fun setMapStyleJson(map: NativeMap, json: ByteArray) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_style_json")
          .invokeNative(map, byteArrayView(arena, json)) as Int
      )
    }
  }

  internal fun setMapFeatureState(
    map: NativeMap,
    selector: FeatureStateSelector,
    value: ByteArray,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoAddressStatusFunction("mln_map_set_feature_state")
          .invokeNative(map, featureStateSelector(arena, selector), byteArrayView(arena, value))
          as Int
      )
    }
  }

  internal fun getMapFeatureState(map: NativeMap, selector: FeatureStateSelector): ByteArray =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.JAVA_LONG)
      outSnapshot.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        mapTwoAddressStatusFunction("mln_map_get_feature_state")
          .invokeNative(map, featureStateSelector(arena, selector), outSnapshot) as Int
      )
      ownedBuffer(NativeOwnedBuffer(outSnapshot.get(ValueLayout.JAVA_LONG, 0)))!!
    }

  internal fun removeMapFeatureState(map: NativeMap, selector: FeatureStateSelector) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_remove_feature_state")
          .invokeNative(map, featureStateSelector(arena, selector)) as Int
      )
    }
  }

  internal fun addStyleSourceJson(map: NativeMap, sourceId: String, sourceJson: ByteArray) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_add_style_source_json")
          .invokeNative(map, stringView(arena, sourceId), byteArrayView(arena, sourceJson)) as Int
      )
    }
  }

  internal fun removeStyleSource(map: NativeMap, sourceId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outRemoved = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_remove_style_source")
          .invokeNative(map, stringView(arena, sourceId), outRemoved) as Int
      )
      outRemoved.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleSourceExists(map: NativeMap, sourceId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outExists = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_style_source_exists")
          .invokeNative(map, stringView(arena, sourceId), outExists) as Int
      )
      outExists.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleSourceType(map: NativeMap, sourceId: String): SourceType? =
    Arena.ofConfined().use { arena ->
      val outType = arena.allocate(ValueLayout.JAVA_INT)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_get_style_source_type")
          .invokeNative(map, stringView(arena, sourceId), outType, outFound) as Int
      )
      if (outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        SourceType.fromNative(outType.get(ValueLayout.JAVA_INT, 0))
      } else null
    }

  internal fun styleSourceInfo(map: NativeMap, sourceId: String): SourceInfo? =
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
          .invokeNative(map, sourceIdView, outInfo, outFound) as Int
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
      val fields = outInfo.get(ValueLayout.JAVA_INT, STYLE_SOURCE_INFO_FIELDS_OFFSET)
      val url =
        if (fields and STYLE_SOURCE_INFO_URL != 0) {
          copyStyleSourceUrl(
            map,
            sourceIdView,
            outInfo.get(ValueLayout.JAVA_LONG, STYLE_SOURCE_INFO_URL_SIZE_OFFSET),
            arena,
          )
        } else null
      val tileJson =
        if (fields and STYLE_SOURCE_INFO_TILEJSON != 0) {
          TileJson(
            styleSourceTileUrls(map, sourceIdView, arena),
            outInfo.get(ValueLayout.JAVA_DOUBLE, STYLE_SOURCE_INFO_MIN_ZOOM_OFFSET),
            outInfo.get(ValueLayout.JAVA_DOUBLE, STYLE_SOURCE_INFO_MAX_ZOOM_OFFSET),
            TileScheme.fromNative(
              outInfo.get(ValueLayout.JAVA_INT, STYLE_SOURCE_INFO_SCHEME_OFFSET)
            ),
            if (fields and STYLE_SOURCE_INFO_BOUNDS != 0)
              latLngBounds(outInfo.asSlice(STYLE_SOURCE_INFO_BOUNDS_OFFSET, LAT_LNG_BOUNDS_SIZE))
            else null,
          )
        } else null
      SourceInfo(
        SourceType.fromNative(outInfo.get(ValueLayout.JAVA_INT, STYLE_SOURCE_INFO_TYPE_OFFSET)),
        outInfo.get(ValueLayout.JAVA_BOOLEAN, STYLE_SOURCE_INFO_IS_VOLATILE_OFFSET),
        attribution,
        url,
        tileJson,
        if (fields and STYLE_SOURCE_INFO_TILE_SIZE != 0)
          Math.toIntExact(
            Integer.toUnsignedLong(
              outInfo.get(ValueLayout.JAVA_INT, STYLE_SOURCE_INFO_TILE_SIZE_OFFSET)
            )
          )
        else null,
        if (fields and STYLE_SOURCE_INFO_VECTOR_ENCODING != 0)
          VectorTileEncoding.fromNative(
            outInfo.get(ValueLayout.JAVA_INT, STYLE_SOURCE_INFO_VECTOR_ENCODING_OFFSET)
          )
        else null,
        if (fields and STYLE_SOURCE_INFO_RASTER_ENCODING != 0)
          RasterDemEncoding.fromNative(
            outInfo.get(ValueLayout.JAVA_INT, STYLE_SOURCE_INFO_RASTER_ENCODING_OFFSET)
          )
        else null,
      )
    }

  internal fun setStyleSourceVolatile(map: NativeMap, sourceId: String, isVolatile: Boolean) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewBooleanStatusFunction("mln_map_set_style_source_volatile")
          .invokeNative(map, stringView(arena, sourceId), isVolatile) as Int
      )
    }
  }

  internal fun styleSourceIds(map: NativeMap): List<String> =
    Arena.ofConfined().use { arena ->
      val outList = arena.allocate(ValueLayout.JAVA_LONG)
      outList.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(mapListStyleSourceIdsFunction().invokeNative(map, outList) as Int)
      styleIdList(NativeStyleIdList(outList.get(ValueLayout.JAVA_LONG, 0)))
    }

  internal fun addGeoJsonSourceUrl(
    map: NativeMap,
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsAddressStatusFunction("mln_map_add_geojson_source_url")
          .invokeNative(
            map,
            stringView(arena, sourceId),
            stringView(arena, url),
            geoJsonSourceOptions(arena, options),
          ) as Int
      )
    }
  }

  internal fun createGeoJsonSourceData(
    data: ByteArray,
    options: GeoJsonSourceOptions?,
  ): NativeGeoJsonSourceData =
    Arena.ofConfined().use { arena ->
      val outData = arena.allocate(ValueLayout.JAVA_LONG)
      outData.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        geoJsonSourceDataCreateFunction()
          .invokeNative(byteArrayView(arena, data), geoJsonSourceOptions(arena, options), outData)
          as Int
      )
      NativeGeoJsonSourceData(outData.get(ValueLayout.JAVA_LONG, 0)).also { handle ->
        require(!handle.isNull) { "mln_geojson_source_data_create returned the null handle" }
      }
    }

  internal fun destroyGeoJsonSourceData(data: NativeGeoJsonSourceData) {
    geoJsonSourceDataDestroyFunction().invokeNative(data)
  }

  internal fun addGeoJsonSourceData(
    map: NativeMap,
    sourceId: String,
    data: NativeGeoJsonSourceData,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewHandleStatusFunction("mln_map_add_geojson_source_data")
          .invokeNative(map, stringView(arena, sourceId), data) as Int
      )
    }
  }

  internal fun setGeoJsonSourceUrl(map: NativeMap, sourceId: String, url: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_set_geojson_source_url")
          .invokeNative(map, stringView(arena, sourceId), stringView(arena, url)) as Int
      )
    }
  }

  internal fun setGeoJsonSourceData(
    map: NativeMap,
    sourceId: String,
    data: NativeGeoJsonSourceData,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewHandleStatusFunction("mln_map_set_geojson_source_data")
          .invokeNative(map, stringView(arena, sourceId), data) as Int
      )
    }
  }

  internal fun setGeoJsonSourceSynchronousTiling(
    map: NativeMap,
    sourceId: String,
    enabled: Boolean,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewBooleanStatusFunction("mln_map_set_geojson_source_synchronous_tiling")
          .invokeNative(map, stringView(arena, sourceId), enabled) as Int
      )
    }
  }

  internal fun addCustomGeometrySource(map: NativeMap, sourceId: String, options: MemorySegment) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_add_custom_geometry_source")
          .invokeNative(map, stringView(arena, sourceId), options) as Int
      )
    }
  }

  internal fun setCustomGeometrySourceTileData(
    map: NativeMap,
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewCanonicalTileIdAddressStatusFunction(
            "mln_map_set_custom_geometry_source_tile_data"
          )
          .invokeNative(
            map,
            stringView(arena, sourceId),
            canonicalTileId(arena, tileId),
            byteArrayView(arena, data),
          ) as Int
      )
    }
  }

  internal fun invalidateCustomGeometrySourceTile(
    map: NativeMap,
    sourceId: String,
    tileId: CanonicalTileId,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewCanonicalTileIdStatusFunction("mln_map_invalidate_custom_geometry_source_tile")
          .invokeNative(map, stringView(arena, sourceId), canonicalTileId(arena, tileId)) as Int
      )
    }
  }

  internal fun addCustomMvtVectorSource(map: NativeMap, sourceId: String, options: MemorySegment) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_add_custom_mvt_vector_source")
          .invokeNative(map, stringView(arena, sourceId), options) as Int
      )
    }
  }

  internal fun setCustomMvtVectorSourceTileData(
    map: NativeMap,
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewCanonicalTileIdAddressStatusFunction(
            "mln_map_set_custom_mvt_vector_source_tile_data"
          )
          .invokeNative(
            map,
            stringView(arena, sourceId),
            canonicalTileId(arena, tileId),
            byteArrayView(arena, data),
          ) as Int
      )
    }
  }

  internal fun setCustomMvtVectorSourceTileError(
    map: NativeMap,
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewCanonicalTileIdAddressStatusFunction(
            "mln_map_set_custom_mvt_vector_source_tile_error"
          )
          .invokeNative(
            map,
            stringView(arena, sourceId),
            canonicalTileId(arena, tileId),
            stringView(arena, message),
          ) as Int
      )
    }
  }

  internal fun invalidateCustomMvtVectorSourceTile(
    map: NativeMap,
    sourceId: String,
    tileId: CanonicalTileId,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewCanonicalTileIdStatusFunction(
            "mln_map_invalidate_custom_mvt_vector_source_tile"
          )
          .invokeNative(map, stringView(arena, sourceId), canonicalTileId(arena, tileId)) as Int
      )
    }
  }

  internal fun invalidateCustomGeometrySourceRegion(
    map: NativeMap,
    sourceId: String,
    bounds: LatLngBounds,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewLatLngBoundsStatusFunction("mln_map_invalidate_custom_geometry_source_region")
          .invokeNative(map, stringView(arena, sourceId), latLngBounds(arena, bounds)) as Int
      )
    }
  }

  internal fun addVectorSourceUrl(
    map: NativeMap,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    addTileSourceUrl("mln_map_add_vector_source_url", map, sourceId, url, options)
  }

  internal fun addVectorSourceTiles(
    map: NativeMap,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    addTileSourceTiles("mln_map_add_vector_source_tiles", map, sourceId, tiles, options)
  }

  internal fun addRasterSourceUrl(
    map: NativeMap,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    addTileSourceUrl("mln_map_add_raster_source_url", map, sourceId, url, options)
  }

  internal fun addRasterSourceTiles(
    map: NativeMap,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    addTileSourceTiles("mln_map_add_raster_source_tiles", map, sourceId, tiles, options)
  }

  internal fun addRasterDemSourceUrl(
    map: NativeMap,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    addTileSourceUrl("mln_map_add_raster_dem_source_url", map, sourceId, url, options)
  }

  internal fun addRasterDemSourceTiles(
    map: NativeMap,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    addTileSourceTiles("mln_map_add_raster_dem_source_tiles", map, sourceId, tiles, options)
  }

  internal fun setStyleImage(
    map: NativeMap,
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_set_style_image")
          .invokeNative(
            map,
            stringView(arena, imageId),
            premultipliedRgba8Image(arena, image),
            styleImageOptions(arena, options),
          ) as Int
      )
    }
  }

  internal fun removeStyleImage(map: NativeMap, imageId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outRemoved = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_remove_style_image")
          .invokeNative(map, stringView(arena, imageId), outRemoved) as Int
      )
      outRemoved.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleImageExists(map: NativeMap, imageId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outExists = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_style_image_exists")
          .invokeNative(map, stringView(arena, imageId), outExists) as Int
      )
      outExists.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleImageInfo(map: NativeMap, imageId: String): StyleImageInfo? =
    Arena.ofConfined().use { arena ->
      val outInfo = styleImageInfoDefault(arena)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_get_style_image_info")
          .invokeNative(map, stringView(arena, imageId), outInfo, outFound) as Int
      )
      if (outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) styleImageInfo(outInfo) else null
    }

  internal fun copyStyleImagePremultipliedRgba8(map: NativeMap, imageId: String): StyleImage? {
    val info = styleImageInfo(map, imageId) ?: return null
    return Arena.ofConfined().use { arena ->
      val outPixels = arena.allocate(info.byteLength)
      val outByteLength = arena.allocate(ValueLayout.JAVA_LONG)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressLongTwoAddressStatusFunction(
            "mln_map_copy_style_image_premultiplied_rgba8"
          )
          .invokeNative(
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
    map: NativeMap,
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ) {
    val coordinateSnapshot = coordinates.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressLongStringViewStatusFunction("mln_map_add_image_source_url")
          .invokeNative(
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
    map: NativeMap,
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ) {
    val coordinateSnapshot = coordinates.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressLongAddressStatusFunction("mln_map_add_image_source_image")
          .invokeNative(
            map,
            stringView(arena, sourceId),
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
            premultipliedRgba8Image(arena, image),
          ) as Int
      )
    }
  }

  internal fun setImageSourceUrl(map: NativeMap, sourceId: String, url: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_set_image_source_url")
          .invokeNative(map, stringView(arena, sourceId), stringView(arena, url)) as Int
      )
    }
  }

  internal fun setImageSourceImage(
    map: NativeMap,
    sourceId: String,
    image: PremultipliedRgba8Image,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_set_image_source_image")
          .invokeNative(map, stringView(arena, sourceId), premultipliedRgba8Image(arena, image))
          as Int
      )
    }
  }

  internal fun setImageSourceCoordinates(
    map: NativeMap,
    sourceId: String,
    coordinates: List<LatLng>,
  ) {
    val coordinateSnapshot = coordinates.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressLongStatusFunction("mln_map_set_image_source_coordinates")
          .invokeNative(
            map,
            stringView(arena, sourceId),
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
          ) as Int
      )
    }
  }

  internal fun imageSourceCoordinates(map: NativeMap, sourceId: String): List<LatLng>? =
    Arena.ofConfined().use { arena ->
      val outCoordinates = arena.allocate(latLngLayout.byteSize() * IMAGE_SOURCE_COORDINATE_COUNT)
      val outCoordinateCount = arena.allocate(ValueLayout.JAVA_LONG)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressLongTwoAddressStatusFunction("mln_map_get_image_source_coordinates")
          .invokeNative(
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

  internal fun addStyleLayerJson(map: NativeMap, layerJson: ByteArray, beforeLayerId: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStringViewStatusFunction("mln_map_add_style_layer_json")
          .invokeNative(map, byteArrayView(arena, layerJson), stringView(arena, beforeLayerId))
          as Int
      )
    }
  }

  internal fun removeStyleLayer(map: NativeMap, layerId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outRemoved = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_remove_style_layer")
          .invokeNative(map, stringView(arena, layerId), outRemoved) as Int
      )
      outRemoved.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleLayerExists(map: NativeMap, layerId: String): Boolean =
    Arena.ofConfined().use { arena ->
      val outExists = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_style_layer_exists")
          .invokeNative(map, stringView(arena, layerId), outExists) as Int
      )
      outExists.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun styleLayerType(map: NativeMap, layerId: String): String? =
    Arena.ofConfined().use { arena ->
      val outType = arena.allocate(STRING_VIEW_SIZE)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_get_style_layer_type")
          .invokeNative(map, stringView(arena, layerId), outType, outFound) as Int
      )
      if (outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) stringView(outType) else null
    }

  internal fun styleLayerIds(map: NativeMap): List<String> =
    Arena.ofConfined().use { arena ->
      val outList = arena.allocate(ValueLayout.JAVA_LONG)
      outList.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(mapListStyleLayerIdsFunction().invokeNative(map, outList) as Int)
      styleIdList(NativeStyleIdList(outList.get(ValueLayout.JAVA_LONG, 0)))
    }

  internal fun addHillshadeLayer(
    map: NativeMap,
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapThreeStringViewsStatusFunction("mln_map_add_hillshade_layer")
          .invokeNative(
            map,
            stringView(arena, layerId),
            stringView(arena, sourceId),
            stringView(arena, beforeLayerId),
          ) as Int
      )
    }
  }

  internal fun addColorReliefLayer(
    map: NativeMap,
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapThreeStringViewsStatusFunction("mln_map_add_color_relief_layer")
          .invokeNative(
            map,
            stringView(arena, layerId),
            stringView(arena, sourceId),
            stringView(arena, beforeLayerId),
          ) as Int
      )
    }
  }

  internal fun addLocationIndicatorLayer(map: NativeMap, layerId: String, beforeLayerId: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_add_location_indicator_layer")
          .invokeNative(map, stringView(arena, layerId), stringView(arena, beforeLayerId)) as Int
      )
    }
  }

  internal fun setLocationIndicatorLocation(
    map: NativeMap,
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewLatLngDoubleStatusFunction("mln_map_set_location_indicator_location")
          .invokeNative(map, stringView(arena, layerId), latLng(coordinate, arena), altitude) as Int
      )
    }
  }

  internal fun setLocationIndicatorBearing(map: NativeMap, layerId: String, bearing: Double) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewDoubleStatusFunction("mln_map_set_location_indicator_bearing")
          .invokeNative(map, stringView(arena, layerId), bearing) as Int
      )
    }
  }

  internal fun setLocationIndicatorAccuracyRadius(map: NativeMap, layerId: String, radius: Double) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewDoubleStatusFunction("mln_map_set_location_indicator_accuracy_radius")
          .invokeNative(map, stringView(arena, layerId), radius) as Int
      )
    }
  }

  internal fun setLocationIndicatorImageName(
    map: NativeMap,
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewIntStringViewStatusFunction("mln_map_set_location_indicator_image_name")
          .invokeNative(
            map,
            stringView(arena, layerId),
            imageKind.nativeValue,
            stringView(arena, imageId),
          ) as Int
      )
    }
  }

  internal fun moveStyleLayer(map: NativeMap, layerId: String, beforeLayerId: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_move_style_layer")
          .invokeNative(map, stringView(arena, layerId), stringView(arena, beforeLayerId)) as Int
      )
    }
  }

  internal fun styleLayerJson(map: NativeMap, layerId: String): ByteArray? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.JAVA_LONG)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      outSnapshot.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        mapStringViewTwoAddressStatusFunction("mln_map_get_style_layer_json")
          .invokeNative(map, stringView(arena, layerId), outSnapshot, outFound) as Int
      )
      if (outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        ownedBuffer(NativeOwnedBuffer(outSnapshot.get(ValueLayout.JAVA_LONG, 0)))
      } else null
    }

  internal fun setStyleLightJson(map: NativeMap, lightJson: ByteArray) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_style_light_json")
          .invokeNative(map, byteArrayView(arena, lightJson)) as Int
      )
    }
  }

  internal fun setStyleLightProperty(map: NativeMap, propertyName: String, value: ByteArray) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_set_style_light_property")
          .invokeNative(map, stringView(arena, propertyName), byteArrayView(arena, value)) as Int
      )
    }
  }

  internal fun styleLightProperty(map: NativeMap, propertyName: String): ByteArray? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.JAVA_LONG)
      outSnapshot.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_get_style_light_property")
          .invokeNative(map, stringView(arena, propertyName), outSnapshot) as Int
      )
      ownedBuffer(NativeOwnedBuffer(outSnapshot.get(ValueLayout.JAVA_LONG, 0)))
    }

  internal fun setStyleTransitionOptions(map: NativeMap, options: StyleTransitionOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_style_transition_options")
          .invokeNative(map, styleTransitionOptions(arena, options)) as Int
      )
    }
  }

  internal fun styleTransitionOptions(map: NativeMap): StyleTransitionOptions =
    Arena.ofConfined().use { arena ->
      val outOptions = styleTransitionOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_style_transition_options")
          .invokeNative(map, outOptions) as Int
      )
      styleTransitionOptions(outOptions)
    }

  internal fun setLayerProperty(
    map: NativeMap,
    layerId: String,
    propertyName: String,
    value: ByteArray,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsAddressStatusFunction("mln_map_set_layer_property")
          .invokeNative(
            map,
            stringView(arena, layerId),
            stringView(arena, propertyName),
            byteArrayView(arena, value),
          ) as Int
      )
    }
  }

  internal fun layerProperty(map: NativeMap, layerId: String, propertyName: String): ByteArray? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.JAVA_LONG)
      outSnapshot.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        mapTwoStringViewsAddressStatusFunction("mln_map_get_layer_property")
          .invokeNative(
            map,
            stringView(arena, layerId),
            stringView(arena, propertyName),
            outSnapshot,
          ) as Int
      )
      ownedBuffer(NativeOwnedBuffer(outSnapshot.get(ValueLayout.JAVA_LONG, 0)))
    }

  internal fun setLayerFilter(map: NativeMap, layerId: String, filter: ByteArray) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_set_layer_filter")
          .invokeNative(map, stringView(arena, layerId), byteArrayView(arena, filter)) as Int
      )
    }
  }

  internal fun clearLayerFilter(map: NativeMap, layerId: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_set_layer_filter")
          .invokeNative(map, stringView(arena, layerId), MemorySegment.NULL) as Int
      )
    }
  }

  internal fun layerFilter(map: NativeMap, layerId: String): ByteArray? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.JAVA_LONG)
      outSnapshot.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_get_layer_filter")
          .invokeNative(map, stringView(arena, layerId), outSnapshot) as Int
      )
      ownedBuffer(NativeOwnedBuffer(outSnapshot.get(ValueLayout.JAVA_LONG, 0)))
    }

  private fun stretchArray(arena: Arena, stretches: List<ImageStretch>): MemorySegment {
    if (stretches.isEmpty()) return MemorySegment.NULL
    val array = arena.allocate(IMAGE_STRETCH_SIZE * stretches.size)
    stretches.forEachIndexed { index, stretch ->
      val element = array.asSlice(IMAGE_STRETCH_SIZE * index, IMAGE_STRETCH_SIZE)
      element.set(ValueLayout.JAVA_FLOAT, IMAGE_STRETCH_FROM_OFFSET, stretch.from)
      element.set(ValueLayout.JAVA_FLOAT, IMAGE_STRETCH_TO_OFFSET, stretch.to)
    }
    return array
  }

  /**
   * Probes the required interval counts, then copies. Null arrays with zero capacity are a size
   * probe the C API answers with OK.
   */
  internal fun styleImageStretches(
    map: NativeMap,
    imageId: String,
  ): Pair<List<ImageStretch>, List<ImageStretch>>? =
    Arena.ofConfined().use { arena ->
      val function = downcall("mln_map_copy_style_image_stretches")
      val outXCount = arena.allocate(ValueLayout.JAVA_LONG)
      val outYCount = arena.allocate(ValueLayout.JAVA_LONG)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        function.invokeNative(
          map,
          stringView(arena, imageId),
          MemorySegment.NULL,
          0L,
          outXCount,
          MemorySegment.NULL,
          0L,
          outYCount,
          outFound,
        ) as Int
      )
      if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        return@use null
      }

      val xCount = outXCount.get(ValueLayout.JAVA_LONG, 0)
      val yCount = outYCount.get(ValueLayout.JAVA_LONG, 0)
      val rawX =
        if (xCount == 0L) MemorySegment.NULL else arena.allocate(IMAGE_STRETCH_SIZE * xCount)
      val rawY =
        if (yCount == 0L) MemorySegment.NULL else arena.allocate(IMAGE_STRETCH_SIZE * yCount)
      Status.check(
        function.invokeNative(
          map,
          stringView(arena, imageId),
          rawX,
          xCount,
          outXCount,
          rawY,
          yCount,
          outYCount,
          outFound,
        ) as Int
      )
      readStretches(rawX, xCount) to readStretches(rawY, yCount)
    }

  private fun readStretches(array: MemorySegment, count: Long): List<ImageStretch> =
    List(count.toInt()) { index ->
      val element = array.asSlice(IMAGE_STRETCH_SIZE * index, IMAGE_STRETCH_SIZE)
      ImageStretch(
        element.get(ValueLayout.JAVA_FLOAT, IMAGE_STRETCH_FROM_OFFSET),
        element.get(ValueLayout.JAVA_FLOAT, IMAGE_STRETCH_TO_OFFSET),
      )
    }

  internal fun setLayerSourceLayer(map: NativeMap, layerId: String, sourceLayer: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_set_layer_source_layer")
          .invokeNative(map, stringView(arena, layerId), stringView(arena, sourceLayer)) as Int
      )
    }
  }

  internal fun layerSourceLayer(map: NativeMap, layerId: String): String =
    copyLayerText(map, layerId, "mln_map_copy_layer_source_layer")

  internal fun setLayerSourceId(map: NativeMap, layerId: String, sourceId: String) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsStatusFunction("mln_map_set_layer_source_id")
          .invokeNative(map, stringView(arena, layerId), stringView(arena, sourceId)) as Int
      )
    }
  }

  internal fun layerSourceId(map: NativeMap, layerId: String): String =
    copyLayerText(map, layerId, "mln_map_copy_layer_source_id")

  /**
   * Probes the required length, then copies. A null buffer with zero capacity is a size probe the C
   * API answers with OK.
   */
  internal fun loadedStyleJson(map: NativeMap): ByteArray =
    copyMapBytes(map, "mln_map_copy_loaded_style_json")

  internal fun styleUrl(map: NativeMap): String = copyMapText(map, "mln_map_copy_style_url")

  private fun copyMapBytes(map: NativeMap, name: String): ByteArray =
    Arena.ofConfined().use { arena ->
      val function = downcall(name)
      val outSize = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(function.invokeNative(map, MemorySegment.NULL, 0L, outSize) as Int)
      val required = outSize.get(ValueLayout.JAVA_LONG, 0)
      if (required == 0L) return@use byteArrayOf()
      val buffer = arena.allocate(required)
      val outCopied = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(function.invokeNative(map, buffer, required, outCopied) as Int)
      buffer.asSlice(0, outCopied.get(ValueLayout.JAVA_LONG, 0)).toArray(ValueLayout.JAVA_BYTE)
    }

  private fun copyMapText(map: NativeMap, name: String): String =
    Arena.ofConfined().use { arena ->
      val function = downcall(name)
      val outSize = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(function.invokeNative(map, MemorySegment.NULL, 0L, outSize) as Int)
      val required = outSize.get(ValueLayout.JAVA_LONG, 0)
      if (required == 0L) {
        ""
      } else {
        val buffer = arena.allocate(required)
        val outCopied = arena.allocate(ValueLayout.JAVA_LONG)
        Status.check(function.invokeNative(map, buffer, required, outCopied) as Int)
        val copied = outCopied.get(ValueLayout.JAVA_LONG, 0)
        buffer.asSlice(0, copied).toArray(ValueLayout.JAVA_BYTE).decodeToString()
      }
    }

  private fun copyLayerText(map: NativeMap, layerId: String, name: String): String =
    Arena.ofConfined().use { arena ->
      val function = mapStringViewAddressLongAddressStatusFunction(name)
      val outSize = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        function.invokeNative(map, stringView(arena, layerId), MemorySegment.NULL, 0L, outSize)
          as Int
      )
      val required = outSize.get(ValueLayout.JAVA_LONG, 0)
      if (required == 0L) {
        ""
      } else {
        val buffer = arena.allocate(required)
        val outCopied = arena.allocate(ValueLayout.JAVA_LONG)
        Status.check(
          function.invokeNative(map, stringView(arena, layerId), buffer, required, outCopied) as Int
        )
        val copied = outCopied.get(ValueLayout.JAVA_LONG, 0)
        buffer.asSlice(0, copied).toArray(ValueLayout.JAVA_BYTE).decodeToString()
      }
    }

  internal fun setLayerMinZoom(map: NativeMap, layerId: String, minZoom: Double) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewDoubleStatusFunction("mln_map_set_layer_min_zoom")
          .invokeNative(map, stringView(arena, layerId), minZoom) as Int
      )
    }
  }

  internal fun layerMinZoom(map: NativeMap, layerId: String): Double =
    layerZoom(map, layerId, "mln_map_get_layer_min_zoom")

  internal fun setLayerMaxZoom(map: NativeMap, layerId: String, maxZoom: Double) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewDoubleStatusFunction("mln_map_set_layer_max_zoom")
          .invokeNative(map, stringView(arena, layerId), maxZoom) as Int
      )
    }
  }

  internal fun layerMaxZoom(map: NativeMap, layerId: String): Double =
    layerZoom(map, layerId, "mln_map_get_layer_max_zoom")

  private fun layerZoom(map: NativeMap, layerId: String, name: String): Double =
    Arena.ofConfined().use { arena ->
      val outZoom = arena.allocate(ValueLayout.JAVA_DOUBLE)
      Status.check(
        mapStringViewAddressStatusFunction(name)
          .invokeNative(map, stringView(arena, layerId), outZoom) as Int
      )
      outZoom.get(ValueLayout.JAVA_DOUBLE, 0)
    }

  internal fun setLayerVisibility(map: NativeMap, layerId: String, visibility: Int) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewIntStatusFunction("mln_map_set_layer_visibility")
          .invokeNative(map, stringView(arena, layerId), visibility) as Int
      )
    }
  }

  internal fun layerVisibility(map: NativeMap, layerId: String): Int =
    Arena.ofConfined().use { arena ->
      val outVisibility = arena.allocate(ValueLayout.JAVA_INT)
      Status.check(
        mapStringViewAddressStatusFunction("mln_map_get_layer_visibility")
          .invokeNative(map, stringView(arena, layerId), outVisibility) as Int
      )
      outVisibility.get(ValueLayout.JAVA_INT, 0)
    }

  internal fun requestRepaint(map: NativeMap) {
    Status.check(mapStatusFunction("mln_map_request_repaint").invokeNative(map) as Int)
  }

  internal fun requestStillImage(map: NativeMap) {
    Status.check(mapStatusFunction("mln_map_request_still_image").invokeNative(map) as Int)
  }

  internal fun setDebugOptions(map: NativeMap, options: Set<DebugOption>) {
    val mask = options.fold(0) { acc, option -> acc or option.nativeMask }
    Status.check(mapIntStatusFunction("mln_map_set_debug_options").invokeNative(map, mask) as Int)
  }

  internal fun debugOptions(map: NativeMap): Set<DebugOption> =
    Arena.ofConfined().use { arena ->
      val outOptions = arena.allocate(ValueLayout.JAVA_INT)
      Status.check(
        mapAddressStatusFunction("mln_map_get_debug_options").invokeNative(map, outOptions) as Int
      )
      debugOptions(outOptions.get(ValueLayout.JAVA_INT, 0))
    }

  internal fun setRenderingStatsViewEnabled(map: NativeMap, enabled: Boolean) {
    Status.check(
      mapBooleanStatusFunction("mln_map_set_rendering_stats_view_enabled")
        .invokeNative(map, enabled) as Int
    )
  }

  internal fun renderingStatsViewEnabled(map: NativeMap): Boolean =
    Arena.ofConfined().use { arena ->
      val outEnabled = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapAddressStatusFunction("mln_map_get_rendering_stats_view_enabled")
          .invokeNative(map, outEnabled) as Int
      )
      outEnabled.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun isFullyLoaded(map: NativeMap): Boolean =
    Arena.ofConfined().use { arena ->
      val outLoaded = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapAddressStatusFunction("mln_map_is_fully_loaded").invokeNative(map, outLoaded) as Int
      )
      outLoaded.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun dumpDebugLogs(map: NativeMap) {
    Status.check(mapStatusFunction("mln_map_dump_debug_logs").invokeNative(map) as Int)
  }

  internal fun mapSize(map: NativeMap): MapSize =
    Arena.ofConfined().use { arena ->
      val outWidth = arena.allocate(ValueLayout.JAVA_INT)
      val outHeight = arena.allocate(ValueLayout.JAVA_INT)
      val outScaleFactor = arena.allocate(ValueLayout.JAVA_DOUBLE)
      Status.check(
        mapAddressAddressAddressStatusFunction("mln_map_get_size")
          .invokeNative(map, outWidth, outHeight, outScaleFactor) as Int
      )
      MapSize(
        outWidth.get(ValueLayout.JAVA_INT, 0),
        outHeight.get(ValueLayout.JAVA_INT, 0),
        outScaleFactor.get(ValueLayout.JAVA_DOUBLE, 0),
      )
    }

  internal fun viewportOptions(map: NativeMap): ViewportOptions =
    Arena.ofConfined().use { arena ->
      val outOptions = viewportOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_viewport_options").invokeNative(map, outOptions)
          as Int
      )
      readViewportOptions(outOptions)
    }

  internal fun setViewportOptions(map: NativeMap, options: ViewportOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_viewport_options")
          .invokeNative(map, viewportOptions(arena, options)) as Int
      )
    }
  }

  internal fun tileOptions(map: NativeMap): TileOptions =
    Arena.ofConfined().use { arena ->
      val outOptions = tileOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_tile_options").invokeNative(map, outOptions) as Int
      )
      readTileOptions(outOptions)
    }

  internal fun setTileOptions(map: NativeMap, options: TileOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_tile_options")
          .invokeNative(map, tileOptions(arena, options)) as Int
      )
    }
  }

  internal fun projectionMode(map: NativeMap): ProjectionModeOptions =
    Arena.ofConfined().use { arena ->
      val outMode = projectionModeDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_projection_mode").invokeNative(map, outMode) as Int
      )
      projectionModeOptions(outMode)
    }

  internal fun setProjectionMode(map: NativeMap, mode: ProjectionModeOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_projection_mode")
          .invokeNative(map, projectionModeOptions(arena, mode)) as Int
      )
    }
  }

  internal fun camera(map: NativeMap): CameraOptions =
    Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_camera").invokeNative(map, outCamera) as Int
      )
      cameraOptions(outCamera)
    }

  internal fun jumpTo(map: NativeMap, camera: CameraOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_jump_to").invokeNative(map, cameraOptions(arena, camera))
          as Int
      )
    }
  }

  internal fun easeTo(map: NativeMap, camera: CameraOptions, animation: AnimationOptions?) {
    mapCameraAnimationCommand("mln_map_ease_to", map, camera, animation)
  }

  internal fun flyTo(map: NativeMap, camera: CameraOptions, animation: AnimationOptions?) {
    mapCameraAnimationCommand("mln_map_fly_to", map, camera, animation)
  }

  internal fun moveBy(map: NativeMap, deltaX: Double, deltaY: Double) {
    Status.check(
      mapDoubleDoubleStatusFunction("mln_map_move_by").invokeNative(map, deltaX, deltaY) as Int
    )
  }

  internal fun moveByAnimated(
    map: NativeMap,
    deltaX: Double,
    deltaY: Double,
    animation: AnimationOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapDoubleDoubleAddressStatusFunction("mln_map_move_by_animated")
          .invokeNative(map, deltaX, deltaY, animationOptions(arena, animation)) as Int
      )
    }
  }

  internal fun scaleBy(map: NativeMap, scale: Double, anchor: ScreenPoint?) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapDoubleAddressStatusFunction("mln_map_scale_by")
          .invokeNative(map, scale, anchor?.let { screenPoint(it, arena) } ?: MemorySegment.NULL)
          as Int
      )
    }
  }

  internal fun scaleByAnimated(
    map: NativeMap,
    scale: Double,
    anchor: ScreenPoint?,
    animation: AnimationOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapDoubleAddressAddressStatusFunction("mln_map_scale_by_animated")
          .invokeNative(
            map,
            scale,
            anchor?.let { screenPoint(it, arena) } ?: MemorySegment.NULL,
            animationOptions(arena, animation),
          ) as Int
      )
    }
  }

  internal fun rotateBy(map: NativeMap, first: ScreenPoint, second: ScreenPoint) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoScreenPointsStatusFunction("mln_map_rotate_by")
          .invokeNative(map, screenPoint(first, arena), screenPoint(second, arena)) as Int
      )
    }
  }

  internal fun rotateByAnimated(
    map: NativeMap,
    first: ScreenPoint,
    second: ScreenPoint,
    animation: AnimationOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoScreenPointsAddressStatusFunction("mln_map_rotate_by_animated")
          .invokeNative(
            map,
            screenPoint(first, arena),
            screenPoint(second, arena),
            animationOptions(arena, animation),
          ) as Int
      )
    }
  }

  internal fun pitchBy(map: NativeMap, pitch: Double) {
    Status.check(mapDoubleStatusFunction("mln_map_pitch_by").invokeNative(map, pitch) as Int)
  }

  internal fun pitchByAnimated(map: NativeMap, pitch: Double, animation: AnimationOptions?) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapDoubleAddressStatusFunction("mln_map_pitch_by_animated")
          .invokeNative(map, pitch, animationOptions(arena, animation)) as Int
      )
    }
  }

  internal fun cancelTransitions(map: NativeMap) {
    Status.check(mapStatusFunction("mln_map_cancel_transitions").invokeNative(map) as Int)
  }

  internal fun setGestureInProgress(map: NativeMap, inProgress: Boolean) {
    Status.check(
      mapBooleanStatusFunction("mln_map_set_gesture_in_progress").invokeNative(map, inProgress)
        as Int
    )
  }

  internal fun isGestureInProgress(map: NativeMap): Boolean =
    Arena.ofConfined().use { arena ->
      val outInProgress = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        mapAddressStatusFunction("mln_map_is_gesture_in_progress").invokeNative(map, outInProgress)
          as Int
      )
      outInProgress.get(ValueLayout.JAVA_BOOLEAN, 0)
    }

  internal fun cameraForLatLngBounds(
    map: NativeMap,
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): CameraOptions =
    Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapLatLngBoundsAddressAddressStatusFunction("mln_map_camera_for_lat_lng_bounds")
          .invokeNative(
            map,
            latLngBounds(arena, bounds),
            cameraFitOptions(arena, fitOptions),
            outCamera,
          ) as Int
      )
      cameraOptions(outCamera)
    }

  internal fun cameraForLatLngs(
    map: NativeMap,
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): CameraOptions {
    val coordinateSnapshot = coordinates.toList()
    return Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapAddressLongAddressAddressStatusFunction("mln_map_camera_for_lat_lngs")
          .invokeNative(
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
    map: NativeMap,
    geometry: ByteArray,
    fitOptions: CameraFitOptions?,
  ): CameraOptions =
    Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapAddressAddressAddressStatusFunction("mln_map_camera_for_geometry")
          .invokeNative(
            map,
            byteArrayView(arena, geometry),
            cameraFitOptions(arena, fitOptions),
            outCamera,
          ) as Int
      )
      cameraOptions(outCamera)
    }

  internal fun latLngBoundsForCamera(map: NativeMap, camera: CameraOptions): LatLngBounds =
    mapLatLngBoundsForCamera("mln_map_lat_lng_bounds_for_camera", map, camera)

  internal fun latLngBoundsForCameraUnwrapped(map: NativeMap, camera: CameraOptions): LatLngBounds =
    mapLatLngBoundsForCamera("mln_map_lat_lng_bounds_for_camera_unwrapped", map, camera)

  internal fun bounds(map: NativeMap): BoundOptions =
    Arena.ofConfined().use { arena ->
      val outBounds = boundOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_bounds").invokeNative(map, outBounds) as Int
      )
      boundOptions(outBounds)
    }

  internal fun setBounds(map: NativeMap, options: BoundOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_bounds")
          .invokeNative(map, boundOptions(arena, options)) as Int
      )
    }
  }

  internal fun freeCameraOptions(map: NativeMap): FreeCameraOptions =
    Arena.ofConfined().use { arena ->
      val outOptions = freeCameraOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_get_free_camera_options").invokeNative(map, outOptions)
          as Int
      )
      readFreeCameraOptions(outOptions)
    }

  internal fun setFreeCameraOptions(map: NativeMap, options: FreeCameraOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_set_free_camera_options")
          .invokeNative(map, freeCameraOptions(arena, options)) as Int
      )
    }
  }

  internal fun pixelForLatLng(map: NativeMap, coordinate: LatLng): ScreenPoint =
    Arena.ofConfined().use { arena ->
      val outPoint = arena.allocate(screenPointLayout)
      Status.check(
        mapLatLngAddressStatusFunction("mln_map_pixel_for_lat_lng")
          .invokeNative(map, latLng(coordinate, arena), outPoint) as Int
      )
      screenPoint(outPoint)
    }

  internal fun latLngForPixel(map: NativeMap, point: ScreenPoint): LatLng =
    Arena.ofConfined().use { arena ->
      val outCoordinate = arena.allocate(latLngLayout)
      Status.check(
        mapScreenPointAddressStatusFunction("mln_map_lat_lng_for_pixel")
          .invokeNative(map, screenPoint(point, arena), outCoordinate) as Int
      )
      latLng(outCoordinate)
    }

  internal fun latLngForPixelUnwrapped(map: NativeMap, point: ScreenPoint): LatLng =
    Arena.ofConfined().use { arena ->
      val outCoordinate = arena.allocate(latLngLayout)
      Status.check(
        mapScreenPointAddressStatusFunction("mln_map_lat_lng_for_pixel_unwrapped")
          .invokeNative(map, screenPoint(point, arena), outCoordinate) as Int
      )
      latLng(outCoordinate)
    }

  internal fun pixelsForLatLngs(map: NativeMap, coordinates: List<LatLng>): List<ScreenPoint> {
    val coordinateSnapshot = coordinates.toList()
    if (coordinateSnapshot.isEmpty()) {
      Arena.ofConfined().use { arena ->
        Status.check(
          mapAddressLongAddressStatusFunction("mln_map_pixels_for_lat_lngs")
            .invokeNative(map, MemorySegment.NULL, 0L, MemorySegment.NULL) as Int
        )
      }
      return emptyList()
    }
    return Arena.ofConfined().use { arena ->
      val outPoints = arena.allocate(screenPointLayout.byteSize() * coordinateSnapshot.size)
      Status.check(
        mapAddressLongAddressStatusFunction("mln_map_pixels_for_lat_lngs")
          .invokeNative(
            map,
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
            outPoints,
          ) as Int
      )
      screenPointArray(outPoints, coordinateSnapshot.size)
    }
  }

  internal fun latLngsForPixels(map: NativeMap, points: List<ScreenPoint>): List<LatLng> {
    val pointSnapshot = points.toList()
    if (pointSnapshot.isEmpty()) {
      Arena.ofConfined().use { arena ->
        Status.check(
          mapAddressLongAddressStatusFunction("mln_map_lat_lngs_for_pixels")
            .invokeNative(map, MemorySegment.NULL, 0L, MemorySegment.NULL) as Int
        )
      }
      return emptyList()
    }
    return Arena.ofConfined().use { arena ->
      val outCoordinates = arena.allocate(latLngLayout.byteSize() * pointSnapshot.size)
      Status.check(
        mapAddressLongAddressStatusFunction("mln_map_lat_lngs_for_pixels")
          .invokeNative(
            map,
            screenPointArray(arena, pointSnapshot),
            pointSnapshot.size.toLong(),
            outCoordinates,
          ) as Int
      )
      latLngArray(outCoordinates, pointSnapshot.size)
    }
  }

  internal fun latLngsForPixelsUnwrapped(map: NativeMap, points: List<ScreenPoint>): List<LatLng> {
    val pointSnapshot = points.toList()
    if (pointSnapshot.isEmpty()) {
      Arena.ofConfined().use {
        Status.check(
          mapAddressLongAddressStatusFunction("mln_map_lat_lngs_for_pixels_unwrapped")
            .invokeNative(map, MemorySegment.NULL, 0L, MemorySegment.NULL) as Int
        )
      }
      return emptyList()
    }
    return Arena.ofConfined().use { arena ->
      val outCoordinates = arena.allocate(latLngLayout.byteSize() * pointSnapshot.size)
      Status.check(
        mapAddressLongAddressStatusFunction("mln_map_lat_lngs_for_pixels_unwrapped")
          .invokeNative(
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
    map: NativeMap,
    descriptor: MetalOwnedTextureDescriptor,
  ): NativeRenderSession =
    attachRenderSession(
      map,
      "mln_metal_owned_texture_attach",
      metalOwnedTextureDescriptor(descriptor),
    )

  internal fun attachMetalBorrowedTexture(
    map: NativeMap,
    descriptor: MetalBorrowedTextureDescriptor,
  ): NativeRenderSession =
    attachRenderSession(
      map,
      "mln_metal_borrowed_texture_attach",
      metalBorrowedTextureDescriptor(descriptor),
    )

  internal fun attachVulkanOwnedTexture(
    map: NativeMap,
    descriptor: VulkanOwnedTextureDescriptor,
  ): NativeRenderSession =
    attachRenderSession(
      map,
      "mln_vulkan_owned_texture_attach",
      vulkanOwnedTextureDescriptor(descriptor),
    )

  internal fun attachVulkanBorrowedTexture(
    map: NativeMap,
    descriptor: VulkanBorrowedTextureDescriptor,
  ): NativeRenderSession =
    attachRenderSession(
      map,
      "mln_vulkan_borrowed_texture_attach",
      vulkanBorrowedTextureDescriptor(descriptor),
    )

  internal fun attachOpenGLOwnedTexture(
    map: NativeMap,
    descriptor: OpenGLOwnedTextureDescriptor,
  ): NativeRenderSession =
    attachRenderSession(
      map,
      "mln_opengl_owned_texture_attach",
      openglOwnedTextureDescriptor(descriptor),
    )

  internal fun attachOpenGLBorrowedTexture(
    map: NativeMap,
    descriptor: OpenGLBorrowedTextureDescriptor,
  ): NativeRenderSession =
    attachRenderSession(
      map,
      "mln_opengl_borrowed_texture_attach",
      openglBorrowedTextureDescriptor(descriptor),
    )

  internal fun attachMetalSurface(
    map: NativeMap,
    descriptor: MetalSurfaceDescriptor,
  ): NativeRenderSession =
    attachRenderSession(map, "mln_metal_surface_attach", metalSurfaceDescriptor(descriptor))

  internal fun attachVulkanSurface(
    map: NativeMap,
    descriptor: VulkanSurfaceDescriptor,
  ): NativeRenderSession =
    attachRenderSession(map, "mln_vulkan_surface_attach", vulkanSurfaceDescriptor(descriptor))

  internal fun attachOpenGLSurface(
    map: NativeMap,
    descriptor: OpenGLSurfaceDescriptor,
  ): NativeRenderSession =
    attachRenderSession(map, "mln_opengl_surface_attach", openglSurfaceDescriptor(descriptor))

  internal fun destroyRenderSession(session: NativeRenderSession): Int =
    renderSessionStatusFunction("mln_render_session_destroy").invokeNative(session) as Int

  internal fun resizeRenderSession(
    session: NativeRenderSession,
    width: Int,
    height: Int,
    scaleFactor: Double,
  ) {
    Status.check(
      renderSessionResizeFunction().invokeNative(session, width, height, scaleFactor) as Int
    )
  }

  internal fun setMetalSurfaceTarget(
    session: NativeRenderSession,
    descriptor: MetalSurfaceDescriptor,
  ) {
    setRenderSessionTarget(
      session,
      "mln_metal_surface_set_target",
      metalSurfaceDescriptor(descriptor),
    )
  }

  internal fun setVulkanSurfaceTarget(
    session: NativeRenderSession,
    descriptor: VulkanSurfaceDescriptor,
  ) {
    setRenderSessionTarget(
      session,
      "mln_vulkan_surface_set_target",
      vulkanSurfaceDescriptor(descriptor),
    )
  }

  internal fun setOpenGLSurfaceTarget(
    session: NativeRenderSession,
    descriptor: OpenGLSurfaceDescriptor,
  ) {
    setRenderSessionTarget(
      session,
      "mln_opengl_surface_set_target",
      openglSurfaceDescriptor(descriptor),
    )
  }

  internal fun setMetalBorrowedTextureTarget(
    session: NativeRenderSession,
    descriptor: MetalBorrowedTextureDescriptor,
  ) {
    setRenderSessionTarget(
      session,
      "mln_metal_borrowed_texture_set_target",
      metalBorrowedTextureDescriptor(descriptor),
    )
  }

  internal fun setVulkanBorrowedTextureTarget(
    session: NativeRenderSession,
    descriptor: VulkanBorrowedTextureDescriptor,
  ) {
    setRenderSessionTarget(
      session,
      "mln_vulkan_borrowed_texture_set_target",
      vulkanBorrowedTextureDescriptor(descriptor),
    )
  }

  internal fun setOpenGLBorrowedTextureTarget(
    session: NativeRenderSession,
    descriptor: OpenGLBorrowedTextureDescriptor,
  ) {
    setRenderSessionTarget(
      session,
      "mln_opengl_borrowed_texture_set_target",
      openglBorrowedTextureDescriptor(descriptor),
    )
  }

  internal fun renderUpdate(session: NativeRenderSession): RenderUpdate =
    Arena.ofConfined().use { arena ->
      val outResult = arena.allocate(ValueLayout.JAVA_INT)
      val outNeedsRepaint = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      Status.check(
        renderSessionRenderUpdateFunction().invokeNative(session, outResult, outNeedsRepaint) as Int
      )
      RenderUpdate(
        RenderResult.fromNative(outResult.get(ValueLayout.JAVA_INT, 0)),
        outNeedsRepaint.get(ValueLayout.JAVA_BOOLEAN, 0),
      )
    }

  internal fun detachRenderSession(session: NativeRenderSession) {
    Status.check(
      renderSessionStatusFunction("mln_render_session_detach").invokeNative(session) as Int
    )
  }

  internal fun reduceRenderSessionMemoryUse(session: NativeRenderSession) {
    Status.check(
      renderSessionStatusFunction("mln_render_session_reduce_memory_use").invokeNative(session)
        as Int
    )
  }

  internal fun clearRenderSessionData(session: NativeRenderSession) {
    Status.check(
      renderSessionStatusFunction("mln_render_session_clear_data").invokeNative(session) as Int
    )
  }

  internal fun dumpRenderSessionDebugLogs(session: NativeRenderSession) {
    Status.check(
      renderSessionStatusFunction("mln_render_session_dump_debug_logs").invokeNative(session) as Int
    )
  }

  internal fun queryRenderedFeatures(
    session: NativeRenderSession,
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature> =
    Arena.ofConfined().use { arena ->
      val outResult = arena.allocate(ValueLayout.JAVA_LONG)
      outResult.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        renderSessionQueryRenderedFeaturesFunction()
          .invokeNative(
            session,
            renderedQueryGeometry(arena, geometry),
            renderedFeatureQueryOptions(arena, options),
            outResult,
          ) as Int
      )
      queriedFeatureList(NativeQueriedFeatureList(outResult.get(ValueLayout.JAVA_LONG, 0)))
    }

  internal fun querySourceFeatures(
    session: NativeRenderSession,
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature> =
    Arena.ofConfined().use { arena ->
      val outResult = arena.allocate(ValueLayout.JAVA_LONG)
      outResult.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        renderSessionQuerySourceFeaturesFunction()
          .invokeNative(
            session,
            stringView(arena, sourceId),
            sourceFeatureQueryOptions(arena, options),
            outResult,
          ) as Int
      )
      queriedFeatureList(NativeQueriedFeatureList(outResult.get(ValueLayout.JAVA_LONG, 0)))
    }

  internal fun queryFeatureExtension(
    session: NativeRenderSession,
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): ByteArray =
    Arena.ofConfined().use { arena ->
      val outResult = arena.allocate(ValueLayout.JAVA_LONG)
      outResult.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        renderSessionQueryFeatureExtensionsFunction()
          .invokeNative(
            session,
            stringView(arena, sourceId),
            byteArrayView(arena, feature),
            stringView(arena, extension),
            stringView(arena, extensionField),
            arguments?.let { byteArrayView(arena, it) } ?: MemorySegment.NULL,
            outResult,
          ) as Int
      )
      ownedBuffer(NativeOwnedBuffer(outResult.get(ValueLayout.JAVA_LONG, 0)))!!
    }

  internal fun textureImageInfo(session: NativeRenderSession): TextureImageInfo =
    Arena.ofConfined().use { arena ->
      val outInfo = textureImageInfo(arena)
      val status =
        textureReadPremultipliedRgba8Function()
          .invokeNative(session, MemorySegment.NULL, 0L, outInfo) as Int
      val info = readTextureImageInfo(outInfo)
      if (status == 0 || (status == -1 && info.byteLength > 0L)) {
        info
      } else {
        Status.check(status)
        error("unreachable")
      }
    }

  internal fun readPremultipliedRgba8(
    session: NativeRenderSession,
    buffer: NativeBuffer,
  ): TextureImageInfo =
    Arena.ofConfined().use { arena ->
      val outInfo = textureImageInfo(arena)
      buffer.borrow { segment, length ->
        Status.check(
          textureReadPremultipliedRgba8Function().invokeNative(session, segment, length, outInfo)
            as Int
        )
      }
      readTextureImageInfo(outInfo)
    }

  internal fun acquireMetalOwnedTextureFrame(
    session: NativeRenderSession
  ): OwnedTextureFrameSegment {
    val arena = Arena.ofShared()
    val frame = mln_metal_owned_texture_frame.allocate(arena)
    mln_metal_owned_texture_frame.size(frame, mln_metal_owned_texture_frame.sizeof().toInt())
    try {
      Status.check(metalOwnedTextureAcquireFrameFunction().invokeNative(session, frame) as Int)
      return OwnedTextureFrameSegment(frame, arena)
    } catch (error: Throwable) {
      arena.close()
      throw error
    }
  }

  internal fun acquireVulkanOwnedTextureFrame(
    session: NativeRenderSession
  ): OwnedTextureFrameSegment {
    val arena = Arena.ofShared()
    val frame = mln_vulkan_owned_texture_frame.allocate(arena)
    mln_vulkan_owned_texture_frame.size(frame, mln_vulkan_owned_texture_frame.sizeof().toInt())
    try {
      Status.check(vulkanOwnedTextureAcquireFrameFunction().invokeNative(session, frame) as Int)
      return OwnedTextureFrameSegment(frame, arena)
    } catch (error: Throwable) {
      arena.close()
      throw error
    }
  }

  internal fun acquireOpenGLOwnedTextureFrame(
    session: NativeRenderSession
  ): OwnedTextureFrameSegment {
    val arena = Arena.ofShared()
    val frame = mln_opengl_owned_texture_frame.allocate(arena)
    mln_opengl_owned_texture_frame.size(frame, mln_opengl_owned_texture_frame.sizeof().toInt())
    try {
      Status.check(openglOwnedTextureAcquireFrameFunction().invokeNative(session, frame) as Int)
      return OwnedTextureFrameSegment(frame, arena)
    } catch (error: Throwable) {
      arena.close()
      throw error
    }
  }

  internal fun releaseMetalOwnedTextureFrame(session: NativeRenderSession, frame: MemorySegment) {
    Status.check(metalOwnedTextureReleaseFrameFunction().invokeNative(session, frame) as Int)
  }

  internal fun releaseVulkanOwnedTextureFrame(session: NativeRenderSession, frame: MemorySegment) {
    Status.check(vulkanOwnedTextureReleaseFrameFunction().invokeNative(session, frame) as Int)
  }

  internal fun releaseOpenGLOwnedTextureFrame(session: NativeRenderSession, frame: MemorySegment) {
    Status.check(openglOwnedTextureReleaseFrameFunction().invokeNative(session, frame) as Int)
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
      VulkanHandle.scoped(mln_vulkan_owned_texture_frame.image(segment), scope),
      VulkanHandle.scoped(mln_vulkan_owned_texture_frame.image_view(segment), scope),
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

  internal fun createMapProjection(map: NativeMap): NativeMapProjection =
    Arena.ofConfined().use { arena ->
      val outProjection = arena.allocate(ValueLayout.JAVA_LONG)
      outProjection.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        mapAddressStatusFunction("mln_map_projection_create").invokeNative(map, outProjection)
          as Int
      )
      NativeMapProjection(outProjection.get(ValueLayout.JAVA_LONG, 0)).also { projection ->
        require(!projection.isNull) { "mln_map_projection_create returned the null handle" }
      }
    }

  internal fun destroyMapProjection(projection: NativeMapProjection): Int =
    mapStatusFunction("mln_map_projection_destroy").invokeNative(projection) as Int

  internal fun projectionCamera(projection: NativeMapProjection): CameraOptions =
    Arena.ofConfined().use { arena ->
      val outCamera = cameraOptionsDefault(arena)
      Status.check(
        mapAddressStatusFunction("mln_map_projection_get_camera")
          .invokeNative(projection, outCamera) as Int
      )
      cameraOptions(outCamera)
    }

  internal fun setProjectionCamera(projection: NativeMapProjection, camera: CameraOptions) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapAddressStatusFunction("mln_map_projection_set_camera")
          .invokeNative(projection, cameraOptions(arena, camera)) as Int
      )
    }
  }

  internal fun setProjectionVisibleCoordinates(
    projection: NativeMapProjection,
    coordinates: List<LatLng>,
    padding: EdgeInsets,
  ) {
    val coordinateSnapshot = coordinates.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        projectionAddressLongEdgeInsetsStatusFunction("mln_map_projection_set_visible_coordinates")
          .invokeNative(
            projection,
            latLngArray(arena, coordinateSnapshot),
            coordinateSnapshot.size.toLong(),
            edgeInsets(arena, padding),
          ) as Int
      )
    }
  }

  internal fun setProjectionVisibleGeometry(
    projection: NativeMapProjection,
    geometry: ByteArray,
    padding: EdgeInsets,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        projectionAddressEdgeInsetsStatusFunction("mln_map_projection_set_visible_geometry")
          .invokeNative(projection, byteArrayView(arena, geometry), edgeInsets(arena, padding))
          as Int
      )
    }
  }

  internal fun projectionPixelForLatLng(
    projection: NativeMapProjection,
    coordinate: LatLng,
  ): ScreenPoint =
    Arena.ofConfined().use { arena ->
      val outPoint = arena.allocate(screenPointLayout)
      Status.check(
        projectionLatLngAddressStatusFunction("mln_map_projection_pixel_for_lat_lng")
          .invokeNative(projection, latLng(coordinate, arena), outPoint) as Int
      )
      screenPoint(outPoint)
    }

  internal fun projectionLatLngForPixel(
    projection: NativeMapProjection,
    point: ScreenPoint,
  ): LatLng =
    Arena.ofConfined().use { arena ->
      val outCoordinate = arena.allocate(latLngLayout)
      Status.check(
        projectionScreenPointAddressStatusFunction("mln_map_projection_lat_lng_for_pixel")
          .invokeNative(projection, screenPoint(point, arena), outCoordinate) as Int
      )
      latLng(outCoordinate)
    }

  internal fun projectionLatLngForPixelUnwrapped(
    projection: NativeMapProjection,
    point: ScreenPoint,
  ): LatLng =
    Arena.ofConfined().use { arena ->
      val outCoordinate = arena.allocate(latLngLayout)
      Status.check(
        projectionScreenPointAddressStatusFunction("mln_map_projection_lat_lng_for_pixel_unwrapped")
          .invokeNative(projection, screenPoint(point, arena), outCoordinate) as Int
      )
      latLng(outCoordinate)
    }

  internal fun setResourceTransformResponseUrl(response: MemorySegment, value: String): Int =
    Arena.ofConfined().use { arena ->
      val bytes = value.toByteArray(StandardCharsets.UTF_8)
      resourceTransformResponseSetUrlFunction()
        .invokeNative(response, nativeBytes(arena, bytes), bytes.size.toLong()) as Int
    }

  internal fun setHttpHeaderTransformResponse(
    response: MemorySegment,
    name: String,
    value: String,
  ): Int =
    Arena.ofConfined().use { arena ->
      val nameBytes = name.toByteArray(StandardCharsets.UTF_8)
      val valueBytes = value.toByteArray(StandardCharsets.UTF_8)
      httpHeaderTransformResponseSetFunction()
        .invokeNative(
          response,
          nativeBytes(arena, nameBytes),
          nameBytes.size.toLong(),
          nativeBytes(arena, valueBytes),
          valueBytes.size.toLong(),
        ) as Int
    }

  internal fun customGeometrySourceOptions(
    arena: Arena,
    value: CustomGeometrySourceOptions,
    fetchTile: MemorySegment,
    cancelTile: MemorySegment,
    releaseUserData: MemorySegment,
    userData: MemorySegment,
  ): MemorySegment {
    val segment = arena.allocate(CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZE_OFFSET,
      CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZE.toInt(),
    )
    segment.set(ValueLayout.ADDRESS, CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE_OFFSET, fetchTile)
    segment.set(ValueLayout.ADDRESS, CUSTOM_GEOMETRY_SOURCE_OPTIONS_CANCEL_TILE_OFFSET, cancelTile)
    segment.set(ValueLayout.ADDRESS, CUSTOM_GEOMETRY_SOURCE_OPTIONS_USER_DATA_OFFSET, userData)
    segment.set(
      ValueLayout.ADDRESS,
      CUSTOM_GEOMETRY_SOURCE_OPTIONS_RELEASE_USER_DATA_OFFSET,
      releaseUserData,
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

  internal fun customMvtVectorSourceOptions(
    arena: Arena,
    value: CustomMvtVectorSourceOptions,
    fetchTile: MemorySegment,
    cancelTile: MemorySegment,
    releaseUserData: MemorySegment,
    userData: MemorySegment,
  ): MemorySegment {
    val segment = arena.allocate(CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_SIZE_OFFSET,
      CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_SIZE.toInt(),
    )
    segment.set(ValueLayout.ADDRESS, CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_FETCH_TILE_OFFSET, fetchTile)
    segment.set(
      ValueLayout.ADDRESS,
      CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_CANCEL_TILE_OFFSET,
      cancelTile,
    )
    segment.set(ValueLayout.ADDRESS, CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_USER_DATA_OFFSET, userData)
    segment.set(
      ValueLayout.ADDRESS,
      CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_RELEASE_USER_DATA_OFFSET,
      releaseUserData,
    )
    var fields = 0
    value.minZoom?.let {
      fields = fields or CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
      segment.set(ValueLayout.JAVA_DOUBLE, CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_MIN_ZOOM_OFFSET, it)
    }
    value.maxZoom?.let {
      fields = fields or CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM
      segment.set(ValueLayout.JAVA_DOUBLE, CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_MAX_ZOOM_OFFSET, it)
    }
    segment.set(ValueLayout.JAVA_INT, CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_FIELDS_OFFSET, fields)
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
    map: NativeMap,
    functionName: String,
    descriptor: (Arena) -> MemorySegment,
  ): NativeRenderSession =
    Arena.ofConfined().use { arena ->
      val outSession = arena.allocate(ValueLayout.JAVA_LONG)
      outSession.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(
        mapAddressAddressStatusFunction(functionName)
          .invokeNative(map, descriptor(arena), outSession) as Int
      )
      NativeRenderSession(outSession.get(ValueLayout.JAVA_LONG, 0)).also { session ->
        require(!session.isNull) { "render session attach returned the null handle" }
      }
    }

  // The C API borrows the replacement descriptor for the call alone, so the
  // confined arena that materialized it closes once the call returns.
  private fun setRenderSessionTarget(
    session: NativeRenderSession,
    functionName: String,
    descriptor: (Arena) -> MemorySegment,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        renderSessionAddressStatusFunction(functionName).invokeNative(session, descriptor(arena))
          as Int
      )
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
      mln_metal_borrowed_texture_descriptor.physical_width(segment, descriptor.physicalWidth)
      mln_metal_borrowed_texture_descriptor.physical_height(segment, descriptor.physicalHeight)
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
      mln_vulkan_borrowed_texture_descriptor.physical_width(segment, descriptor.physicalWidth)
      mln_vulkan_borrowed_texture_descriptor.physical_height(segment, descriptor.physicalHeight)
      fillVulkanContext(mln_vulkan_borrowed_texture_descriptor.context(segment), descriptor.context)
      mln_vulkan_borrowed_texture_descriptor.image(segment, descriptor.image.bits)
      mln_vulkan_borrowed_texture_descriptor.image_view(segment, descriptor.imageView.bits)
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
      mln_opengl_borrowed_texture_descriptor.physical_width(segment, descriptor.physicalWidth)
      mln_opengl_borrowed_texture_descriptor.physical_height(segment, descriptor.physicalHeight)
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
      mln_vulkan_surface_descriptor.surface(segment, descriptor.surface.bits)
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
    mln_opengl_context_descriptor.ownership(segment, context.ownership.nativeValue)
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
    mln_egl_context_descriptor.client_api(segment, context.clientApi.nativeValue)
    mln_egl_context_descriptor.get_proc_address(segment, nativePointer(context.getProcAddress))
  }

  private fun featureStateSelector(arena: Arena, selector: FeatureStateSelector): MemorySegment {
    val segment = arena.allocate(FEATURE_STATE_SELECTOR_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      FEATURE_STATE_SELECTOR_SIZE_OFFSET,
      FEATURE_STATE_SELECTOR_SIZE.toInt(),
    )
    segment
      .asSlice(FEATURE_STATE_SELECTOR_SOURCE_ID_OFFSET, STRING_VIEW_SIZE)
      .copyFrom(stringView(arena, selector.sourceId))
    var fields = 0
    selector.sourceLayerId?.let {
      fields = fields or FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
      segment
        .asSlice(FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID_OFFSET, STRING_VIEW_SIZE)
        .copyFrom(stringView(arena, it))
    }
    selector.featureId?.let {
      fields = fields or FEATURE_STATE_SELECTOR_FEATURE_ID
      segment
        .asSlice(FEATURE_STATE_SELECTOR_FEATURE_ID_OFFSET, STRING_VIEW_SIZE)
        .copyFrom(stringView(arena, it))
    }
    selector.stateKey?.let {
      fields = fields or FEATURE_STATE_SELECTOR_STATE_KEY
      segment
        .asSlice(FEATURE_STATE_SELECTOR_STATE_KEY_OFFSET, STRING_VIEW_SIZE)
        .copyFrom(stringView(arena, it))
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
    value.filterTransit?.let { filter ->
      segment.set(
        ValueLayout.ADDRESS,
        RENDERED_FEATURE_QUERY_OPTIONS_FILTER_OFFSET,
        byteArrayView(arena, filter),
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
    value.filterTransit?.let { filter ->
      segment.set(
        ValueLayout.ADDRESS,
        SOURCE_FEATURE_QUERY_OPTIONS_FILTER_OFFSET,
        byteArrayView(arena, filter),
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
      checkedLong(mln_texture_image_info.byte_length(segment), "texture image byte length"),
    )

  private fun nativePointer(pointer: NativePointer): MemorySegment =
    if (pointer.isNull) MemorySegment.NULL else MemorySegment.ofAddress(pointer.address)

  private fun scopedPointer(pointer: MemorySegment, scope: FrameScope): NativePointer =
    if (pointer == MemorySegment.NULL) NativePointer.NULL_POINTER
    else NativePointer.scoped(pointer.address(), scope)

  internal fun takeOfflineRegionStatusResult(
    runtime: NativeRuntime,
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
        runtimeOfflineRegionStatusTakeResultFunction().invokeNative(runtime, operationId, status)
          as Int
      )
      try {
        offlineRegionStatus(status)
      } finally {
        markTaken()
      }
    }

  internal fun takeCreateOfflineRegionResult(
    runtime: NativeRuntime,
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
    runtime: NativeRuntime,
    operationId: Long,
    markTaken: () -> Unit,
  ): OfflineRegionInfo? =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.JAVA_LONG)
      val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
      outSnapshot.set(ValueLayout.JAVA_LONG, 0, 0L)
      outFound.set(ValueLayout.JAVA_BOOLEAN, 0, false)
      Status.check(
        runtimeOfflineRegionGetTakeResultFunction()
          .invokeNative(runtime, operationId, outSnapshot, outFound) as Int
      )
      if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        markTaken()
        return@use null
      }
      try {
        offlineRegionSnapshot(
          NativeOfflineRegionSnapshot(outSnapshot.get(ValueLayout.JAVA_LONG, 0))
        )
      } finally {
        markTaken()
      }
    }

  internal fun takeOfflineRegionsResult(
    runtime: NativeRuntime,
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
    runtime: NativeRuntime,
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
    runtime: NativeRuntime,
    operationId: Long,
    markTaken: () -> Unit,
  ): OfflineRegionInfo =
    takeOfflineRegionSnapshot(
      runtime,
      operationId,
      runtimeOfflineRegionUpdateMetadataTakeResultFunction(),
      markTaken,
    )

  /** Allocates the reused batch struct that [drainRuntimeEvents] fills. */
  internal fun allocateRuntimeEventBatch(arena: Arena): MemorySegment =
    arena.allocate(RUNTIME_EVENT_BATCH_SIZE)

  /**
   * Drains at most [maxEvents] events into [batch], zero draining every queued event, and copies
   * every field of every event out of the runtime-owned arena before returning.
   */
  internal fun drainRuntimeEvents(
    runtime: NativeRuntime,
    maxEvents: Long,
    batch: MemorySegment,
  ): NativeRuntimeEventBatch {
    batch.set(
      ValueLayout.JAVA_INT,
      RUNTIME_EVENT_BATCH_SIZE_OFFSET,
      RUNTIME_EVENT_BATCH_SIZE.toInt(),
    )
    Status.check(MapLibreNativeC.mln_runtime_drain_events(runtime.raw, maxEvents, batch))
    val eventCount = batch.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_BATCH_EVENT_COUNT_OFFSET)
    val remainingCount = batch.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_BATCH_REMAINING_OFFSET)
    if (eventCount == 0L) {
      return NativeRuntimeEventBatch(emptyList(), remainingCount)
    }
    // The stride the batch reports can exceed this binding's compiled event
    // record, so index by it rather than by mln_runtime_event.sizeof().
    val eventSize =
      Integer.toUnsignedLong(batch.get(ValueLayout.JAVA_INT, RUNTIME_EVENT_BATCH_EVENT_SIZE_OFFSET))
    val payloadExtent = eventSize - RUNTIME_EVENT_PAYLOAD_OFFSET
    val events =
      batch
        .get(ValueLayout.ADDRESS, RUNTIME_EVENT_BATCH_EVENTS_OFFSET)
        .reinterpret(eventCount * eventSize)
    val messagesSize = batch.get(ValueLayout.JAVA_LONG, RUNTIME_EVENT_BATCH_MESSAGES_SIZE_OFFSET)
    val messages =
      batch.get(ValueLayout.ADDRESS, RUNTIME_EVENT_BATCH_MESSAGES_OFFSET).reinterpret(messagesSize)
    return NativeRuntimeEventBatch(
      List(Math.toIntExact(eventCount)) { index ->
        val base = index * eventSize
        val payloadType = events.get(ValueLayout.JAVA_INT, base + RUNTIME_EVENT_PAYLOAD_TYPE_OFFSET)
        NativeRuntimeEvent(
          type = events.get(ValueLayout.JAVA_INT, base + RUNTIME_EVENT_TYPE_OFFSET),
          sourceType = events.get(ValueLayout.JAVA_INT, base + RUNTIME_EVENT_SOURCE_TYPE_OFFSET),
          sourceId = events.get(ValueLayout.JAVA_LONG, base + RUNTIME_EVENT_SOURCE_OFFSET),
          code = events.get(ValueLayout.JAVA_INT, base + RUNTIME_EVENT_CODE_OFFSET),
          payload =
            runtimeEventPayload(
              payloadType,
              events.asSlice(base + RUNTIME_EVENT_PAYLOAD_OFFSET, payloadExtent),
            ),
          message =
            copyMessage(
              messages,
              Integer.toUnsignedLong(
                events.get(ValueLayout.JAVA_INT, base + RUNTIME_EVENT_MESSAGE_OFFSET_OFFSET)
              ),
              Integer.toUnsignedLong(
                events.get(ValueLayout.JAVA_INT, base + RUNTIME_EVENT_MESSAGE_SIZE_OFFSET)
              ),
            ),
        )
      },
      remainingCount,
    )
  }

  internal fun setRuntimeEventMask(runtime: NativeRuntime, mask: Long) {
    Status.check(MapLibreNativeC.mln_runtime_set_event_mask(runtime.raw, mask))
  }

  internal fun runtimeEventMask(runtime: NativeRuntime): Long =
    Arena.ofConfined().use { arena ->
      val outMask = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(MapLibreNativeC.mln_runtime_get_event_mask(runtime.raw, outMask))
      outMask.get(ValueLayout.JAVA_LONG, 0)
    }

  internal fun setMapEventMask(map: NativeMap, mask: Long) {
    Status.check(MapLibreNativeC.mln_map_set_event_mask(map.raw, mask))
  }

  internal fun mapEventMask(map: NativeMap): Long =
    Arena.ofConfined().use { arena ->
      val outMask = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(MapLibreNativeC.mln_map_get_event_mask(map.raw, outMask))
      outMask.get(ValueLayout.JAVA_LONG, 0)
    }

  private fun copyMessage(messages: MemorySegment, offset: Long, size: Long): String {
    if (size == 0L) {
      return ""
    }
    val bytes = ByteArray(Math.toIntExact(size))
    MemorySegment.copy(messages, ValueLayout.JAVA_BYTE, offset, bytes, 0, bytes.size)
    return String(bytes, StandardCharsets.UTF_8)
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

  private fun runtimePumpFunction(): MethodHandle = downcall("mln_runtime_pump")

  private fun runtimeWakeSourceAcquireFunction(): MethodHandle =
    downcall("mln_runtime_wake_source_acquire")

  private fun wakeSourceSignalFunction(): MethodHandle = downcall("mln_wake_source_signal")

  private fun wakeSourceDestroyFunction(): MethodHandle = downcall("mln_wake_source_destroy")

  private fun runtimeAmbientCacheOperationStartFunction(): MethodHandle =
    downcall("mln_runtime_run_ambient_cache_operation_start")

  private fun runtimeSetMaximumAmbientCacheSizeStartFunction(): MethodHandle =
    downcall("mln_runtime_set_maximum_ambient_cache_size_start")

  private fun runtimeOfflineRegionCreateStartFunction(): MethodHandle =
    downcall("mln_runtime_offline_region_create_start")

  private fun runtimeOfflineOperationDiscardFunction(): MethodHandle =
    downcall("mln_runtime_offline_operation_discard")

  private fun runtimeSetResourceProviderFunction(): MethodHandle =
    downcall("mln_runtime_set_resource_provider")

  private fun runtimeClearResourceProviderFunction(): MethodHandle =
    downcall("mln_runtime_clear_resource_provider")

  private fun runtimeSetResourceTransformFunction(): MethodHandle =
    downcall("mln_runtime_set_resource_transform")

  private fun runtimeClearResourceTransformFunction(): MethodHandle =
    downcall("mln_runtime_clear_resource_transform")

  private fun runtimeSetHttpHeaderTransformFunction(): MethodHandle =
    downcall("mln_runtime_set_http_header_transform")

  private fun runtimeClearHttpHeaderTransformFunction(): MethodHandle =
    downcall("mln_runtime_clear_http_header_transform")

  private fun resourceTransformResponseSetUrlFunction(): MethodHandle =
    downcall("mln_resource_transform_response_set_url")

  private fun httpHeaderTransformResponseSetFunction(): MethodHandle =
    downcall("mln_http_header_transform_response_set")

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

  private fun mapStringViewHandleStatusFunction(name: String): MethodHandle = downcall(name)

  private fun mapStringViewBooleanStatusFunction(name: String): MethodHandle = downcall(name)

  private fun geoJsonSourceDataCreateFunction(): MethodHandle =
    downcall("mln_geojson_source_data_create")

  private fun geoJsonSourceDataDestroyFunction(): MethodHandle =
    downcall("mln_geojson_source_data_destroy")

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

  private fun renderSessionRenderUpdateFunction(): MethodHandle =
    downcall("mln_render_session_render_update")

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

  private fun mapStringViewIntStatusFunction(name: String): MethodHandle = downcall(name)

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

  private fun queriedFeatureListCountFunction(): MethodHandle =
    downcall("mln_queried_feature_list_count")

  private fun queriedFeatureListGetFunction(): MethodHandle =
    downcall("mln_queried_feature_list_get")

  private fun queriedFeatureListDestroyFunction(): MethodHandle =
    downcall("mln_queried_feature_list_destroy")

  private fun mapGetStyleSourceTileUrlsFunction(): MethodHandle =
    downcall("mln_map_get_style_source_tile_urls")

  private fun styleStringListCountFunction(): MethodHandle = downcall("mln_style_string_list_count")

  private fun styleStringListGetFunction(): MethodHandle = downcall("mln_style_string_list_get")

  private fun styleStringListDestroyFunction(): MethodHandle =
    downcall("mln_style_string_list_destroy")

  private fun copyStyleSourceAttributionFunction(): MethodHandle =
    downcall("mln_map_copy_style_source_attribution")

  private fun copyStyleSourceUrlFunction(): MethodHandle = downcall("mln_map_copy_style_source_url")

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

  private fun addTileSourceUrl(
    functionName: String,
    map: NativeMap,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoStringViewsAddressStatusFunction(functionName)
          .invokeNative(
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
    map: NativeMap,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    val tileSnapshot = tiles.toList()
    Arena.ofConfined().use { arena ->
      Status.check(
        mapStringViewAddressLongAddressStatusFunction(functionName)
          .invokeNative(
            map,
            stringView(arena, sourceId),
            stringViewArray(arena, tileSnapshot),
            tileSnapshot.size.toLong(),
            tileSourceOptions(arena, options),
          ) as Int
      )
    }
  }

  private fun startRuntimeOperation(name: String, runtime: NativeRuntime): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(runtimeOperationStartFunction(name).invokeNative(runtime, outOperationId) as Int)
      outOperationId.get(ValueLayout.JAVA_LONG, 0)
    }

  private fun startRuntimeLongOperation(name: String, runtime: NativeRuntime, value: Long): Long =
    Arena.ofConfined().use { arena ->
      val outOperationId = arena.allocate(ValueLayout.JAVA_LONG)
      Status.check(
        runtimeLongOperationStartFunction(name).invokeNative(runtime, value, outOperationId) as Int
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

  internal fun defaultRuntimeOptionsEventMask(): Long {
    ensureLoaded()
    return Arena.ofConfined().use { arena ->
      mln_runtime_options.event_mask(MapLibreNativeC.mln_runtime_options_default(arena))
    }
  }

  internal fun defaultMapOptionsEventMask(): Long {
    ensureLoaded()
    return Arena.ofConfined().use { arena ->
      mln_map_options.event_mask(MapLibreNativeC.mln_map_options_default(arena))
    }
  }

  private fun runtimeOptions(options: RuntimeOptions, arena: Arena): MemorySegment {
    val nativeOptions = MapLibreNativeC.mln_runtime_options_default(arena)
    mln_runtime_options.asset_path(nativeOptions, optionalCString(arena, options.assetPath))
    mln_runtime_options.cache_path(nativeOptions, optionalCString(arena, options.cachePath))
    mln_runtime_options.event_mask(nativeOptions, options.eventMask.nativeValue)
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

  private fun byteArrayView(arena: Arena, value: ByteArray): MemorySegment {
    val segment = mln_buffer_view.allocate(arena)
    mln_buffer_view.data(segment, nativeBytes(arena, value))
    mln_buffer_view.size(segment, value.size.toLong())
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

  private fun geoJsonSourceOptions(arena: Arena, value: GeoJsonSourceOptions?): MemorySegment {
    if (value == null) {
      return MemorySegment.NULL
    }
    val segment = arena.allocate(GEOJSON_SOURCE_OPTIONS_SIZE)
    var fields = 0
    segment.set(
      ValueLayout.JAVA_INT,
      GEOJSON_SOURCE_OPTIONS_SIZE_OFFSET,
      GEOJSON_SOURCE_OPTIONS_SIZE.toInt(),
    )
    value.minZoom?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_MIN_ZOOM
      segment.set(ValueLayout.JAVA_DOUBLE, GEOJSON_SOURCE_OPTIONS_MIN_ZOOM_OFFSET, it)
    }
    value.maxZoom?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_MAX_ZOOM
      segment.set(ValueLayout.JAVA_DOUBLE, GEOJSON_SOURCE_OPTIONS_MAX_ZOOM_OFFSET, it)
    }
    value.tolerance?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_TOLERANCE
      segment.set(ValueLayout.JAVA_DOUBLE, GEOJSON_SOURCE_OPTIONS_TOLERANCE_OFFSET, it)
    }
    value.clusterMaxZoom?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
      segment.set(ValueLayout.JAVA_DOUBLE, GEOJSON_SOURCE_OPTIONS_CLUSTER_MAX_ZOOM_OFFSET, it)
    }
    value.clusterPropertiesTransit?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
      segment
        .asSlice(GEOJSON_SOURCE_OPTIONS_CLUSTER_PROPERTIES_OFFSET, BUFFER_VIEW_SIZE)
        .copyFrom(byteArrayView(arena, it))
    }
    value.tileSize?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_TILE_SIZE
      segment.set(ValueLayout.JAVA_INT, GEOJSON_SOURCE_OPTIONS_TILE_SIZE_OFFSET, it)
    }
    value.buffer?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_BUFFER
      segment.set(ValueLayout.JAVA_INT, GEOJSON_SOURCE_OPTIONS_BUFFER_OFFSET, it)
    }
    value.clusterRadius?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
      segment.set(ValueLayout.JAVA_INT, GEOJSON_SOURCE_OPTIONS_CLUSTER_RADIUS_OFFSET, it)
    }
    value.clusterMinPoints?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
      segment.set(ValueLayout.JAVA_INT, GEOJSON_SOURCE_OPTIONS_CLUSTER_MIN_POINTS_OFFSET, it)
    }
    value.lineMetrics?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_LINE_METRICS
      segment.set(ValueLayout.JAVA_BOOLEAN, GEOJSON_SOURCE_OPTIONS_LINE_METRICS_OFFSET, it)
    }
    value.cluster?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_CLUSTER
      segment.set(ValueLayout.JAVA_BOOLEAN, GEOJSON_SOURCE_OPTIONS_CLUSTER_OFFSET, it)
    }
    value.synchronousTiling?.let {
      fields = fields or GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING
      segment.set(ValueLayout.JAVA_BOOLEAN, GEOJSON_SOURCE_OPTIONS_SYNCHRONOUS_TILING_OFFSET, it)
    }
    segment.set(ValueLayout.JAVA_INT, GEOJSON_SOURCE_OPTIONS_FIELDS_OFFSET, fields)
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
    value.transitionId?.let {
      fields = fields or MapLibreNativeC.MLN_ANIMATION_OPTION_TRANSITION_ID()
      mln_animation_options.transition_id(segment, it)
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
    map: NativeMap,
    camera: CameraOptions,
    animation: AnimationOptions?,
  ) {
    Arena.ofConfined().use { arena ->
      Status.check(
        mapTwoAddressStatusFunction(functionName)
          .invokeNative(map, cameraOptions(arena, camera), animationOptions(arena, animation))
          as Int
      )
    }
  }

  private fun mapLatLngBoundsForCamera(
    functionName: String,
    map: NativeMap,
    camera: CameraOptions,
  ): LatLngBounds =
    Arena.ofConfined().use { arena ->
      val outBounds = arena.allocate(latLngBoundsLayout)
      Status.check(
        mapTwoAddressStatusFunction(functionName)
          .invokeNative(map, cameraOptions(arena, camera), outBounds) as Int
      )
      latLngBounds(outBounds)
    }

  private fun boundOptionsDefault(arena: Arena): MemorySegment =
    MapLibreNativeC.mln_bound_options_default(arena)

  private fun boundOptions(arena: Arena, value: BoundOptions): MemorySegment {
    val segment = boundOptionsDefault(arena)
    var fields = 0
    when (val constraint = value.bounds) {
      is BoundsConstraint.Bounded -> {
        fields = fields or MapLibreNativeC.MLN_BOUND_OPTION_BOUNDS()
        mln_bound_options.bounds(segment).copyFrom(latLngBounds(arena, constraint.bounds))
      }
      BoundsConstraint.Unbounded -> {
        fields = fields or MapLibreNativeC.MLN_BOUND_OPTION_UNBOUNDED()
      }
      null -> {}
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
        bounds = BoundsConstraint.Bounded(latLngBounds(mln_bound_options.bounds(segment)))
      } else if ((fields and MapLibreNativeC.MLN_BOUND_OPTION_UNBOUNDED()) != 0) {
        bounds = BoundsConstraint.Unbounded
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
    value?.stretchX?.let {
      fields = fields or STYLE_IMAGE_OPTION_STRETCH_X
      segment.set(
        ValueLayout.ADDRESS,
        STYLE_IMAGE_OPTIONS_STRETCH_X_OFFSET,
        stretchArray(arena, it),
      )
      segment.set(
        ValueLayout.JAVA_LONG,
        STYLE_IMAGE_OPTIONS_STRETCH_X_COUNT_OFFSET,
        it.size.toLong(),
      )
    }
    value?.stretchY?.let {
      fields = fields or STYLE_IMAGE_OPTION_STRETCH_Y
      segment.set(
        ValueLayout.ADDRESS,
        STYLE_IMAGE_OPTIONS_STRETCH_Y_OFFSET,
        stretchArray(arena, it),
      )
      segment.set(
        ValueLayout.JAVA_LONG,
        STYLE_IMAGE_OPTIONS_STRETCH_Y_COUNT_OFFSET,
        it.size.toLong(),
      )
    }
    value?.content?.let {
      fields = fields or STYLE_IMAGE_OPTION_CONTENT
      val content = segment.asSlice(STYLE_IMAGE_OPTIONS_CONTENT_OFFSET, mln_image_content.sizeof())
      content.set(ValueLayout.JAVA_FLOAT, IMAGE_CONTENT_LEFT_OFFSET, it.left)
      content.set(ValueLayout.JAVA_FLOAT, IMAGE_CONTENT_TOP_OFFSET, it.top)
      content.set(ValueLayout.JAVA_FLOAT, IMAGE_CONTENT_RIGHT_OFFSET, it.right)
      content.set(ValueLayout.JAVA_FLOAT, IMAGE_CONTENT_BOTTOM_OFFSET, it.bottom)
    }
    value?.textFitWidth?.let {
      fields = fields or STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
      segment.set(ValueLayout.JAVA_INT, STYLE_IMAGE_OPTIONS_TEXT_FIT_WIDTH_OFFSET, it.nativeValue)
    }
    value?.textFitHeight?.let {
      fields = fields or STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
      segment.set(ValueLayout.JAVA_INT, STYLE_IMAGE_OPTIONS_TEXT_FIT_HEIGHT_OFFSET, it.nativeValue)
    }
    segment.set(ValueLayout.JAVA_INT, STYLE_IMAGE_OPTIONS_FIELDS_OFFSET, fields)
    return segment
  }

  private fun styleTransitionOptionsDefault(arena: Arena): MemorySegment {
    val segment = arena.allocate(STYLE_TRANSITION_OPTIONS_SIZE)
    segment.set(
      ValueLayout.JAVA_INT,
      STYLE_TRANSITION_OPTIONS_SIZE_OFFSET,
      STYLE_TRANSITION_OPTIONS_SIZE.toInt(),
    )
    segment.set(
      ValueLayout.JAVA_BOOLEAN,
      STYLE_TRANSITION_OPTIONS_ENABLE_PLACEMENT_TRANSITIONS_OFFSET,
      true,
    )
    return segment
  }

  private fun styleTransitionOptions(arena: Arena, value: StyleTransitionOptions): MemorySegment {
    val segment = styleTransitionOptionsDefault(arena)
    var fields = 0
    value.enablePlacementTransitions?.let {
      fields = fields or STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
      segment.set(
        ValueLayout.JAVA_BOOLEAN,
        STYLE_TRANSITION_OPTIONS_ENABLE_PLACEMENT_TRANSITIONS_OFFSET,
        it,
      )
    }
    value.durationMs?.let {
      fields = fields or STYLE_TRANSITION_OPTION_DURATION
      segment.set(ValueLayout.JAVA_DOUBLE, STYLE_TRANSITION_OPTIONS_DURATION_MS_OFFSET, it)
    }
    value.delayMs?.let {
      fields = fields or STYLE_TRANSITION_OPTION_DELAY
      segment.set(ValueLayout.JAVA_DOUBLE, STYLE_TRANSITION_OPTIONS_DELAY_MS_OFFSET, it)
    }
    segment.set(ValueLayout.JAVA_INT, STYLE_TRANSITION_OPTIONS_FIELDS_OFFSET, fields)
    return segment
  }

  private fun styleTransitionOptions(segment: MemorySegment): StyleTransitionOptions {
    val fields = segment.get(ValueLayout.JAVA_INT, STYLE_TRANSITION_OPTIONS_FIELDS_OFFSET)
    return StyleTransitionOptions().apply {
      durationMs =
        if (fields and STYLE_TRANSITION_OPTION_DURATION != 0) {
          segment.get(ValueLayout.JAVA_DOUBLE, STYLE_TRANSITION_OPTIONS_DURATION_MS_OFFSET)
        } else {
          null
        }
      delayMs =
        if (fields and STYLE_TRANSITION_OPTION_DELAY != 0) {
          segment.get(ValueLayout.JAVA_DOUBLE, STYLE_TRANSITION_OPTIONS_DELAY_MS_OFFSET)
        } else {
          null
        }
      enablePlacementTransitions =
        if (fields and STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS != 0) {
          segment.get(
            ValueLayout.JAVA_BOOLEAN,
            STYLE_TRANSITION_OPTIONS_ENABLE_PLACEMENT_TRANSITIONS_OFFSET,
          )
        } else {
          null
        }
    }
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
      checkedLong(
        segment.get(ValueLayout.JAVA_LONG, STYLE_IMAGE_INFO_BYTE_LENGTH_OFFSET),
        "style image byte length",
      ),
      segment.get(ValueLayout.JAVA_FLOAT, STYLE_IMAGE_INFO_PIXEL_RATIO_OFFSET),
      segment.get(ValueLayout.JAVA_BOOLEAN, STYLE_IMAGE_INFO_SDF_OFFSET),
      checkedLong(
        segment.get(ValueLayout.JAVA_LONG, STYLE_IMAGE_INFO_STRETCH_X_COUNT_OFFSET),
        "style image stretch X count",
      ),
      checkedLong(
        segment.get(ValueLayout.JAVA_LONG, STYLE_IMAGE_INFO_STRETCH_Y_COUNT_OFFSET),
        "style image stretch Y count",
      ),
      if (segment.get(ValueLayout.JAVA_BOOLEAN, STYLE_IMAGE_INFO_HAS_CONTENT_OFFSET)) {
        val content = segment.asSlice(STYLE_IMAGE_INFO_CONTENT_OFFSET, mln_image_content.sizeof())
        ImageContent(
          content.get(ValueLayout.JAVA_FLOAT, IMAGE_CONTENT_LEFT_OFFSET),
          content.get(ValueLayout.JAVA_FLOAT, IMAGE_CONTENT_TOP_OFFSET),
          content.get(ValueLayout.JAVA_FLOAT, IMAGE_CONTENT_RIGHT_OFFSET),
          content.get(ValueLayout.JAVA_FLOAT, IMAGE_CONTENT_BOTTOM_OFFSET),
        )
      } else null,
      if (segment.get(ValueLayout.JAVA_BOOLEAN, STYLE_IMAGE_INFO_HAS_TEXT_FIT_WIDTH_OFFSET)) {
        StyleImageTextFit.fromNative(
          segment.get(ValueLayout.JAVA_INT, STYLE_IMAGE_INFO_TEXT_FIT_WIDTH_OFFSET)
        )
      } else null,
      if (segment.get(ValueLayout.JAVA_BOOLEAN, STYLE_IMAGE_INFO_HAS_TEXT_FIT_HEIGHT_OFFSET)) {
        StyleImageTextFit.fromNative(
          segment.get(ValueLayout.JAVA_INT, STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT_OFFSET)
        )
      } else null,
    )

  private fun debugOptions(mask: Int): Set<DebugOption> =
    DebugOption.entries.filterTo(mutableSetOf()) { option -> (mask and option.nativeMask) != 0 }

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

  private fun ownedBuffer(buffer: NativeOwnedBuffer): ByteArray? =
    ownedBuffer(
      buffer,
      getter = { handle, bytes -> downcall("mln_buffer_get").invokeNative(handle, bytes) as Int },
      destroyer = { handle -> downcall("mln_buffer_destroy").invokeNative(handle) },
    )

  private fun ownedBuffer(
    buffer: NativeOwnedBuffer,
    getter: (NativeOwnedBuffer, MemorySegment) -> Int,
    destroyer: (NativeOwnedBuffer) -> Unit,
  ): ByteArray? {
    if (buffer.isNull) return null
    return try {
      Arena.ofConfined().use { arena ->
        val bytes = mln_buffer_view.allocate(arena)
        Status.check(getter(buffer, bytes))
        copyBytes(mln_buffer_view.data(bytes), mln_buffer_view.size(bytes))
      }
    } finally {
      destroyer(buffer)
    }
  }

  private fun copyString(address: MemorySegment, byteCount: Long): String =
    String(copyBytes(address, byteCount), StandardCharsets.UTF_8)

  private fun checkedInt(value: Long): Int {
    require(value <= Int.MAX_VALUE) { "native count exceeds Int.MAX_VALUE" }
    require(value >= 0L) { "native count must be non-negative" }
    return value.toInt()
  }

  private fun checkedLong(value: Long, name: String): Long {
    require(value >= 0L) { "$name exceeds Long.MAX_VALUE" }
    return value
  }

  private fun stringView(segment: MemorySegment): String =
    copyString(
      segment.get(ValueLayout.ADDRESS, STRING_VIEW_DATA_OFFSET),
      segment.get(ValueLayout.JAVA_LONG, STRING_VIEW_SIZE_OFFSET),
    )

  internal fun resourceRequest(request: MemorySegment): ResourceRequest =
    ResourceRequest(
      copyCString(request.get(ValueLayout.ADDRESS, RESOURCE_REQUEST_REQUESTED_URL_OFFSET)),
      copyCString(request.get(ValueLayout.ADDRESS, RESOURCE_REQUEST_RESOLVED_URL_OFFSET)),
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
    options.fastPforEnabled?.let { mln_map_options.fast_pfor_enabled(segment, it) }
    mln_map_options.event_mask(segment, options.eventMask.nativeValue)
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
    segment
      .asSlice(OFFLINE_GEOMETRY_DEFINITION_GEOMETRY_OFFSET, BUFFER_VIEW_SIZE)
      .copyFrom(byteArrayView(arena, value.geometryTransit))
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

  private fun offlineRegionSnapshot(snapshot: NativeOfflineRegionSnapshot): OfflineRegionInfo =
    try {
      Arena.ofConfined().use { arena ->
        val info = arena.allocate(OFFLINE_REGION_INFO_SIZE)
        info.set(
          ValueLayout.JAVA_INT,
          OFFLINE_REGION_INFO_SIZE_OFFSET,
          OFFLINE_REGION_INFO_SIZE.toInt(),
        )
        Status.check(offlineRegionSnapshotGetFunction().invokeNative(snapshot, info) as Int)
        offlineRegionInfo(info)
      }
    } finally {
      offlineRegionSnapshotDestroyFunction().invokeNative(snapshot)
    }

  private fun offlineRegionList(list: NativeOfflineRegionList): List<OfflineRegionInfo> =
    offlineRegionList(
      list,
      counter = { handle, outCount ->
        offlineRegionListCountFunction().invokeNative(handle, outCount) as Int
      },
      getter = { handle, index, outInfo ->
        offlineRegionListGetFunction().invokeNative(handle, index, outInfo) as Int
      },
      destroyer = { handle -> offlineRegionListDestroyFunction().invokeNative(handle) },
    )

  private fun offlineRegionList(
    list: NativeOfflineRegionList,
    counter: (NativeOfflineRegionList, MemorySegment) -> Int,
    getter: (NativeOfflineRegionList, Long, MemorySegment) -> Int,
    destroyer: (NativeOfflineRegionList) -> Unit,
  ): List<OfflineRegionInfo> =
    try {
      Arena.ofConfined().use { arena ->
        val outCount = arena.allocate(ValueLayout.JAVA_LONG)
        Status.check(counter(list, outCount))
        val count = Math.toIntExact(outCount.get(ValueLayout.JAVA_LONG, 0))
        List(count) { index ->
          val info = arena.allocate(OFFLINE_REGION_INFO_SIZE)
          info.set(
            ValueLayout.JAVA_INT,
            OFFLINE_REGION_INFO_SIZE_OFFSET,
            OFFLINE_REGION_INFO_SIZE.toInt(),
          )
          Status.check(getter(list, index.toLong(), info))
          offlineRegionInfo(info)
        }
      }
    } finally {
      destroyer(list)
    }

  private fun styleIdList(list: NativeStyleIdList): List<String> =
    try {
      Arena.ofConfined().use { arena ->
        val outCount = arena.allocate(ValueLayout.JAVA_LONG)
        Status.check(styleIdListCountFunction().invokeNative(list, outCount) as Int)
        val count = Math.toIntExact(outCount.get(ValueLayout.JAVA_LONG, 0))
        List(count) { index ->
          val outId = arena.allocate(STRING_VIEW_SIZE)
          Status.check(styleIdListGetFunction().invokeNative(list, index.toLong(), outId) as Int)
          stringView(outId)
        }
      }
    } finally {
      styleIdListDestroyFunction().invokeNative(list)
    }

  private fun queriedFeatureList(list: NativeQueriedFeatureList): List<QueriedFeature> =
    try {
      check(!list.isNull) { "mln_queried_feature_list returned the null handle" }
      Arena.ofConfined().use { arena ->
        val outCount = arena.allocate(ValueLayout.JAVA_LONG)
        Status.check(queriedFeatureListCountFunction().invokeNative(list, outCount) as Int)
        val count = Math.toIntExact(outCount.get(ValueLayout.JAVA_LONG, 0))
        List(count) { index ->
          val outFeature = mln_queried_feature.allocate(arena)
          mln_queried_feature.size(outFeature, mln_queried_feature.sizeof().toInt())
          Status.check(
            queriedFeatureListGetFunction().invokeNative(list, index.toLong(), outFeature) as Int
          )
          queriedFeature(outFeature)
        }
      }
    } finally {
      queriedFeatureListDestroyFunction().invokeNative(list)
    }

  private fun queriedFeature(segment: MemorySegment): QueriedFeature {
    val fields = mln_queried_feature.fields(segment)
    return QueriedFeature(
      copyBufferView(mln_queried_feature.feature(segment)),
      if (fields and QUERIED_FEATURE_SOURCE_ID != 0)
        stringView(mln_queried_feature.source_id(segment))
      else null,
      if (fields and QUERIED_FEATURE_SOURCE_LAYER_ID != 0)
        stringView(mln_queried_feature.source_layer_id(segment))
      else null,
      if (fields and QUERIED_FEATURE_STATE != 0) copyBufferView(mln_queried_feature.state(segment))
      else null,
    )
  }

  private fun copyBufferView(view: MemorySegment): ByteArray =
    copyBytes(mln_buffer_view.data(view), mln_buffer_view.size(view))

  private fun styleSourceTileUrls(
    map: NativeMap,
    sourceId: MemorySegment,
    arena: Arena,
  ): List<String> {
    val outList = arena.allocate(ValueLayout.JAVA_LONG)
    outList.set(ValueLayout.JAVA_LONG, 0, 0L)
    val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
    Status.check(
      mapGetStyleSourceTileUrlsFunction().invokeNative(map, sourceId, outList, outFound) as Int
    )
    check(outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
      "style source disappeared while its metadata was copied"
    }
    val list = NativeStyleStringList(outList.get(ValueLayout.JAVA_LONG, 0))
    check(!list.isNull) { "mln_map_get_style_source_tile_urls returned the null handle" }
    return styleStringList(list)
  }

  private fun styleStringList(list: NativeStyleStringList): List<String> =
    styleStringList(
      list,
      counter = { handle, outCount ->
        styleStringListCountFunction().invokeNative(handle, outCount) as Int
      },
      getter = { handle, index, outValue ->
        styleStringListGetFunction().invokeNative(handle, index, outValue) as Int
      },
      destroyer = { handle -> styleStringListDestroyFunction().invokeNative(handle) },
    )

  private fun styleStringList(
    list: NativeStyleStringList,
    counter: (NativeStyleStringList, MemorySegment) -> Int,
    getter: (NativeStyleStringList, Long, MemorySegment) -> Int,
    destroyer: (NativeStyleStringList) -> Unit,
  ): List<String> =
    try {
      Arena.ofConfined().use { arena ->
        val outCount = arena.allocate(ValueLayout.JAVA_LONG)
        Status.check(counter(list, outCount))
        val count = Math.toIntExact(outCount.get(ValueLayout.JAVA_LONG, 0))
        List(count) { index ->
          val outValue = arena.allocate(STRING_VIEW_SIZE)
          Status.check(getter(list, index.toLong(), outValue))
          stringView(outValue)
        }
      }
    } finally {
      destroyer(list)
    }

  private fun copyStyleSourceAttribution(
    map: NativeMap,
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
        .invokeNative(map, sourceId, outAttribution, attributionSize, outAttributionSize, outFound)
        as Int
    )
    if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
      return null
    }
    return copyString(outAttribution, outAttributionSize.get(ValueLayout.JAVA_LONG, 0))
  }

  private fun copyStyleSourceUrl(
    map: NativeMap,
    sourceId: MemorySegment,
    urlSize: Long,
    arena: Arena,
  ): String? {
    val outUrl = if (urlSize == 0L) MemorySegment.NULL else arena.allocate(urlSize)
    val outUrlSize = arena.allocate(ValueLayout.JAVA_LONG)
    val outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN)
    Status.check(
      copyStyleSourceUrlFunction()
        .invokeNative(map, sourceId, outUrl, urlSize, outUrlSize, outFound) as Int
    )
    if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) return null
    return if (urlSize == 0L) "" else copyString(outUrl, outUrlSize.get(ValueLayout.JAVA_LONG, 0))
  }

  private fun takeOfflineRegionSnapshot(
    runtime: NativeRuntime,
    operationId: Long,
    function: MethodHandle,
    markTaken: () -> Unit,
  ): OfflineRegionInfo =
    Arena.ofConfined().use { arena ->
      val outSnapshot = arena.allocate(ValueLayout.JAVA_LONG)
      outSnapshot.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(function.invokeNative(runtime, operationId, outSnapshot) as Int)
      try {
        offlineRegionSnapshot(
          NativeOfflineRegionSnapshot(outSnapshot.get(ValueLayout.JAVA_LONG, 0))
        )
      } finally {
        markTaken()
      }
    }

  private fun takeOfflineRegionList(
    runtime: NativeRuntime,
    operationId: Long,
    function: MethodHandle,
    markTaken: () -> Unit,
  ): List<OfflineRegionInfo> =
    Arena.ofConfined().use { arena ->
      val outList = arena.allocate(ValueLayout.JAVA_LONG)
      outList.set(ValueLayout.JAVA_LONG, 0, 0L)
      Status.check(function.invokeNative(runtime, operationId, outList) as Int)
      try {
        offlineRegionList(NativeOfflineRegionList(outList.get(ValueLayout.JAVA_LONG, 0)))
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
      copyBytes(
        mln_buffer_view.data(
          segment.asSlice(OFFLINE_GEOMETRY_DEFINITION_GEOMETRY_OFFSET, BUFFER_VIEW_SIZE)
        ),
        mln_buffer_view.size(
          segment.asSlice(OFFLINE_GEOMETRY_DEFINITION_GEOMETRY_OFFSET, BUFFER_VIEW_SIZE)
        ),
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

  /** Decodes one payload window, for tests that synthesize a payload this version cannot queue. */
  internal fun runtimeEventPayloadForTesting(
    payloadType: Int,
    payload: MemorySegment,
  ): RuntimeEventPayload = runtimeEventPayload(payloadType, payload)

  private fun runtimeEventPayload(payloadType: Int, payload: MemorySegment): RuntimeEventPayload =
    when (payloadType) {
      PAYLOAD_NONE -> RuntimeEventPayload.None
      PAYLOAD_RENDER_FRAME -> renderFramePayload(payload)
      PAYLOAD_RENDER_MAP -> renderMapPayload(payload)
      PAYLOAD_TILE_ACTION -> tileActionPayload(payload)
      PAYLOAD_OFFLINE_REGION_STATUS -> offlineRegionStatusPayload(payload)
      PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR -> offlineRegionResponseErrorPayload(payload)
      PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT -> offlineRegionTileCountLimitPayload(payload)
      PAYLOAD_OFFLINE_OPERATION_COMPLETED -> offlineOperationCompletedPayload(payload)
      PAYLOAD_CAMERA_TRANSITION_FINISHED -> cameraTransitionFinishedPayload(payload)
      else -> unknownPayload(payloadType, payload)
    }

  private fun unknownPayload(
    payloadType: Int,
    payload: MemorySegment,
  ): RuntimeEventPayload.Unknown =
    RuntimeEventPayload.Unknown(payloadType, copyBytes(payload, payload.byteSize()))

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

  private fun cameraTransitionFinishedPayload(
    payload: MemorySegment
  ): RuntimeEventPayload.CameraTransitionFinished =
    RuntimeEventPayload.CameraTransitionFinished(
      payload.get(
        ValueLayout.JAVA_LONG,
        RUNTIME_EVENT_CAMERA_TRANSITION_FINISHED_TRANSITION_ID_OFFSET,
      )
    )

  /**
   * Invokes a downcall, passing each handle as the integer the C API declares.
   *
   * `MethodHandle.invokeWithArguments` is untyped, so an unconverted handle wrapper would fail only
   * at run time.
   */
  private fun MethodHandle.invokeNative(vararg args: Any?): Any? =
    invokeWithArguments(args.map { if (it is NativeHandle) it.raw else it })

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
  private val stringViewLayout = mln_buffer_view.layout()

  private val STRING_VIEW_SIZE: Long = mln_buffer_view.sizeof()
  private val STRING_VIEW_DATA_OFFSET: Long = mln_buffer_view.`data$offset`()
  private val STRING_VIEW_SIZE_OFFSET: Long = mln_buffer_view.`size$offset`()
  private val BUFFER_VIEW_SIZE: Long = mln_buffer_view.sizeof()

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

  private val CANONICAL_TILE_ID_SIZE: Long = mln_canonical_tile_id.sizeof()
  private val CANONICAL_TILE_ID_Z_OFFSET: Long = mln_canonical_tile_id.`z$offset`()
  private val CANONICAL_TILE_ID_X_OFFSET: Long = mln_canonical_tile_id.`x$offset`()
  private val CANONICAL_TILE_ID_Y_OFFSET: Long = mln_canonical_tile_id.`y$offset`()

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
  private const val QUERIED_FEATURE_SOURCE_ID: Int = 1 shl 0
  private const val QUERIED_FEATURE_SOURCE_LAYER_ID: Int = 1 shl 1
  private const val QUERIED_FEATURE_STATE: Int = 1 shl 2

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

  private val STYLE_SOURCE_INFO_SIZE: Long = mln_style_source_info.sizeof()
  private val STYLE_SOURCE_INFO_SIZE_OFFSET: Long = mln_style_source_info.`size$offset`()
  private val STYLE_SOURCE_INFO_TYPE_OFFSET: Long = mln_style_source_info.`type$offset`()
  private val STYLE_SOURCE_INFO_FIELDS_OFFSET: Long = mln_style_source_info.`fields$offset`()
  private val STYLE_SOURCE_INFO_IS_VOLATILE_OFFSET: Long =
    mln_style_source_info.`is_volatile$offset`()
  private val STYLE_SOURCE_INFO_HAS_ATTRIBUTION_OFFSET: Long =
    mln_style_source_info.`has_attribution$offset`()
  private val STYLE_SOURCE_INFO_ATTRIBUTION_SIZE_OFFSET: Long =
    mln_style_source_info.`attribution_size$offset`()
  private val STYLE_SOURCE_INFO_URL_SIZE_OFFSET: Long = mln_style_source_info.`url_size$offset`()
  private val STYLE_SOURCE_INFO_MIN_ZOOM_OFFSET: Long = mln_style_source_info.`min_zoom$offset`()
  private val STYLE_SOURCE_INFO_MAX_ZOOM_OFFSET: Long = mln_style_source_info.`max_zoom$offset`()
  private val STYLE_SOURCE_INFO_SCHEME_OFFSET: Long = mln_style_source_info.`scheme$offset`()
  private val STYLE_SOURCE_INFO_BOUNDS_OFFSET: Long = mln_style_source_info.`bounds$offset`()
  private val STYLE_SOURCE_INFO_TILE_SIZE_OFFSET: Long = mln_style_source_info.`tile_size$offset`()
  private val STYLE_SOURCE_INFO_VECTOR_ENCODING_OFFSET: Long =
    mln_style_source_info.`vector_encoding$offset`()
  private val STYLE_SOURCE_INFO_RASTER_ENCODING_OFFSET: Long =
    mln_style_source_info.`raster_encoding$offset`()

  private const val STYLE_SOURCE_INFO_URL: Int = 1 shl 0
  private const val STYLE_SOURCE_INFO_TILEJSON: Int = 1 shl 1
  private const val STYLE_SOURCE_INFO_BOUNDS: Int = 1 shl 2
  private const val STYLE_SOURCE_INFO_TILE_SIZE: Int = 1 shl 3
  private const val STYLE_SOURCE_INFO_VECTOR_ENCODING: Int = 1 shl 4
  private const val STYLE_SOURCE_INFO_RASTER_ENCODING: Int = 1 shl 5

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

  private const val GEOJSON_SOURCE_OPTION_MIN_ZOOM: Int = 1 shl 0
  private const val GEOJSON_SOURCE_OPTION_MAX_ZOOM: Int = 1 shl 1
  private const val GEOJSON_SOURCE_OPTION_TOLERANCE: Int = 1 shl 2
  private const val GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM: Int = 1 shl 3
  private const val GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES: Int = 1 shl 4
  private const val GEOJSON_SOURCE_OPTION_TILE_SIZE: Int = 1 shl 5
  private const val GEOJSON_SOURCE_OPTION_BUFFER: Int = 1 shl 6
  private const val GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS: Int = 1 shl 7
  private const val GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS: Int = 1 shl 8
  private const val GEOJSON_SOURCE_OPTION_LINE_METRICS: Int = 1 shl 9
  private const val GEOJSON_SOURCE_OPTION_CLUSTER: Int = 1 shl 10
  private const val GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING: Int = 1 shl 11

  private val GEOJSON_SOURCE_OPTIONS_SIZE: Long = mln_geojson_source_options.sizeof()
  private val GEOJSON_SOURCE_OPTIONS_SIZE_OFFSET: Long = mln_geojson_source_options.`size$offset`()
  private val GEOJSON_SOURCE_OPTIONS_FIELDS_OFFSET: Long =
    mln_geojson_source_options.`fields$offset`()
  private val GEOJSON_SOURCE_OPTIONS_MIN_ZOOM_OFFSET: Long =
    mln_geojson_source_options.`min_zoom$offset`()
  private val GEOJSON_SOURCE_OPTIONS_MAX_ZOOM_OFFSET: Long =
    mln_geojson_source_options.`max_zoom$offset`()
  private val GEOJSON_SOURCE_OPTIONS_TOLERANCE_OFFSET: Long =
    mln_geojson_source_options.`tolerance$offset`()
  private val GEOJSON_SOURCE_OPTIONS_CLUSTER_MAX_ZOOM_OFFSET: Long =
    mln_geojson_source_options.`cluster_max_zoom$offset`()
  private val GEOJSON_SOURCE_OPTIONS_CLUSTER_PROPERTIES_OFFSET: Long =
    mln_geojson_source_options.`cluster_properties$offset`()
  private val GEOJSON_SOURCE_OPTIONS_TILE_SIZE_OFFSET: Long =
    mln_geojson_source_options.`tile_size$offset`()
  private val GEOJSON_SOURCE_OPTIONS_BUFFER_OFFSET: Long =
    mln_geojson_source_options.`buffer$offset`()
  private val GEOJSON_SOURCE_OPTIONS_CLUSTER_RADIUS_OFFSET: Long =
    mln_geojson_source_options.`cluster_radius$offset`()
  private val GEOJSON_SOURCE_OPTIONS_CLUSTER_MIN_POINTS_OFFSET: Long =
    mln_geojson_source_options.`cluster_min_points$offset`()
  private val GEOJSON_SOURCE_OPTIONS_LINE_METRICS_OFFSET: Long =
    mln_geojson_source_options.`line_metrics$offset`()
  private val GEOJSON_SOURCE_OPTIONS_CLUSTER_OFFSET: Long =
    mln_geojson_source_options.`cluster$offset`()
  private val GEOJSON_SOURCE_OPTIONS_SYNCHRONOUS_TILING_OFFSET: Long =
    mln_geojson_source_options.`synchronous_tiling$offset`()

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
  private val CUSTOM_GEOMETRY_SOURCE_OPTIONS_RELEASE_USER_DATA_OFFSET: Long =
    mln_custom_geometry_source_options.`release_user_data$offset`()

  private const val CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM: Int = 1 shl 0
  private const val CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM: Int = 1 shl 1

  private val CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_SIZE: Long =
    mln_custom_mvt_vector_source_options.sizeof()
  private val CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_SIZE_OFFSET: Long =
    mln_custom_mvt_vector_source_options.`size$offset`()
  private val CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_FIELDS_OFFSET: Long =
    mln_custom_mvt_vector_source_options.`fields$offset`()
  private val CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_FETCH_TILE_OFFSET: Long =
    mln_custom_mvt_vector_source_options.`fetch_tile$offset`()
  private val CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_CANCEL_TILE_OFFSET: Long =
    mln_custom_mvt_vector_source_options.`cancel_tile$offset`()
  private val CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_USER_DATA_OFFSET: Long =
    mln_custom_mvt_vector_source_options.`user_data$offset`()
  private val CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_MIN_ZOOM_OFFSET: Long =
    mln_custom_mvt_vector_source_options.`min_zoom$offset`()
  private val CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_MAX_ZOOM_OFFSET: Long =
    mln_custom_mvt_vector_source_options.`max_zoom$offset`()
  private val CUSTOM_MVT_VECTOR_SOURCE_OPTIONS_RELEASE_USER_DATA_OFFSET: Long =
    mln_custom_mvt_vector_source_options.`release_user_data$offset`()

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
  private const val STYLE_IMAGE_OPTION_STRETCH_X: Int = 1 shl 2
  private const val STYLE_IMAGE_OPTION_STRETCH_Y: Int = 1 shl 3
  private const val STYLE_IMAGE_OPTION_CONTENT: Int = 1 shl 4
  private const val STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH: Int = 1 shl 5
  private const val STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT: Int = 1 shl 6

  private const val DEFAULT_PIXEL_RATIO: Float = 1.0f

  private val STYLE_IMAGE_OPTIONS_SIZE: Long = mln_style_image_options.sizeof()
  private val STYLE_IMAGE_OPTIONS_SIZE_OFFSET: Long = mln_style_image_options.`size$offset`()
  private val STYLE_IMAGE_OPTIONS_FIELDS_OFFSET: Long = mln_style_image_options.`fields$offset`()
  private val STYLE_IMAGE_OPTIONS_PIXEL_RATIO_OFFSET: Long =
    mln_style_image_options.`pixel_ratio$offset`()
  private val STYLE_IMAGE_OPTIONS_SDF_OFFSET: Long = mln_style_image_options.`sdf$offset`()
  private val STYLE_IMAGE_OPTIONS_STRETCH_X_OFFSET: Long =
    mln_style_image_options.`stretch_x$offset`()
  private val STYLE_IMAGE_OPTIONS_STRETCH_X_COUNT_OFFSET: Long =
    mln_style_image_options.`stretch_x_count$offset`()
  private val STYLE_IMAGE_OPTIONS_STRETCH_Y_OFFSET: Long =
    mln_style_image_options.`stretch_y$offset`()
  private val STYLE_IMAGE_OPTIONS_STRETCH_Y_COUNT_OFFSET: Long =
    mln_style_image_options.`stretch_y_count$offset`()
  private val STYLE_IMAGE_OPTIONS_CONTENT_OFFSET: Long = mln_style_image_options.`content$offset`()
  private val STYLE_IMAGE_OPTIONS_TEXT_FIT_WIDTH_OFFSET: Long =
    mln_style_image_options.`text_fit_width$offset`()
  private val STYLE_IMAGE_OPTIONS_TEXT_FIT_HEIGHT_OFFSET: Long =
    mln_style_image_options.`text_fit_height$offset`()

  private const val STYLE_TRANSITION_OPTION_DURATION: Int = 1 shl 0
  private const val STYLE_TRANSITION_OPTION_DELAY: Int = 1 shl 1
  private const val STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS: Int = 1 shl 2

  private val STYLE_TRANSITION_OPTIONS_SIZE: Long = mln_style_transition_options.sizeof()
  private val STYLE_TRANSITION_OPTIONS_SIZE_OFFSET: Long =
    mln_style_transition_options.`size$offset`()
  private val STYLE_TRANSITION_OPTIONS_FIELDS_OFFSET: Long =
    mln_style_transition_options.`fields$offset`()
  private val STYLE_TRANSITION_OPTIONS_DURATION_MS_OFFSET: Long =
    mln_style_transition_options.`duration_ms$offset`()
  private val STYLE_TRANSITION_OPTIONS_DELAY_MS_OFFSET: Long =
    mln_style_transition_options.`delay_ms$offset`()
  private val STYLE_TRANSITION_OPTIONS_ENABLE_PLACEMENT_TRANSITIONS_OFFSET: Long =
    mln_style_transition_options.`enable_placement_transitions$offset`()

  private val IMAGE_STRETCH_SIZE: Long = mln_image_stretch.sizeof()
  private val IMAGE_STRETCH_FROM_OFFSET: Long = mln_image_stretch.`from$offset`()
  private val IMAGE_STRETCH_TO_OFFSET: Long = mln_image_stretch.`to$offset`()
  private val IMAGE_CONTENT_LEFT_OFFSET: Long = mln_image_content.`left$offset`()
  private val IMAGE_CONTENT_TOP_OFFSET: Long = mln_image_content.`top$offset`()
  private val IMAGE_CONTENT_RIGHT_OFFSET: Long = mln_image_content.`right$offset`()
  private val IMAGE_CONTENT_BOTTOM_OFFSET: Long = mln_image_content.`bottom$offset`()

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
  private val STYLE_IMAGE_INFO_STRETCH_X_COUNT_OFFSET: Long =
    mln_style_image_info.`stretch_x_count$offset`()
  private val STYLE_IMAGE_INFO_STRETCH_Y_COUNT_OFFSET: Long =
    mln_style_image_info.`stretch_y_count$offset`()
  private val STYLE_IMAGE_INFO_CONTENT_OFFSET: Long = mln_style_image_info.`content$offset`()
  private val STYLE_IMAGE_INFO_TEXT_FIT_WIDTH_OFFSET: Long =
    mln_style_image_info.`text_fit_width$offset`()
  private val STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT_OFFSET: Long =
    mln_style_image_info.`text_fit_height$offset`()
  private val STYLE_IMAGE_INFO_HAS_CONTENT_OFFSET: Long =
    mln_style_image_info.`has_content$offset`()
  private val STYLE_IMAGE_INFO_HAS_TEXT_FIT_WIDTH_OFFSET: Long =
    mln_style_image_info.`has_text_fit_width$offset`()
  private val STYLE_IMAGE_INFO_HAS_TEXT_FIT_HEIGHT_OFFSET: Long =
    mln_style_image_info.`has_text_fit_height$offset`()

  private val RUNTIME_EVENT_TYPE_OFFSET: Long = mln_runtime_event.`type$offset`()
  private val RUNTIME_EVENT_SOURCE_TYPE_OFFSET: Long = mln_runtime_event.`source_type$offset`()
  private val RUNTIME_EVENT_SOURCE_OFFSET: Long = mln_runtime_event.`source$offset`()
  private val RUNTIME_EVENT_CODE_OFFSET: Long = mln_runtime_event.`code$offset`()
  private val RUNTIME_EVENT_PAYLOAD_TYPE_OFFSET: Long = mln_runtime_event.`payload_type$offset`()
  private val RUNTIME_EVENT_PAYLOAD_OFFSET: Long = mln_runtime_event.`payload$offset`()
  private val RUNTIME_EVENT_MESSAGE_OFFSET_OFFSET: Long =
    mln_runtime_event.`message_offset$offset`()
  private val RUNTIME_EVENT_MESSAGE_SIZE_OFFSET: Long = mln_runtime_event.`message_size$offset`()

  private val RUNTIME_EVENT_BATCH_SIZE: Long = mln_runtime_event_batch.sizeof()
  private val RUNTIME_EVENT_BATCH_SIZE_OFFSET: Long = mln_runtime_event_batch.`size$offset`()
  private val RUNTIME_EVENT_BATCH_EVENT_SIZE_OFFSET: Long =
    mln_runtime_event_batch.`event_size$offset`()
  private val RUNTIME_EVENT_BATCH_EVENTS_OFFSET: Long = mln_runtime_event_batch.`events$offset`()
  private val RUNTIME_EVENT_BATCH_EVENT_COUNT_OFFSET: Long =
    mln_runtime_event_batch.`event_count$offset`()
  private val RUNTIME_EVENT_BATCH_MESSAGES_OFFSET: Long =
    mln_runtime_event_batch.`messages$offset`()
  private val RUNTIME_EVENT_BATCH_MESSAGES_SIZE_OFFSET: Long =
    mln_runtime_event_batch.`messages_size$offset`()
  private val RUNTIME_EVENT_BATCH_REMAINING_OFFSET: Long =
    mln_runtime_event_batch.`remaining_count$offset`()

  private val RESOURCE_REQUEST_REQUESTED_URL_OFFSET: Long =
    mln_resource_request.`requested_url$offset`()
  private val RESOURCE_REQUEST_RESOLVED_URL_OFFSET: Long =
    mln_resource_request.`resolved_url$offset`()
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

  private val PAYLOAD_NONE: Int = MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_NONE()
  private val PAYLOAD_RENDER_FRAME: Int = MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME()
  private val PAYLOAD_RENDER_MAP: Int = MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP()
  private val PAYLOAD_TILE_ACTION: Int = MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION()
  private val PAYLOAD_OFFLINE_REGION_STATUS: Int =
    MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS()
  private val PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR: Int =
    MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR()
  private val PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT: Int =
    MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT()
  private val PAYLOAD_OFFLINE_OPERATION_COMPLETED: Int =
    MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED()
  private val PAYLOAD_CAMERA_TRANSITION_FINISHED: Int =
    MapLibreNativeC.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED()

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

  private val RUNTIME_EVENT_RENDER_MAP_MODE_OFFSET: Long =
    mln_runtime_event_render_map.`mode$offset`()

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

  private val RUNTIME_EVENT_OFFLINE_REGION_STATUS_REGION_ID_OFFSET: Long =
    mln_runtime_event_offline_region_status.`region_id$offset`()
  private val RUNTIME_EVENT_OFFLINE_REGION_STATUS_STATUS_OFFSET: Long =
    mln_runtime_event_offline_region_status.`status$offset`()

  private val RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR_REGION_ID_OFFSET: Long =
    mln_runtime_event_offline_region_response_error.`region_id$offset`()
  private val RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR_REASON_OFFSET: Long =
    mln_runtime_event_offline_region_response_error.`reason$offset`()

  private val RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_REGION_ID_OFFSET: Long =
    mln_runtime_event_offline_region_tile_count_limit.`region_id$offset`()
  private val RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_LIMIT_OFFSET: Long =
    mln_runtime_event_offline_region_tile_count_limit.`limit$offset`()

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

  private val RUNTIME_EVENT_CAMERA_TRANSITION_FINISHED_TRANSITION_ID_OFFSET: Long =
    mln_runtime_event_camera_transition_finished.`transition_id$offset`()

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

  /** Test seam for the handwritten JVM FFM materializers and readers. */
  internal object JvmStructs {
    fun stringViewRoundTrip(value: String): String =
      Arena.ofConfined().use { arena -> stringView(stringView(arena, value)) }

    fun cameraOptionsRoundTrip(value: CameraOptions): Pair<Int, CameraOptions> =
      Arena.ofConfined().use { arena ->
        val native = cameraOptions(arena, value)
        mln_camera_options.fields(native) to cameraOptions(native)
      }

    fun animationOptionsSnapshot(value: AnimationOptions): AnimationOptionsSnapshot =
      Arena.ofConfined().use { arena ->
        val native = animationOptions(arena, value)
        AnimationOptionsSnapshot(
          mln_animation_options.fields(native),
          mln_animation_options.duration_ms(native),
          mln_animation_options.velocity(native),
          mln_animation_options.transition_id(native),
        )
      }

    fun viewportOptionsRoundTrip(value: ViewportOptions): ViewportOptions =
      Arena.ofConfined().use { arena -> readViewportOptions(viewportOptions(arena, value)) }

    fun tileOptionsRoundTrip(value: TileOptions): TileOptions =
      Arena.ofConfined().use { arena -> readTileOptions(tileOptions(arena, value)) }

    fun projectionModeOptionsRoundTrip(value: ProjectionModeOptions): ProjectionModeOptions =
      Arena.ofConfined().use { arena -> projectionModeOptions(projectionModeOptions(arena, value)) }

    fun renderedQueryGeometryType(value: RenderedQueryGeometry): Int =
      Arena.ofConfined().use { arena ->
        val native = renderedQueryGeometry(arena, value)
        native.get(ValueLayout.JAVA_INT, RENDERED_QUERY_GEOMETRY_TYPE_OFFSET)
      }

    fun featureStateSelectorSnapshot(value: FeatureStateSelector): FeatureStateSelectorSnapshot =
      Arena.ofConfined().use { arena ->
        val native = featureStateSelector(arena, value)
        val fields = native.get(ValueLayout.JAVA_INT, FEATURE_STATE_SELECTOR_FIELDS_OFFSET)
        FeatureStateSelectorSnapshot(
          fields,
          stringView(native.asSlice(FEATURE_STATE_SELECTOR_SOURCE_ID_OFFSET, STRING_VIEW_SIZE)),
          if ((fields and FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID) != 0)
            stringView(
              native.asSlice(FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID_OFFSET, STRING_VIEW_SIZE)
            )
          else null,
          if ((fields and FEATURE_STATE_SELECTOR_FEATURE_ID) != 0)
            stringView(native.asSlice(FEATURE_STATE_SELECTOR_FEATURE_ID_OFFSET, STRING_VIEW_SIZE))
          else null,
          if ((fields and FEATURE_STATE_SELECTOR_STATE_KEY) != 0)
            stringView(native.asSlice(FEATURE_STATE_SELECTOR_STATE_KEY_OFFSET, STRING_VIEW_SIZE))
          else null,
        )
      }

    data class FeatureStateSelectorSnapshot(
      val fields: Int,
      val sourceId: String,
      val sourceLayerId: String?,
      val featureId: String?,
      val stateKey: String?,
    )

    fun offlineRegionDefinitionRoundTrip(value: OfflineRegionDefinition): OfflineRegionDefinition =
      Arena.ofConfined().use { arena ->
        offlineRegionDefinition(offlineRegionDefinition(value, arena))
      }

    fun offlineRegionInfoSnapshot(
      id: Long,
      definition: OfflineRegionDefinition,
      metadata: ByteArray,
    ): OfflineRegionInfo =
      Arena.ofConfined().use { arena ->
        val native = arena.allocate(OFFLINE_REGION_INFO_SIZE)
        native.set(
          ValueLayout.JAVA_INT,
          OFFLINE_REGION_INFO_SIZE_OFFSET,
          OFFLINE_REGION_INFO_SIZE.toInt(),
        )
        native.set(ValueLayout.JAVA_LONG, OFFLINE_REGION_INFO_ID_OFFSET, id)
        native
          .asSlice(OFFLINE_REGION_INFO_DEFINITION_OFFSET, OFFLINE_REGION_DEFINITION_SIZE)
          .copyFrom(offlineRegionDefinition(definition, arena))
        val copiedMetadata =
          if (metadata.isEmpty()) MemorySegment.NULL
          else
            arena.allocate(metadata.size.toLong()).also {
              MemorySegment.copy(metadata, 0, it, ValueLayout.JAVA_BYTE, 0, metadata.size)
            }
        native.set(ValueLayout.ADDRESS, OFFLINE_REGION_INFO_METADATA_OFFSET, copiedMetadata)
        native.set(
          ValueLayout.JAVA_LONG,
          OFFLINE_REGION_INFO_METADATA_SIZE_OFFSET,
          metadata.size.toLong(),
        )
        offlineRegionInfo(native)
      }

    fun styleImageOptionsSnapshot(value: StyleImageOptions): StyleImageOptionsSnapshot =
      Arena.ofConfined().use { arena ->
        val native = styleImageOptions(arena, value)
        StyleImageOptionsSnapshot(
          native.get(ValueLayout.JAVA_INT, STYLE_IMAGE_OPTIONS_FIELDS_OFFSET),
          native.get(ValueLayout.JAVA_FLOAT, STYLE_IMAGE_OPTIONS_PIXEL_RATIO_OFFSET),
          native.get(ValueLayout.JAVA_BOOLEAN, STYLE_IMAGE_OPTIONS_SDF_OFFSET),
        )
      }

    fun textureImageInfoSnapshot(
      width: Int,
      height: Int,
      stride: Int,
      byteLength: Long,
    ): TextureImageInfo =
      Arena.ofConfined().use { arena ->
        val native = textureImageInfo(arena)
        mln_texture_image_info.width(native, width)
        mln_texture_image_info.height(native, height)
        mln_texture_image_info.stride(native, stride)
        mln_texture_image_info.byte_length(native, byteLength)
        readTextureImageInfo(native)
      }

    fun styleImageInfoSnapshot(byteLength: Long): StyleImageInfo =
      Arena.ofConfined().use { arena ->
        val native = styleImageInfoDefault(arena)
        native.set(ValueLayout.JAVA_INT, STYLE_IMAGE_INFO_WIDTH_OFFSET, 2)
        native.set(ValueLayout.JAVA_INT, STYLE_IMAGE_INFO_HEIGHT_OFFSET, 3)
        native.set(ValueLayout.JAVA_INT, STYLE_IMAGE_INFO_STRIDE_OFFSET, 8)
        native.set(ValueLayout.JAVA_LONG, STYLE_IMAGE_INFO_BYTE_LENGTH_OFFSET, byteLength)
        styleImageInfo(native)
      }

    fun sourceInfoSnapshot(type: Int, volatileSource: Boolean): SourceInfo =
      Arena.ofConfined().use { arena ->
        val native = arena.allocate(STYLE_SOURCE_INFO_SIZE)
        native.set(ValueLayout.JAVA_INT, STYLE_SOURCE_INFO_TYPE_OFFSET, type)
        native.set(ValueLayout.JAVA_BOOLEAN, STYLE_SOURCE_INFO_IS_VOLATILE_OFFSET, volatileSource)
        SourceInfo(
          SourceType.fromNative(native.get(ValueLayout.JAVA_INT, STYLE_SOURCE_INFO_TYPE_OFFSET)),
          native.get(ValueLayout.JAVA_BOOLEAN, STYLE_SOURCE_INFO_IS_VOLATILE_OFFSET),
          null,
          null,
          null,
          null,
          null,
          null,
        )
      }

    /**
     * Decodes a payload window of [bytes], for tests that synthesize a payload kind this version
     * cannot queue. The synthetic window is [bytes] alone.
     */
    fun unknownRuntimePayload(type: Int, bytes: ByteArray): RuntimeEventPayload =
      Arena.ofConfined().use { arena ->
        val payload =
          if (bytes.isEmpty()) MemorySegment.NULL
          else
            arena.allocate(bytes.size.toLong()).also {
              MemorySegment.copy(bytes, 0, it, ValueLayout.JAVA_BYTE, 0, bytes.size)
            }
        runtimeEventPayload(type, payload)
      }

    fun ownedBufferCleanupAfterCopyFailure(): Int {
      var destroys = 0
      try {
        ownedBuffer(
          NativeOwnedBuffer(1L),
          getter = { _, bytes ->
            mln_buffer_view.data(bytes, MemorySegment.ofAddress(1L))
            mln_buffer_view.size(bytes, -1L)
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )
      } catch (_: IllegalArgumentException) {
        return destroys
      }
      error("buffer conversion unexpectedly succeeded")
    }

    fun offlineRegionListCleanupAfterCopyFailure(): Int {
      var destroys = 0
      try {
        offlineRegionList(
          NativeOfflineRegionList(1L),
          counter = { _, outCount ->
            outCount.set(ValueLayout.JAVA_LONG, 0, Long.MAX_VALUE)
            MaplibreStatus.OK.nativeCode
          },
          getter = { _, _, _ -> MaplibreStatus.OK.nativeCode },
          destroyer = { destroys++ },
        )
      } catch (_: ArithmeticException) {
        return destroys
      }
      error("offline list conversion unexpectedly succeeded")
    }

    fun styleStringListCleanupAfterCopyFailure(): Int {
      var destroys = 0
      try {
        styleStringList(
          NativeStyleStringList(1L),
          counter = { _, outCount ->
            outCount.set(ValueLayout.JAVA_LONG, 0, Long.MAX_VALUE)
            MaplibreStatus.OK.nativeCode
          },
          getter = { _, _, _ -> MaplibreStatus.OK.nativeCode },
          destroyer = { destroys++ },
        )
      } catch (_: ArithmeticException) {
        return destroys
      }
      error("style list conversion unexpectedly succeeded")
    }

    fun mapOptionsSnapshot(value: MapOptions): MapOptionsSnapshot =
      Arena.ofConfined().use { arena ->
        val native = mapOptions(value, arena)
        MapOptionsSnapshot(
          mln_map_options.width(native),
          mln_map_options.height(native),
          mln_map_options.scale_factor(native),
          mln_map_options.map_mode(native),
          mln_map_options.fast_pfor_enabled(native),
        )
      }

    fun metalSnapshot(value: MetalBorrowedTextureDescriptor): RenderDescriptorSnapshot =
      Arena.ofConfined().use { arena ->
        val native = metalBorrowedTextureDescriptor(value)(arena)
        val extent = mln_metal_borrowed_texture_descriptor.extent(native)
        RenderDescriptorSnapshot(
          mln_render_target_extent.width(extent),
          mln_render_target_extent.height(extent),
          mln_render_target_extent.scale_factor(extent),
          mln_metal_borrowed_texture_descriptor.texture(native).address(),
          0L,
          0,
        )
      }

    fun vulkanSnapshot(value: VulkanBorrowedTextureDescriptor): RenderDescriptorSnapshot =
      Arena.ofConfined().use { arena ->
        val native = vulkanBorrowedTextureDescriptor(value)(arena)
        val extent = mln_vulkan_borrowed_texture_descriptor.extent(native)
        RenderDescriptorSnapshot(
          mln_render_target_extent.width(extent),
          mln_render_target_extent.height(extent),
          mln_render_target_extent.scale_factor(extent),
          mln_vulkan_borrowed_texture_descriptor.image(native),
          mln_vulkan_borrowed_texture_descriptor.image_view(native),
          mln_vulkan_borrowed_texture_descriptor.final_layout(native),
        )
      }

    fun openGlSnapshot(value: OpenGLBorrowedTextureDescriptor): RenderDescriptorSnapshot =
      Arena.ofConfined().use { arena ->
        val native = openglBorrowedTextureDescriptor(value)(arena)
        val extent = mln_opengl_borrowed_texture_descriptor.extent(native)
        val context = mln_opengl_borrowed_texture_descriptor.context(native)
        val data = mln_opengl_context_descriptor.data(context)
        val egl = mln_opengl_context_descriptor.data.egl(data)
        RenderDescriptorSnapshot(
          mln_render_target_extent.width(extent),
          mln_render_target_extent.height(extent),
          mln_render_target_extent.scale_factor(extent),
          mln_opengl_borrowed_texture_descriptor.texture(native).toLong(),
          mln_egl_context_descriptor.display(egl).address(),
          mln_opengl_borrowed_texture_descriptor.target(native),
        )
      }

    fun resourceResponseSnapshot(value: ResourceResponse): ResourceResponseSnapshot =
      Arena.ofConfined().use { arena ->
        val native = resourceResponse(value, arena)
        ResourceResponseSnapshot(
          native.get(ValueLayout.JAVA_INT, RESOURCE_RESPONSE_STATUS_OFFSET),
          copyBytes(
            native.get(ValueLayout.ADDRESS, RESOURCE_RESPONSE_BYTES_OFFSET),
            native.get(ValueLayout.JAVA_LONG, RESOURCE_RESPONSE_BYTE_COUNT_OFFSET),
          ),
          native.get(ValueLayout.JAVA_BOOLEAN, RESOURCE_RESPONSE_MUST_REVALIDATE_OFFSET),
          native.get(ValueLayout.JAVA_BOOLEAN, RESOURCE_RESPONSE_HAS_MODIFIED_OFFSET),
          native.get(ValueLayout.JAVA_BOOLEAN, RESOURCE_RESPONSE_HAS_EXPIRES_OFFSET),
          native.get(ValueLayout.JAVA_BOOLEAN, RESOURCE_RESPONSE_HAS_RETRY_AFTER_OFFSET),
        )
      }

    data class StyleImageOptionsSnapshot(val fields: Int, val pixelRatio: Float, val sdf: Boolean)

    data class AnimationOptionsSnapshot(
      val fields: Int,
      val durationMs: Double,
      val velocity: Double,
      val transitionId: Long,
    )

    data class MapOptionsSnapshot(
      val width: Int,
      val height: Int,
      val scaleFactor: Double,
      val mapMode: Int,
      val fastPforEnabled: Boolean,
    )

    data class ResourceResponseSnapshot(
      val status: Int,
      val bytes: ByteArray,
      val mustRevalidate: Boolean,
      val hasModified: Boolean,
      val hasExpires: Boolean,
      val hasRetryAfter: Boolean,
    )

    data class RenderDescriptorSnapshot(
      val width: Int,
      val height: Int,
      val scaleFactor: Double,
      val firstPointer: Long,
      val secondPointer: Long,
      val extra: Int,
    )
  }

  internal data class NativeRuntimeEvent(
    val type: Int,
    val sourceType: Int,
    val sourceId: Long,
    val code: Int,
    val payload: RuntimeEventPayload,
    val message: String,
  )

  /** Events copied out of one drained batch, plus the count the drain left queued. */
  internal data class NativeRuntimeEventBatch(
    val events: List<NativeRuntimeEvent>,
    val remainingCount: Long,
  )

  internal class OwnedTextureFrameSegment(val segment: MemorySegment, private val arena: Arena) :
    AutoCloseable {
    override fun close() {
      arena.close()
    }
  }
}
