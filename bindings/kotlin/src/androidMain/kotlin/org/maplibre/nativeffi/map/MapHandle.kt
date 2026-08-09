package org.maplibre.nativeffi.map

import org.bytedeco.javacpp.BoolPointer
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.LongPointer
import org.bytedeco.javacpp.Pointer
import org.bytedeco.javacpp.SizeTPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.javacpp.ownedBuffer
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
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
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileJson
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding

/** Owned Android JNI map handle. */
public actual class MapHandle
private constructor(private val runtime: RuntimeHandle, private val handleId: Long) :
  AutoCloseable {
  private val runtimeRetention = runtime.retainChild("MapHandle")
  private val core = HandleStateCore("MapHandle", handleId)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private val customGeometrySources =
    CustomGeometrySourceRegistry<CustomGeometrySourceState>(::releaseCallbackRoot)

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  public actual fun setStyleUrl(url: String) {
    NativeAccess.ensureLoaded()
    optionalCString(url).use { nativeUrl ->
      Status.check(MaplibreNativeC.mln_map_set_style_url(requireLiveHandle(), nativeUrl))
    }
  }

  public actual fun setStyleJson(json: ByteArray) {
    NativeAccess.ensureLoaded()
    ByteArrayViewScope(json).use { nativeJson ->
      Status.check(MaplibreNativeC.mln_map_set_style_json(requireLiveHandle(), nativeJson.view))
    }
    clearCustomGeometrySources()
  }

  public actual fun loadedStyleJson(): ByteArray {
    NativeAccess.ensureLoaded()
    return copyMapBytes(requireLiveHandle()) { mapId, text, capacity, outSize ->
      MaplibreNativeC.mln_map_copy_loaded_style_json(mapId, text, capacity, outSize)
    }
  }

  public actual fun styleUrl(): String {
    NativeAccess.ensureLoaded()
    return copyMapBytes(requireLiveHandle()) { mapId, text, capacity, outSize ->
        MaplibreNativeC.mln_map_copy_style_url(mapId, text, capacity, outSize)
      }
      .toString(java.nio.charset.StandardCharsets.UTF_8)
  }

  public actual fun addStyleSourceJson(sourceId: String, sourceJson: ByteArray) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      ByteArrayViewScope(sourceJson).use { nativeSourceJson ->
        Status.check(
          MaplibreNativeC.mln_map_add_style_source_json(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeSourceJson.view,
          )
        )
      }
    }
  }

  public actual fun removeStyleSource(sourceId: String): Boolean {
    NativeAccess.ensureLoaded()
    val outRemoved = booleanArrayOf(false)
    StringViewScope(sourceId).use { nativeSourceId ->
      Status.check(
        MaplibreNativeC.mln_map_remove_style_source(
          requireLiveHandle(),
          nativeSourceId.view,
          outRemoved,
        )
      )
    }
    if (outRemoved[0]) closeCustomGeometrySource(sourceId)
    return outRemoved[0]
  }

  public actual fun styleSourceExists(sourceId: String): Boolean {
    NativeAccess.ensureLoaded()
    val outExists = booleanArrayOf(false)
    StringViewScope(sourceId).use { nativeSourceId ->
      Status.check(
        MaplibreNativeC.mln_map_style_source_exists(
          requireLiveHandle(),
          nativeSourceId.view,
          outExists,
        )
      )
    }
    return outExists[0]
  }

  public actual fun styleSourceType(sourceId: String): SourceType? {
    NativeAccess.ensureLoaded()
    val outType = intArrayOf(0)
    val outFound = booleanArrayOf(false)
    StringViewScope(sourceId).use { nativeSourceId ->
      Status.check(
        MaplibreNativeC.mln_map_get_style_source_type(
          requireLiveHandle(),
          nativeSourceId.view,
          outType,
          outFound,
        )
      )
    }
    return if (outFound[0]) SourceType.fromNative(outType[0]) else null
  }

  public actual fun styleSourceInfo(sourceId: String): SourceInfo? {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      MaplibreNativeC.mln_style_source_info().use { outInfo ->
        outInfo.size(outInfo.sizeof())
        val outFound = booleanArrayOf(false)
        Status.check(
          MaplibreNativeC.mln_map_get_style_source_info(
            requireLiveHandle(),
            nativeSourceId.view,
            outInfo,
            outFound,
          )
        )
        if (!outFound[0]) return null
        val attribution =
          if (outInfo.has_attribution())
            copyStyleSourceAttribution(requireLiveHandle(), nativeSourceId, outInfo)
          else null
        val fields = outInfo.fields()
        val url =
          if (fields and STYLE_SOURCE_INFO_URL != 0)
            copyStyleSourceUrl(requireLiveHandle(), nativeSourceId, outInfo.url_size())
          else null
        val tileJson =
          if (fields and STYLE_SOURCE_INFO_TILEJSON != 0)
            TileJson(
              copyStyleSourceTileUrls(requireLiveHandle(), nativeSourceId),
              outInfo.min_zoom(),
              outInfo.max_zoom(),
              TileScheme.fromNative(outInfo.scheme()),
              if (fields and STYLE_SOURCE_INFO_BOUNDS != 0) latLngBounds(outInfo.bounds()) else null,
            )
          else null
        return SourceInfo(
          SourceType.fromNative(outInfo.type()),
          outInfo.is_volatile(),
          attribution,
          url,
          tileJson,
          if (fields and STYLE_SOURCE_INFO_TILE_SIZE != 0)
            Math.toIntExact(Integer.toUnsignedLong(outInfo.tile_size()))
          else null,
          if (fields and STYLE_SOURCE_INFO_VECTOR_ENCODING != 0)
            VectorTileEncoding.fromNative(outInfo.vector_encoding())
          else null,
          if (fields and STYLE_SOURCE_INFO_RASTER_ENCODING != 0)
            RasterDemEncoding.fromNative(outInfo.raster_encoding())
          else null,
        )
      }
    }
  }

  public actual fun styleSourceIds(): List<String> {
    NativeAccess.ensureLoaded()
    LongPointer(1).use { outList ->
      outList.put(0, 0L)
      Status.check(MaplibreNativeC.mln_map_list_style_source_ids(requireLiveHandle(), outList))
      val list = outList.get()
      require(list != 0L) { "mln_map_list_style_source_ids returned the null handle" }
      return styleIdList(list)
    }
  }

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewScope(url).use { nativeUrl ->
        GeoJsonSourceOptionsScope(options).use { nativeOptions ->
          Status.check(
            MaplibreNativeC.mln_map_add_geojson_source_url(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeUrl.view,
              nativeOptions.options,
            )
          )
        }
      }
    }
  }

  public actual fun addGeoJsonSourceData(
    sourceId: String,
    data: ByteArray,
    options: GeoJsonSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      ByteArrayViewScope(data).use { nativeData ->
        GeoJsonSourceOptionsScope(options).use { nativeOptions ->
          Status.check(
            MaplibreNativeC.mln_map_add_geojson_source_data(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeData.view,
              nativeOptions.options,
            )
          )
        }
      }
    }
  }

  public actual fun setGeoJsonSourceUrl(sourceId: String, url: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewScope(url).use { nativeUrl ->
        Status.check(
          MaplibreNativeC.mln_map_set_geojson_source_url(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeUrl.view,
          )
        )
      }
    }
  }

  public actual fun setGeoJsonSourceData(sourceId: String, data: ByteArray) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      ByteArrayViewScope(data).use { nativeData ->
        Status.check(
          MaplibreNativeC.mln_map_set_geojson_source_data(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeData.view,
          )
        )
      }
    }
  }

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ) {
    NativeAccess.ensureLoaded()
    val sourceState = CustomGeometrySourceState(options)
    customGeometrySources.install(sourceId, sourceState) {
      StringViewScope(sourceId).use { nativeSourceId ->
        Status.check(
          MaplibreNativeC.mln_map_add_custom_geometry_source(
            requireLiveHandle(),
            nativeSourceId.view,
            sourceState.descriptor,
          )
        )
      }
      HandleLeakCleaner.retainNativeCallbackRoot(sourceState)
    }
  }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      CanonicalTileIdScope(tileId).use { nativeTileId ->
        ByteArrayViewScope(data).use { nativeData ->
          Status.check(
            MaplibreNativeC.mln_map_set_custom_geometry_source_tile_data(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeTileId.tileId,
              nativeData.view,
            )
          )
        }
      }
    }
  }

  public actual fun invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileId) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      CanonicalTileIdScope(tileId).use { nativeTileId ->
        Status.check(
          MaplibreNativeC.mln_map_invalidate_custom_geometry_source_tile(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeTileId.tileId,
          )
        )
      }
    }
  }

  public actual fun invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      Status.check(
        MaplibreNativeC.mln_map_invalidate_custom_geometry_source_region(
          requireLiveHandle(),
          nativeSourceId.view,
          latLngBounds(bounds),
        )
      )
    }
  }

  public actual fun addVectorSourceUrl(sourceId: String, url: String, options: TileSourceOptions?) {
    NativeAccess.ensureLoaded()
    addTileSourceUrl(MaplibreNativeC::mln_map_add_vector_source_url, sourceId, url, options)
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    addTileSourceTiles(MaplibreNativeC::mln_map_add_vector_source_tiles, sourceId, tiles, options)
  }

  public actual fun addRasterSourceUrl(sourceId: String, url: String, options: TileSourceOptions?) {
    NativeAccess.ensureLoaded()
    addTileSourceUrl(MaplibreNativeC::mln_map_add_raster_source_url, sourceId, url, options)
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    addTileSourceTiles(MaplibreNativeC::mln_map_add_raster_source_tiles, sourceId, tiles, options)
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    addTileSourceUrl(MaplibreNativeC::mln_map_add_raster_dem_source_url, sourceId, url, options)
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    addTileSourceTiles(
      MaplibreNativeC::mln_map_add_raster_dem_source_tiles,
      sourceId,
      tiles,
      options,
    )
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ) {
    NativeAccess.ensureLoaded()
    StringViewScope(imageId).use { nativeImageId ->
      PremultipliedImageScope(image).use { nativeImage ->
        StyleImageOptionsScope(options).use { nativeOptions ->
          Status.check(
            MaplibreNativeC.mln_map_set_style_image(
              requireLiveHandle(),
              nativeImageId.view,
              nativeImage.image,
              nativeOptions.options,
            )
          )
        }
      }
    }
  }

  public actual fun removeStyleImage(imageId: String): Boolean {
    NativeAccess.ensureLoaded()
    val outRemoved = booleanArrayOf(false)
    StringViewScope(imageId).use { nativeImageId ->
      Status.check(
        MaplibreNativeC.mln_map_remove_style_image(
          requireLiveHandle(),
          nativeImageId.view,
          outRemoved,
        )
      )
    }
    return outRemoved[0]
  }

  public actual fun styleImageExists(imageId: String): Boolean {
    NativeAccess.ensureLoaded()
    val outExists = booleanArrayOf(false)
    StringViewScope(imageId).use { nativeImageId ->
      Status.check(
        MaplibreNativeC.mln_map_style_image_exists(
          requireLiveHandle(),
          nativeImageId.view,
          outExists,
        )
      )
    }
    return outExists[0]
  }

  public actual fun styleImageInfo(imageId: String): StyleImageInfo? {
    NativeAccess.ensureLoaded()
    StringViewScope(imageId).use { nativeImageId ->
      MaplibreNativeC.mln_style_image_info_default().use { outInfo ->
        val outFound = booleanArrayOf(false)
        Status.check(
          MaplibreNativeC.mln_map_get_style_image_info(
            requireLiveHandle(),
            nativeImageId.view,
            outInfo,
            outFound,
          )
        )
        return if (outFound[0]) styleImageInfo(outInfo) else null
      }
    }
  }

  public actual fun copyStyleImagePremultipliedRgba8(imageId: String): StyleImage? {
    NativeAccess.ensureLoaded()
    val info = styleImageInfo(imageId) ?: return null
    val outPixels = ByteArray(Math.toIntExact(info.byteLength))
    val outFound = booleanArrayOf(false)
    StringViewScope(imageId).use { nativeImageId ->
      SizeTPointer(1).use { outByteLength ->
        Status.check(
          MaplibreNativeC.mln_map_copy_style_image_premultiplied_rgba8(
            requireLiveHandle(),
            nativeImageId.view,
            outPixels,
            outPixels.size.toLong(),
            outByteLength,
            outFound,
          )
        )
      }
    }
    return if (outFound[0]) {
      StyleImage(
        PremultipliedRgba8Image(info.width, info.height, info.stride, outPixels),
        info.pixelRatio,
        info.sdf,
      )
    } else null
  }

  public actual fun addImageSourceUrl(sourceId: String, coordinates: List<LatLng>, url: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      LatLngArrayScope(coordinates).use { nativeCoordinates ->
        StringViewScope(url).use { nativeUrl ->
          Status.check(
            MaplibreNativeC.mln_map_add_image_source_url(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeCoordinates.coordinates,
              nativeCoordinates.count,
              nativeUrl.view,
            )
          )
        }
      }
    }
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      LatLngArrayScope(coordinates).use { nativeCoordinates ->
        PremultipliedImageScope(image).use { nativeImage ->
          Status.check(
            MaplibreNativeC.mln_map_add_image_source_image(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeCoordinates.coordinates,
              nativeCoordinates.count,
              nativeImage.image,
            )
          )
        }
      }
    }
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewScope(url).use { nativeUrl ->
        Status.check(
          MaplibreNativeC.mln_map_set_image_source_url(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeUrl.view,
          )
        )
      }
    }
  }

  public actual fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      PremultipliedImageScope(image).use { nativeImage ->
        Status.check(
          MaplibreNativeC.mln_map_set_image_source_image(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeImage.image,
          )
        )
      }
    }
  }

  public actual fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      LatLngArrayScope(coordinates).use { nativeCoordinates ->
        Status.check(
          MaplibreNativeC.mln_map_set_image_source_coordinates(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeCoordinates.coordinates,
            nativeCoordinates.count,
          )
        )
      }
    }
  }

  public actual fun imageSourceCoordinates(sourceId: String): List<LatLng>? {
    NativeAccess.ensureLoaded()
    val outFound = booleanArrayOf(false)
    StringViewScope(sourceId).use { nativeSourceId ->
      LatLngArrayScope(IMAGE_SOURCE_COORDINATE_COUNT).use { outCoordinates ->
        SizeTPointer(1).use { outCoordinateCount ->
          Status.check(
            MaplibreNativeC.mln_map_get_image_source_coordinates(
              requireLiveHandle(),
              nativeSourceId.view,
              outCoordinates.coordinates,
              outCoordinates.count,
              outCoordinateCount,
              outFound,
            )
          )
          return if (outFound[0]) {
            outCoordinates.toList(Math.toIntExact(outCoordinateCount.get()))
          } else null
        }
      }
    }
  }

  public actual fun addStyleLayerJson(layerJson: ByteArray, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    ByteArrayViewScope(layerJson).use { nativeLayerJson ->
      StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_add_style_layer_json(
            requireLiveHandle(),
            nativeLayerJson.view,
            nativeBeforeLayerId.view,
          )
        )
      }
    }
  }

  public actual fun addHillshadeLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(sourceId).use { nativeSourceId ->
        StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
          Status.check(
            MaplibreNativeC.mln_map_add_hillshade_layer(
              requireLiveHandle(),
              nativeLayerId.view,
              nativeSourceId.view,
              nativeBeforeLayerId.view,
            )
          )
        }
      }
    }
  }

  public actual fun addColorReliefLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(sourceId).use { nativeSourceId ->
        StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
          Status.check(
            MaplibreNativeC.mln_map_add_color_relief_layer(
              requireLiveHandle(),
              nativeLayerId.view,
              nativeSourceId.view,
              nativeBeforeLayerId.view,
            )
          )
        }
      }
    }
  }

  public actual fun addLocationIndicatorLayer(layerId: String, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_add_location_indicator_layer(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeBeforeLayerId.view,
          )
        )
      }
    }
  }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      latLng(coordinate).use { nativeCoordinate ->
        Status.check(
          MaplibreNativeC.mln_map_set_location_indicator_location(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeCoordinate,
            altitude,
          )
        )
      }
    }
  }

  public actual fun setLocationIndicatorBearing(layerId: String, bearing: Double) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_location_indicator_bearing(
          requireLiveHandle(),
          nativeLayerId.view,
          bearing,
        )
      )
    }
  }

  public actual fun setLocationIndicatorAccuracyRadius(layerId: String, radius: Double) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_location_indicator_accuracy_radius(
          requireLiveHandle(),
          nativeLayerId.view,
          radius,
        )
      )
    }
  }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(imageId).use { nativeImageId ->
        Status.check(
          MaplibreNativeC.mln_map_set_location_indicator_image_name(
            requireLiveHandle(),
            nativeLayerId.view,
            imageKind.nativeValue,
            nativeImageId.view,
          )
        )
      }
    }
  }

  public actual fun removeStyleLayer(layerId: String): Boolean {
    NativeAccess.ensureLoaded()
    val outRemoved = booleanArrayOf(false)
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_remove_style_layer(
          requireLiveHandle(),
          nativeLayerId.view,
          outRemoved,
        )
      )
    }
    return outRemoved[0]
  }

  public actual fun styleLayerExists(layerId: String): Boolean {
    NativeAccess.ensureLoaded()
    val outExists = booleanArrayOf(false)
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_style_layer_exists(
          requireLiveHandle(),
          nativeLayerId.view,
          outExists,
        )
      )
    }
    return outExists[0]
  }

  public actual fun styleLayerType(layerId: String): String? {
    NativeAccess.ensureLoaded()
    val outFound = booleanArrayOf(false)
    MaplibreNativeC.mln_buffer_view().use { outType ->
      StringViewScope(layerId).use { nativeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_get_style_layer_type(
            requireLiveHandle(),
            nativeLayerId.view,
            outType,
            outFound,
          )
        )
      }
      return if (outFound[0]) stringView(outType) else null
    }
  }

  public actual fun styleLayerIds(): List<String> {
    NativeAccess.ensureLoaded()
    LongPointer(1).use { outList ->
      outList.put(0, 0L)
      Status.check(MaplibreNativeC.mln_map_list_style_layer_ids(requireLiveHandle(), outList))
      val list = outList.get()
      require(list != 0L) { "mln_map_list_style_layer_ids returned the null handle" }
      return styleIdList(list)
    }
  }

  public actual fun moveStyleLayer(layerId: String, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_move_style_layer(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeBeforeLayerId.view,
          )
        )
      }
    }
  }

  public actual fun styleLayerJson(layerId: String): ByteArray? {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      LongPointer(1).use { outSnapshot ->
        BoolPointer(1).use { outFound ->
          outSnapshot.put(0, 0L)
          Status.check(
            MaplibreNativeC.mln_map_get_style_layer_json(
              requireLiveHandle(),
              nativeLayerId.view,
              outSnapshot,
              outFound,
            )
          )
          return if (outFound.get()) ownedBuffer(outSnapshot.get()) else null
        }
      }
    }
  }

  public actual fun setStyleLightJson(lightJson: ByteArray) {
    NativeAccess.ensureLoaded()
    ByteArrayViewScope(lightJson).use { nativeLightJson ->
      Status.check(
        MaplibreNativeC.mln_map_set_style_light_json(requireLiveHandle(), nativeLightJson.view)
      )
    }
  }

  public actual fun setStyleLightProperty(propertyName: String, value: ByteArray) {
    NativeAccess.ensureLoaded()
    StringViewScope(propertyName).use { nativePropertyName ->
      ByteArrayViewScope(value).use { nativeValue ->
        Status.check(
          MaplibreNativeC.mln_map_set_style_light_property(
            requireLiveHandle(),
            nativePropertyName.view,
            nativeValue.view,
          )
        )
      }
    }
  }

  public actual fun styleLightProperty(propertyName: String): ByteArray? {
    NativeAccess.ensureLoaded()
    StringViewScope(propertyName).use { nativePropertyName ->
      LongPointer(1).use { outSnapshot ->
        outSnapshot.put(0, 0L)
        Status.check(
          MaplibreNativeC.mln_map_get_style_light_property(
            requireLiveHandle(),
            nativePropertyName.view,
            outSnapshot,
          )
        )
        return outSnapshot.get().takeIf { it != 0L }?.let(::ownedBuffer)
      }
    }
  }

  public actual fun setStyleTransitionOptions(options: StyleTransitionOptions) {
    NativeAccess.ensureLoaded()
    StyleTransitionOptionsScope(options).use { nativeOptions ->
      Status.check(
        MaplibreNativeC.mln_map_set_style_transition_options(
          requireLiveHandle(),
          nativeOptions.options,
        )
      )
    }
  }

  public actual fun styleTransitionOptions(): StyleTransitionOptions {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_style_transition_options_default().use { outOptions ->
      Status.check(
        MaplibreNativeC.mln_map_get_style_transition_options(requireLiveHandle(), outOptions)
      )
      return styleTransitionOptions(outOptions)
    }
  }

  public actual fun setLayerProperty(layerId: String, propertyName: String, value: ByteArray) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(propertyName).use { nativePropertyName ->
        ByteArrayViewScope(value).use { nativeValue ->
          Status.check(
            MaplibreNativeC.mln_map_set_layer_property(
              requireLiveHandle(),
              nativeLayerId.view,
              nativePropertyName.view,
              nativeValue.view,
            )
          )
        }
      }
    }
  }

  public actual fun layerProperty(layerId: String, propertyName: String): ByteArray? {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(propertyName).use { nativePropertyName ->
        LongPointer(1).use { outSnapshot ->
          outSnapshot.put(0, 0L)
          Status.check(
            MaplibreNativeC.mln_map_get_layer_property(
              requireLiveHandle(),
              nativeLayerId.view,
              nativePropertyName.view,
              outSnapshot,
            )
          )
          return outSnapshot.get().takeIf { it != 0L }?.let(::ownedBuffer)
        }
      }
    }
  }

  public actual fun setLayerFilter(layerId: String, filter: ByteArray) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      ByteArrayViewScope(filter).use { nativeFilter ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_filter(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeFilter.view,
          )
        )
      }
    }
  }

  public actual fun clearLayerFilter(layerId: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_layer_filter(requireLiveHandle(), nativeLayerId.view, null)
      )
    }
  }

  public actual fun layerFilter(layerId: String): ByteArray? {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      LongPointer(1).use { outSnapshot ->
        outSnapshot.put(0, 0L)
        Status.check(
          MaplibreNativeC.mln_map_get_layer_filter(
            requireLiveHandle(),
            nativeLayerId.view,
            outSnapshot,
          )
        )
        return outSnapshot.get().takeIf { it != 0L }?.let(::ownedBuffer)
      }
    }
  }

  public actual fun styleImageStretches(
    imageId: String
  ): Pair<List<ImageStretch>, List<ImageStretch>>? {
    NativeAccess.ensureLoaded()
    StringViewScope(imageId).use { nativeImageId ->
      SizeTPointer(1).use { outXCount ->
        SizeTPointer(1).use { outYCount ->
          val outFound = booleanArrayOf(false)
          Status.check(
            MaplibreNativeC.mln_map_copy_style_image_stretches(
              requireLiveHandle(),
              nativeImageId.view,
              null,
              0L,
              outXCount,
              null,
              0L,
              outYCount,
              outFound,
            )
          )
          if (!outFound[0]) return null

          val xCount = Math.toIntExact(outXCount.get())
          val yCount = Math.toIntExact(outYCount.get())
          val rawX = if (xCount == 0) null else MaplibreNativeC.mln_image_stretch(xCount.toLong())
          val rawY = if (yCount == 0) null else MaplibreNativeC.mln_image_stretch(yCount.toLong())
          try {
            Status.check(
              MaplibreNativeC.mln_map_copy_style_image_stretches(
                requireLiveHandle(),
                nativeImageId.view,
                rawX,
                xCount.toLong(),
                outXCount,
                rawY,
                yCount.toLong(),
                outYCount,
                outFound,
              )
            )
            return readStretches(rawX, xCount) to readStretches(rawY, yCount)
          } finally {
            rawX?.close()
            rawY?.close()
          }
        }
      }
    }
  }

  public actual fun setLayerSourceLayer(layerId: String, sourceLayer: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(sourceLayer).use { nativeSourceLayer ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_source_layer(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeSourceLayer.view,
          )
        )
      }
    }
  }

  public actual fun layerSourceLayer(layerId: String): String {
    NativeAccess.ensureLoaded()
    return copyLayerText(requireLiveHandle(), layerId) { mapId, view, text, capacity, outSize ->
      MaplibreNativeC.mln_map_copy_layer_source_layer(mapId, view, text, capacity, outSize)
    }
  }

  public actual fun setLayerSourceId(layerId: String, sourceId: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(sourceId).use { nativeSourceId ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_source_id(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeSourceId.view,
          )
        )
      }
    }
  }

  public actual fun layerSourceId(layerId: String): String {
    NativeAccess.ensureLoaded()
    return copyLayerText(requireLiveHandle(), layerId) { mapId, view, text, capacity, outSize ->
      MaplibreNativeC.mln_map_copy_layer_source_id(mapId, view, text, capacity, outSize)
    }
  }

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_layer_min_zoom(requireLiveHandle(), nativeLayerId.view, minZoom)
      )
    }
  }

  public actual fun layerMinZoom(layerId: String): Double {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      val outZoom = doubleArrayOf(0.0)
      Status.check(
        MaplibreNativeC.mln_map_get_layer_min_zoom(requireLiveHandle(), nativeLayerId.view, outZoom)
      )
      return outZoom[0]
    }
  }

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_layer_max_zoom(requireLiveHandle(), nativeLayerId.view, maxZoom)
      )
    }
  }

  public actual fun layerMaxZoom(layerId: String): Double {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      val outZoom = doubleArrayOf(0.0)
      Status.check(
        MaplibreNativeC.mln_map_get_layer_max_zoom(requireLiveHandle(), nativeLayerId.view, outZoom)
      )
      return outZoom[0]
    }
  }

  public actual fun setLayerVisibility(layerId: String, visibility: StyleLayerVisibility) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_layer_visibility(
          requireLiveHandle(),
          nativeLayerId.view,
          visibility.nativeValue,
        )
      )
    }
  }

  public actual fun layerVisibility(layerId: String): StyleLayerVisibility {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      val outVisibility = intArrayOf(0)
      Status.check(
        MaplibreNativeC.mln_map_get_layer_visibility(
          requireLiveHandle(),
          nativeLayerId.view,
          outVisibility,
        )
      )
      return StyleLayerVisibility.fromNative(outVisibility[0])
    }
  }

  public actual fun requestRepaint() {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_request_repaint(requireLiveHandle()))
  }

  public actual fun requestStillImage() {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_request_still_image(requireLiveHandle()))
  }

  public actual var debugOptions: Set<DebugOption>
    get() {
      NativeAccess.ensureLoaded()
      val outOptions = intArrayOf(0)
      Status.check(MaplibreNativeC.mln_map_get_debug_options(requireLiveHandle(), outOptions))
      return debugOptions(outOptions[0])
    }
    set(value) {
      NativeAccess.ensureLoaded()
      Status.check(
        MaplibreNativeC.mln_map_set_debug_options(requireLiveHandle(), debugOptionMask(value))
      )
    }

  public actual var isRenderingStatsViewEnabled: Boolean
    get() {
      NativeAccess.ensureLoaded()
      val outEnabled = booleanArrayOf(false)
      Status.check(
        MaplibreNativeC.mln_map_get_rendering_stats_view_enabled(requireLiveHandle(), outEnabled)
      )
      return outEnabled[0]
    }
    set(value) {
      NativeAccess.ensureLoaded()
      Status.check(
        MaplibreNativeC.mln_map_set_rendering_stats_view_enabled(requireLiveHandle(), value)
      )
    }

  public actual val isFullyLoaded: Boolean
    get() {
      NativeAccess.ensureLoaded()
      val outLoaded = booleanArrayOf(false)
      Status.check(MaplibreNativeC.mln_map_is_fully_loaded(requireLiveHandle(), outLoaded))
      return outLoaded[0]
    }

  public actual fun dumpDebugLogs() {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_dump_debug_logs(requireLiveHandle()))
  }

  public actual val size: MapSize
    get() {
      NativeAccess.ensureLoaded()
      val outWidth = intArrayOf(0)
      val outHeight = intArrayOf(0)
      val outScaleFactor = doubleArrayOf(0.0)
      Status.check(
        MaplibreNativeC.mln_map_get_size(requireLiveHandle(), outWidth, outHeight, outScaleFactor)
      )
      return MapSize(outWidth[0], outHeight[0], outScaleFactor[0])
    }

  public actual var viewportOptions: ViewportOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_map_viewport_options_default().use { outOptions ->
        Status.check(MaplibreNativeC.mln_map_get_viewport_options(requireLiveHandle(), outOptions))
        return viewportOptions(outOptions)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      ViewportOptionsScope(value).use { nativeOptions ->
        Status.check(
          MaplibreNativeC.mln_map_set_viewport_options(requireLiveHandle(), nativeOptions.options)
        )
      }
    }

  public actual var tileOptions: TileOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_map_tile_options_default().use { outOptions ->
        Status.check(MaplibreNativeC.mln_map_get_tile_options(requireLiveHandle(), outOptions))
        return tileOptions(outOptions)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      TileOptionsScope(value).use { nativeOptions ->
        Status.check(
          MaplibreNativeC.mln_map_set_tile_options(requireLiveHandle(), nativeOptions.options)
        )
      }
    }

  public actual val camera: CameraOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_camera_options_default().use { outCamera ->
        Status.check(MaplibreNativeC.mln_map_get_camera(requireLiveHandle(), outCamera))
        return cameraOptions(outCamera)
      }
    }

  public actual fun jumpTo(camera: CameraOptions) {
    NativeAccess.ensureLoaded()
    CameraOptionsScope(camera).use { nativeCamera ->
      Status.check(MaplibreNativeC.mln_map_jump_to(requireLiveHandle(), nativeCamera.options))
    }
  }

  public actual fun easeTo(camera: CameraOptions, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    CameraOptionsScope(camera).use { nativeCamera ->
      AnimationOptionsScope(animation).use { nativeAnimation ->
        Status.check(
          MaplibreNativeC.mln_map_ease_to(
            requireLiveHandle(),
            nativeCamera.options,
            nativeAnimation.options,
          )
        )
      }
    }
  }

  public actual fun flyTo(camera: CameraOptions, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    CameraOptionsScope(camera).use { nativeCamera ->
      AnimationOptionsScope(animation).use { nativeAnimation ->
        Status.check(
          MaplibreNativeC.mln_map_fly_to(
            requireLiveHandle(),
            nativeCamera.options,
            nativeAnimation.options,
          )
        )
      }
    }
  }

  public actual fun moveBy(deltaX: Double, deltaY: Double) {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_move_by(requireLiveHandle(), deltaX, deltaY))
  }

  public actual fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    AnimationOptionsScope(animation).use { nativeAnimation ->
      Status.check(
        MaplibreNativeC.mln_map_move_by_animated(
          requireLiveHandle(),
          deltaX,
          deltaY,
          nativeAnimation.options,
        )
      )
    }
  }

  public actual fun scaleBy(scale: Double, anchor: ScreenPoint?) {
    NativeAccess.ensureLoaded()
    ScreenPointScope(anchor).use { nativeAnchor ->
      Status.check(MaplibreNativeC.mln_map_scale_by(requireLiveHandle(), scale, nativeAnchor.point))
    }
  }

  public actual fun scaleByAnimated(
    scale: Double,
    anchor: ScreenPoint?,
    animation: AnimationOptions?,
  ) {
    NativeAccess.ensureLoaded()
    ScreenPointScope(anchor).use { nativeAnchor ->
      AnimationOptionsScope(animation).use { nativeAnimation ->
        Status.check(
          MaplibreNativeC.mln_map_scale_by_animated(
            requireLiveHandle(),
            scale,
            nativeAnchor.point,
            nativeAnimation.options,
          )
        )
      }
    }
  }

  public actual fun rotateBy(first: ScreenPoint, second: ScreenPoint) {
    NativeAccess.ensureLoaded()
    Status.check(
      MaplibreNativeC.mln_map_rotate_by(
        requireLiveHandle(),
        screenPoint(first),
        screenPoint(second),
      )
    )
  }

  public actual fun rotateByAnimated(
    first: ScreenPoint,
    second: ScreenPoint,
    animation: AnimationOptions?,
  ) {
    NativeAccess.ensureLoaded()
    AnimationOptionsScope(animation).use { nativeAnimation ->
      Status.check(
        MaplibreNativeC.mln_map_rotate_by_animated(
          requireLiveHandle(),
          screenPoint(first),
          screenPoint(second),
          nativeAnimation.options,
        )
      )
    }
  }

  public actual fun pitchBy(pitch: Double) {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_pitch_by(requireLiveHandle(), pitch))
  }

  public actual fun pitchByAnimated(pitch: Double, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    AnimationOptionsScope(animation).use { nativeAnimation ->
      Status.check(
        MaplibreNativeC.mln_map_pitch_by_animated(
          requireLiveHandle(),
          pitch,
          nativeAnimation.options,
        )
      )
    }
  }

  public actual fun cancelTransitions() {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_cancel_transitions(requireLiveHandle()))
  }

  public actual var isGestureInProgress: Boolean
    get() {
      NativeAccess.ensureLoaded()
      val outInProgress = booleanArrayOf(false)
      Status.check(
        MaplibreNativeC.mln_map_is_gesture_in_progress(requireLiveHandle(), outInProgress)
      )
      return outInProgress[0]
    }
    set(value) {
      NativeAccess.ensureLoaded()
      Status.check(MaplibreNativeC.mln_map_set_gesture_in_progress(requireLiveHandle(), value))
    }

  public actual fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): CameraOptions {
    NativeAccess.ensureLoaded()
    CameraFitOptionsScope(fitOptions).use { nativeFitOptions ->
      MaplibreNativeC.mln_camera_options_default().use { outCamera ->
        Status.check(
          MaplibreNativeC.mln_map_camera_for_lat_lng_bounds(
            requireLiveHandle(),
            latLngBounds(bounds),
            nativeFitOptions.options,
            outCamera,
          )
        )
        return cameraOptions(outCamera)
      }
    }
  }

  public actual fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): CameraOptions {
    NativeAccess.ensureLoaded()
    LatLngArrayScope(coordinates).use { nativeCoordinates ->
      CameraFitOptionsScope(fitOptions).use { nativeFitOptions ->
        MaplibreNativeC.mln_camera_options_default().use { outCamera ->
          Status.check(
            MaplibreNativeC.mln_map_camera_for_lat_lngs(
              requireLiveHandle(),
              nativeCoordinates.coordinates,
              nativeCoordinates.count,
              nativeFitOptions.options,
              outCamera,
            )
          )
          return cameraOptions(outCamera)
        }
      }
    }
  }

  public actual fun cameraForGeometry(
    geometry: ByteArray,
    fitOptions: CameraFitOptions?,
  ): CameraOptions {
    NativeAccess.ensureLoaded()
    ByteArrayViewScope(geometry).use { nativeGeometry ->
      CameraFitOptionsScope(fitOptions).use { nativeFitOptions ->
        MaplibreNativeC.mln_camera_options_default().use { outCamera ->
          Status.check(
            MaplibreNativeC.mln_map_camera_for_geometry(
              requireLiveHandle(),
              nativeGeometry.view,
              nativeFitOptions.options,
              outCamera,
            )
          )
          return cameraOptions(outCamera)
        }
      }
    }
  }

  public actual fun latLngBoundsForCamera(camera: CameraOptions): LatLngBounds =
    latLngBoundsForCamera(MaplibreNativeC::mln_map_lat_lng_bounds_for_camera, camera)

  public actual fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): LatLngBounds =
    latLngBoundsForCamera(MaplibreNativeC::mln_map_lat_lng_bounds_for_camera_unwrapped, camera)

  public actual var bounds: BoundOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_bound_options_default().use { outOptions ->
        Status.check(MaplibreNativeC.mln_map_get_bounds(requireLiveHandle(), outOptions))
        return boundOptions(outOptions)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      BoundOptionsScope(value).use { nativeOptions ->
        Status.check(MaplibreNativeC.mln_map_set_bounds(requireLiveHandle(), nativeOptions.options))
      }
    }

  public actual var freeCameraOptions: FreeCameraOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_free_camera_options_default().use { outOptions ->
        Status.check(
          MaplibreNativeC.mln_map_get_free_camera_options(requireLiveHandle(), outOptions)
        )
        return freeCameraOptions(outOptions)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      FreeCameraOptionsScope(value).use { nativeOptions ->
        Status.check(
          MaplibreNativeC.mln_map_set_free_camera_options(
            requireLiveHandle(),
            nativeOptions.options,
          )
        )
      }
    }

  public actual var projectionMode: ProjectionModeOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_projection_mode_default().use { outMode ->
        Status.check(MaplibreNativeC.mln_map_get_projection_mode(requireLiveHandle(), outMode))
        return projectionModeOptions(outMode)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      ProjectionModeOptionsScope(value).use { nativeMode ->
        Status.check(
          MaplibreNativeC.mln_map_set_projection_mode(requireLiveHandle(), nativeMode.mode)
        )
      }
    }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_screen_point().use { outPoint ->
      Status.check(
        MaplibreNativeC.mln_map_pixel_for_lat_lng(requireLiveHandle(), latLng(coordinate), outPoint)
      )
      return screenPoint(outPoint)
    }
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_lat_lng().use { outCoordinate ->
      Status.check(
        MaplibreNativeC.mln_map_lat_lng_for_pixel(
          requireLiveHandle(),
          screenPoint(point),
          outCoordinate,
        )
      )
      return latLng(outCoordinate)
    }
  }

  public actual fun pixelsForLatLngs(coordinates: List<LatLng>): List<ScreenPoint> {
    NativeAccess.ensureLoaded()
    val coordinateSnapshot = coordinates.toList()
    if (coordinateSnapshot.isEmpty()) {
      Status.check(MaplibreNativeC.mln_map_pixels_for_lat_lngs(requireLiveHandle(), null, 0L, null))
      return emptyList()
    }
    LatLngArrayScope(coordinateSnapshot).use { nativeCoordinates ->
      ScreenPointArrayScope(nativeCoordinates.count).use { outPoints ->
        Status.check(
          MaplibreNativeC.mln_map_pixels_for_lat_lngs(
            requireLiveHandle(),
            nativeCoordinates.coordinates,
            nativeCoordinates.count,
            outPoints.points,
          )
        )
        return outPoints.toList(Math.toIntExact(nativeCoordinates.count))
      }
    }
  }

  public actual fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng> {
    NativeAccess.ensureLoaded()
    val pointSnapshot = points.toList()
    if (pointSnapshot.isEmpty()) {
      Status.check(MaplibreNativeC.mln_map_lat_lngs_for_pixels(requireLiveHandle(), null, 0L, null))
      return emptyList()
    }
    ScreenPointArrayScope(pointSnapshot).use { nativePoints ->
      LatLngArrayScope(nativePoints.count).use { outCoordinates ->
        Status.check(
          MaplibreNativeC.mln_map_lat_lngs_for_pixels(
            requireLiveHandle(),
            nativePoints.points,
            nativePoints.count,
            outCoordinates.coordinates,
          )
        )
        return outCoordinates.toList(Math.toIntExact(nativePoints.count))
      }
    }
  }

  public actual fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachMetalOwnedTexture(this, descriptor)

  public actual fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachMetalBorrowedTexture(this, descriptor)

  public actual fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachVulkanOwnedTexture(this, descriptor)

  public actual fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachVulkanBorrowedTexture(this, descriptor)

  public actual fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachOpenGLOwnedTexture(this, descriptor)

  public actual fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachOpenGLBorrowedTexture(this, descriptor)

  public actual fun attachMetalSurface(descriptor: MetalSurfaceDescriptor): RenderSessionHandle =
    RenderSessionHandle.attachMetalSurface(this, descriptor)

  public actual fun attachVulkanSurface(descriptor: VulkanSurfaceDescriptor): RenderSessionHandle =
    RenderSessionHandle.attachVulkanSurface(this, descriptor)

  public actual fun attachOpenGLSurface(descriptor: OpenGLSurfaceDescriptor): RenderSessionHandle =
    RenderSessionHandle.attachOpenGLSurface(this, descriptor)

  public actual fun createProjection(): MapProjectionHandle {
    NativeAccess.ensureLoaded()
    LongPointer(1).use { outProjection ->
      outProjection.put(0, 0L)
      Status.check(MaplibreNativeC.mln_map_projection_create(requireLiveHandle(), outProjection))
      val address = outProjection.get()
      require(address != 0L) { "mln_map_projection_create returned a null projection" }
      return MapProjectionHandle(address)
    }
  }

  public actual override fun close() {
    core.closeOnce(
      destroy = { MaplibreNativeC.mln_map_destroy(handleId) },
      afterSuccess = {
        clearCustomGeometrySources()
        runtime.unregisterMap(this)
        runtimeRetention.close()
      },
    )
  }

  internal fun nativeHandleId(): Long = handleId

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    core.retainChild(childTypeName)

  internal fun releaseDetachedCustomGeometrySources() {
    customGeometrySources.releaseDetached { sourceId ->
      styleSourceType(sourceId) == SourceType.CUSTOM_VECTOR
    }
  }

  private fun addTileSourceUrl(
    function:
      (
        Long,
        MaplibreNativeC.mln_buffer_view,
        MaplibreNativeC.mln_buffer_view,
        MaplibreNativeC.mln_style_tile_source_options,
      ) -> Int,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewScope(url).use { nativeUrl ->
        TileSourceOptionsScope(options).use { nativeOptions ->
          Status.check(
            function(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeUrl.view,
              nativeOptions.options,
            )
          )
        }
      }
    }
  }

  private fun addTileSourceTiles(
    function:
      (
        Long,
        MaplibreNativeC.mln_buffer_view,
        MaplibreNativeC.mln_buffer_view?,
        Long,
        MaplibreNativeC.mln_style_tile_source_options,
      ) -> Int,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    val tileSnapshot = tiles.toList()
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewArrayScope(tileSnapshot).use { nativeTiles ->
        TileSourceOptionsScope(options).use { nativeOptions ->
          Status.check(
            function(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeTiles.views,
              nativeTiles.count,
              nativeOptions.options,
            )
          )
        }
      }
    }
  }

  private fun latLngBoundsForCamera(
    function: (Long, MaplibreNativeC.mln_camera_options, MaplibreNativeC.mln_lat_lng_bounds) -> Int,
    camera: CameraOptions,
  ): LatLngBounds {
    NativeAccess.ensureLoaded()
    CameraOptionsScope(camera).use { nativeCamera ->
      MaplibreNativeC.mln_lat_lng_bounds().use { outBounds ->
        Status.check(function(requireLiveHandle(), nativeCamera.options, outBounds))
        return latLngBounds(outBounds)
      }
    }
  }

  public actual companion object {
    public actual fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle {
      NativeAccess.ensureLoaded()
      MapOptionsScope(options).use { nativeOptions ->
        LongPointer(1).use { outMap ->
          outMap.put(0, 0L)
          Status.check(
            MaplibreNativeC.mln_map_create(runtime.nativeHandleId(), nativeOptions.options, outMap)
          )
          val address = outMap.get()
          require(address != 0L) { "mln_map_create returned a null map" }
          return MapHandle(runtime, address).also { runtime.registerMap(it) }
        }
      }
    }
  }

  private fun requireLiveHandle(): Long {
    core.requireLive()
    return handleId
  }

  private fun closeCustomGeometrySource(sourceId: String) {
    customGeometrySources.remove(sourceId)
  }

  private fun clearCustomGeometrySources() {
    customGeometrySources.clear()
  }
}

private fun latLng(value: LatLng): MaplibreNativeC.mln_lat_lng =
  MaplibreNativeC.mln_lat_lng().latitude(value.latitude).longitude(value.longitude)

private fun latLng(value: MaplibreNativeC.mln_lat_lng): LatLng =
  LatLng(value.latitude(), value.longitude())

private fun latLngBounds(value: LatLngBounds): MaplibreNativeC.mln_lat_lng_bounds =
  MaplibreNativeC.mln_lat_lng_bounds()
    .southwest(latLng(value.southwest))
    .northeast(latLng(value.northeast))

private fun latLngBounds(value: MaplibreNativeC.mln_lat_lng_bounds): LatLngBounds =
  LatLngBounds(latLng(value.southwest()), latLng(value.northeast()))

private fun screenPoint(value: ScreenPoint): MaplibreNativeC.mln_screen_point =
  MaplibreNativeC.mln_screen_point().x(value.x).y(value.y)

private fun screenPoint(value: MaplibreNativeC.mln_screen_point): ScreenPoint =
  ScreenPoint(value.x(), value.y())

private const val IMAGE_SOURCE_COORDINATE_COUNT: Long = 4

private fun optionalCString(value: String): BytePointer {
  return JavaCppSupport.cString(value)
}

private fun stringView(view: MaplibreNativeC.mln_buffer_view): String {
  if (view.size() == 0L || view.data() == null || view.data().isNull) return ""
  val bytes = ByteArray(Math.toIntExact(view.size()))
  BytePointer(view.data()).get(bytes, 0, bytes.size)
  return String(bytes, java.nio.charset.StandardCharsets.UTF_8)
}

private fun styleIdList(list: Long): List<String> =
  try {
    SizeTPointer(1).use { outCount ->
      Status.check(MaplibreNativeC.mln_style_id_list_count(list, outCount))
      List(Math.toIntExact(outCount.get())) { index ->
        MaplibreNativeC.mln_buffer_view().use { outId ->
          Status.check(MaplibreNativeC.mln_style_id_list_get(list, index.toLong(), outId))
          stringView(outId)
        }
      }
    }
  } finally {
    MaplibreNativeC.mln_style_id_list_destroy(list)
  }

private fun styleStringList(list: Long): List<String> =
  styleStringList(
    list,
    counter = MaplibreNativeC::mln_style_string_list_count,
    getter = MaplibreNativeC::mln_style_string_list_get,
    destroyer = MaplibreNativeC::mln_style_string_list_destroy,
  )

private fun styleStringList(
  list: Long,
  counter: (Long, SizeTPointer) -> Int,
  getter: (Long, Long, MaplibreNativeC.mln_buffer_view) -> Int,
  destroyer: (Long) -> Unit,
): List<String> =
  try {
    SizeTPointer(1).use { outCount ->
      Status.check(counter(list, outCount))
      List(Math.toIntExact(outCount.get())) { index ->
        MaplibreNativeC.mln_buffer_view().use { outValue ->
          Status.check(getter(list, index.toLong(), outValue))
          stringView(outValue)
        }
      }
    }
  } finally {
    destroyer(list)
  }

/**
 * Probes the required length, then copies. A null buffer with zero capacity is a size probe the C
 * API answers with OK.
 */
private inline fun copyMapBytes(
  mapId: Long,
  copy: (Long, BytePointer?, Long, SizeTPointer) -> Int,
): ByteArray {
  SizeTPointer(1).use { outSize ->
    Status.check(copy(mapId, null, 0L, outSize))
    val required = Math.toIntExact(outSize.get())
    if (required == 0) return byteArrayOf()
    BytePointer(required.toLong()).use { buffer ->
      SizeTPointer(1).use { outCopied ->
        Status.check(copy(mapId, buffer, required.toLong(), outCopied))
        val bytes = ByteArray(Math.toIntExact(outCopied.get()))
        buffer.get(bytes, 0, bytes.size)
        return bytes
      }
    }
  }
}

private inline fun copyLayerText(
  mapId: Long,
  layerId: String,
  copy: (Long, MaplibreNativeC.mln_buffer_view, BytePointer?, Long, SizeTPointer) -> Int,
): String {
  StringViewScope(layerId).use { nativeLayerId ->
    SizeTPointer(1).use { outSize ->
      Status.check(copy(mapId, nativeLayerId.view, null, 0L, outSize))
      val required = Math.toIntExact(outSize.get())
      if (required == 0) return ""
      BytePointer(required.toLong()).use { buffer ->
        SizeTPointer(1).use { outCopied ->
          Status.check(copy(mapId, nativeLayerId.view, buffer, required.toLong(), outCopied))
          val bytes = ByteArray(Math.toIntExact(outCopied.get()))
          buffer.get(bytes, 0, bytes.size)
          return String(bytes, java.nio.charset.StandardCharsets.UTF_8)
        }
      }
    }
  }
}

private fun copyStyleSourceAttribution(
  mapId: Long,
  sourceId: StringViewScope,
  info: MaplibreNativeC.mln_style_source_info,
): String {
  val attributionSize = Math.toIntExact(info.attribution_size())
  if (attributionSize == 0) return ""
  BytePointer(attributionSize.toLong()).use { outAttribution ->
    SizeTPointer(1).use { outSize ->
      val outFound = booleanArrayOf(false)
      Status.check(
        MaplibreNativeC.mln_map_copy_style_source_attribution(
          mapId,
          sourceId.view,
          outAttribution,
          attributionSize.toLong(),
          outSize,
          outFound,
        )
      )
      if (!outFound[0]) return ""
      val bytes = ByteArray(Math.toIntExact(outSize.get()))
      outAttribution.get(bytes, 0, bytes.size)
      return String(bytes, java.nio.charset.StandardCharsets.UTF_8)
    }
  }
}

private fun copyStyleSourceUrl(mapId: Long, sourceId: StringViewScope, urlSize: Long): String {
  if (urlSize == 0L) return ""
  BytePointer(urlSize).use { outUrl ->
    SizeTPointer(1).use { outSize ->
      val outFound = booleanArrayOf(false)
      Status.check(
        MaplibreNativeC.mln_map_copy_style_source_url(
          mapId,
          sourceId.view,
          outUrl,
          urlSize,
          outSize,
          outFound,
        )
      )
      check(outFound[0]) { "style source disappeared while its metadata was copied" }
      val bytes = ByteArray(Math.toIntExact(outSize.get()))
      outUrl.get(bytes, 0, bytes.size)
      return String(bytes, java.nio.charset.StandardCharsets.UTF_8)
    }
  }
}

private fun copyStyleSourceTileUrls(mapId: Long, sourceId: StringViewScope): List<String> {
  LongPointer(1).use { outList ->
    outList.put(0, 0L)
    val outFound = booleanArrayOf(false)
    Status.check(
      MaplibreNativeC.mln_map_get_style_source_tile_urls(mapId, sourceId.view, outList, outFound)
    )
    check(outFound[0]) { "style source disappeared while its metadata was copied" }
    val list = outList.get()
    require(list != 0L) { "mln_map_get_style_source_tile_urls returned the null handle" }
    return styleStringList(list)
  }
}

private const val STYLE_SOURCE_INFO_URL: Int = 1 shl 0
private const val STYLE_SOURCE_INFO_TILEJSON: Int = 1 shl 1
private const val STYLE_SOURCE_INFO_BOUNDS: Int = 1 shl 2
private const val STYLE_SOURCE_INFO_TILE_SIZE: Int = 1 shl 3
private const val STYLE_SOURCE_INFO_VECTOR_ENCODING: Int = 1 shl 4
private const val STYLE_SOURCE_INFO_RASTER_ENCODING: Int = 1 shl 5

private fun readStretches(
  array: MaplibreNativeC.mln_image_stretch?,
  count: Int,
): List<ImageStretch> =
  List(count) { index ->
    val element = array!!.position(index.toLong())
    ImageStretch(element.from(), element.to())
  }

private fun styleImageInfo(info: MaplibreNativeC.mln_style_image_info): StyleImageInfo =
  StyleImageInfo(
    info.width(),
    info.height(),
    info.stride(),
    checkedSizeT(info.byte_length(), "style image byte length"),
    info.pixel_ratio(),
    info.sdf(),
    checkedSizeT(info.stretch_x_count(), "style image stretch X count"),
    checkedSizeT(info.stretch_y_count(), "style image stretch Y count"),
    if (info.has_content()) {
      val content = info.content()
      ImageContent(content.left(), content.top(), content.right(), content.bottom())
    } else null,
    if (info.has_text_fit_width()) StyleImageTextFit.fromNative(info.text_fit_width()) else null,
    if (info.has_text_fit_height()) StyleImageTextFit.fromNative(info.text_fit_height()) else null,
  )

private fun checkedSizeT(value: Long, name: String): Long {
  require(value >= 0L) { "$name exceeds Long.MAX_VALUE" }
  return value
}

private fun debugOptionMask(options: Set<DebugOption>): Int =
  options.fold(0) { acc, option -> acc or option.nativeMask }

private fun debugOptions(mask: Int): Set<DebugOption> =
  DebugOption.entries.filterTo(mutableSetOf()) { option -> (mask and option.nativeMask) != 0 }

private fun edgeInsets(value: MaplibreNativeC.mln_edge_insets): EdgeInsets =
  EdgeInsets(value.top(), value.left(), value.bottom(), value.right())

private fun writeEdgeInsets(out: MaplibreNativeC.mln_edge_insets, value: EdgeInsets) {
  out.top(value.top).left(value.left).bottom(value.bottom).right(value.right)
}

private fun viewportOptions(value: MaplibreNativeC.mln_map_viewport_options): ViewportOptions {
  val fields = value.fields()
  return ViewportOptions().apply {
    if ((fields and MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0) {
      northOrientation = NorthOrientation.fromNative(value.north_orientation())
    }
    if ((fields and MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0) {
      constrainMode = ConstrainMode.fromNative(value.constrain_mode())
    }
    if ((fields and MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0) {
      viewportMode = ViewportMode.fromNative(value.viewport_mode())
    }
    if ((fields and MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0) {
      frustumOffset = edgeInsets(value.frustum_offset())
    }
  }
}

private fun tileOptions(value: MaplibreNativeC.mln_map_tile_options): TileOptions {
  val fields = value.fields()
  return TileOptions().apply {
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0) {
      prefetchZoomDelta = value.prefetch_zoom_delta()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0) {
      lodMinRadius = value.lod_min_radius()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE) != 0) {
      lodScale = value.lod_scale()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0) {
      lodPitchThreshold = value.lod_pitch_threshold()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0) {
      lodZoomShift = value.lod_zoom_shift()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE) != 0) {
      lodMode = TileLodMode.fromNative(value.lod_mode())
    }
  }
}

private fun projectionModeOptions(
  value: MaplibreNativeC.mln_projection_mode
): ProjectionModeOptions {
  val fields = value.fields()
  return ProjectionModeOptions().apply {
    if ((fields and MaplibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC) != 0) {
      axonometric = value.axonometric()
    }
    if ((fields and MaplibreNativeC.MLN_PROJECTION_MODE_X_SKEW) != 0) {
      xSkew = value.x_skew()
    }
    if ((fields and MaplibreNativeC.MLN_PROJECTION_MODE_Y_SKEW) != 0) {
      ySkew = value.y_skew()
    }
  }
}

private fun cameraOptions(value: MaplibreNativeC.mln_camera_options): CameraOptions {
  val fields = value.fields()
  return CameraOptions().apply {
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_CENTER) != 0) {
      center = LatLng(value.latitude(), value.longitude())
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0) {
      centerAltitude = value.center_altitude()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_PADDING) != 0) {
      padding = edgeInsets(value.padding())
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ANCHOR) != 0) {
      anchor = screenPoint(value.anchor())
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ZOOM) != 0) {
      zoom = value.zoom()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_BEARING) != 0) {
      bearing = value.bearing()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_PITCH) != 0) {
      pitch = value.pitch()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ROLL) != 0) {
      roll = value.roll()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_FOV) != 0) {
      fieldOfView = value.field_of_view()
    }
  }
}

private fun boundOptions(value: MaplibreNativeC.mln_bound_options): BoundOptions {
  val fields = value.fields()
  return BoundOptions().apply {
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_BOUNDS) != 0) {
      bounds = BoundsConstraint.Bounded(latLngBounds(value.bounds()))
    } else if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_UNBOUNDED) != 0) {
      bounds = BoundsConstraint.Unbounded
    }
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM) != 0) {
      minZoom = value.min_zoom()
    }
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM) != 0) {
      maxZoom = value.max_zoom()
    }
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_MIN_PITCH) != 0) {
      minPitch = value.min_pitch()
    }
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_MAX_PITCH) != 0) {
      maxPitch = value.max_pitch()
    }
  }
}

private fun freeCameraOptions(value: MaplibreNativeC.mln_free_camera_options): FreeCameraOptions {
  val fields = value.fields()
  return FreeCameraOptions().apply {
    if ((fields and MaplibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION) != 0) {
      position = Vec3(value._position().x(), value._position().y(), value._position().z())
    }
    if ((fields and MaplibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0) {
      orientation =
        Quaternion(
          value.orientation().x(),
          value.orientation().y(),
          value.orientation().z(),
          value.orientation().w(),
        )
    }
  }
}

private class StringViewScope(value: String) : AutoCloseable {
  private val bytes: BytePointer
  val view: MaplibreNativeC.mln_buffer_view = MaplibreNativeC.mln_buffer_view()

  init {
    val utf8 = value.toByteArray(java.nio.charset.StandardCharsets.UTF_8)
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

private class StringViewArrayScope(values: List<String>) : AutoCloseable {
  private val strings: List<StringViewScope> = values.map(::StringViewScope)
  val views: MaplibreNativeC.mln_buffer_view? =
    if (strings.isEmpty()) null else MaplibreNativeC.mln_buffer_view(strings.size.toLong())
  val count: Long = strings.size.toLong()

  init {
    strings.forEachIndexed { index, string ->
      views?.position(index.toLong())?.put<MaplibreNativeC.mln_buffer_view>(string.view)
    }
    views?.position(0)
  }

  override fun close() {
    views?.close()
    strings.asReversed().forEach(StringViewScope::close)
  }
}

private class LatLngArrayScope : AutoCloseable {
  val coordinates: MaplibreNativeC.mln_lat_lng
  val count: Long

  constructor(values: List<LatLng>) {
    val coordinateSnapshot = values.toList()
    coordinates = MaplibreNativeC.mln_lat_lng(coordinateSnapshot.size.toLong())
    count = coordinateSnapshot.size.toLong()
    coordinateSnapshot.forEachIndexed { index, coordinate ->
      coordinates
        .position(index.toLong())
        .latitude(coordinate.latitude)
        .longitude(coordinate.longitude)
    }
    coordinates.position(0)
  }

  constructor(count: Long) {
    coordinates = MaplibreNativeC.mln_lat_lng(count)
    this.count = count
  }

  fun toList(count: Int): List<LatLng> =
    List(count) { index ->
        val coordinate = coordinates.position(index.toLong())
        LatLng(coordinate.latitude(), coordinate.longitude())
      }
      .also { coordinates.position(0) }

  override fun close() {
    coordinates.close()
  }
}

private class ScreenPointArrayScope : AutoCloseable {
  val points: MaplibreNativeC.mln_screen_point
  val count: Long

  constructor(values: List<ScreenPoint>) {
    val pointSnapshot = values.toList()
    points = MaplibreNativeC.mln_screen_point(pointSnapshot.size.toLong())
    count = pointSnapshot.size.toLong()
    pointSnapshot.forEachIndexed { index, point ->
      points.position(index.toLong()).x(point.x).y(point.y)
    }
    points.position(0)
  }

  constructor(count: Long) {
    points = MaplibreNativeC.mln_screen_point(count)
    this.count = count
  }

  fun toList(count: Int): List<ScreenPoint> =
    List(count) { index ->
        val point = points.position(index.toLong())
        ScreenPoint(point.x(), point.y())
      }
      .also { points.position(0) }

  override fun close() {
    points.close()
  }
}

private class ScreenPointScope(value: ScreenPoint?) : AutoCloseable {
  val point: MaplibreNativeC.mln_screen_point? = value?.let(::screenPoint)

  override fun close() {
    point?.close()
  }
}

private class CameraOptionsScope(value: CameraOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_camera_options = MaplibreNativeC.mln_camera_options_default()

  init {
    var fields = 0
    value.center?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_CENTER
      options.latitude(it.latitude).longitude(it.longitude)
    }
    value.centerAltitude?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE
      options.center_altitude(it)
    }
    value.padding?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_PADDING
      writeEdgeInsets(options.padding(), it)
    }
    value.anchor?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_ANCHOR
      options.anchor(screenPoint(it))
    }
    value.zoom?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_ZOOM
      options.zoom(it)
    }
    value.bearing?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_BEARING
      options.bearing(it)
    }
    value.pitch?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_PITCH
      options.pitch(it)
    }
    value.roll?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_ROLL
      options.roll(it)
    }
    value.fieldOfView?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_FOV
      options.field_of_view(it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class AnimationOptionsScope(value: AnimationOptions?) : AutoCloseable {
  val options: MaplibreNativeC.mln_animation_options? = value?.let {
    MaplibreNativeC.mln_animation_options_default()
  }

  init {
    if (value != null && options != null) {
      var fields = 0
      value.durationMs?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_DURATION
        options.duration_ms(it)
      }
      value.velocity?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_VELOCITY
        options.velocity(it)
      }
      value.minZoom?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_MIN_ZOOM
        options.min_zoom(it)
      }
      value.easing?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_EASING
        options.easing(MaplibreNativeC.mln_unit_bezier().x1(it.x1).y1(it.y1).x2(it.x2).y2(it.y2))
      }
      value.transitionId?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_TRANSITION_ID
        options.transition_id(it)
      }
      options.fields(fields)
    }
  }

  override fun close() {
    options?.close()
  }
}

private class CameraFitOptionsScope(value: CameraFitOptions?) : AutoCloseable {
  val options: MaplibreNativeC.mln_camera_fit_options? = value?.let {
    MaplibreNativeC.mln_camera_fit_options_default()
  }

  init {
    if (value != null && options != null) {
      var fields = 0
      value.padding?.let {
        fields = fields or MaplibreNativeC.MLN_CAMERA_FIT_OPTION_PADDING
        writeEdgeInsets(options.padding(), it)
      }
      value.bearing?.let {
        fields = fields or MaplibreNativeC.MLN_CAMERA_FIT_OPTION_BEARING
        options.bearing(it)
      }
      value.pitch?.let {
        fields = fields or MaplibreNativeC.MLN_CAMERA_FIT_OPTION_PITCH
        options.pitch(it)
      }
      options.fields(fields)
    }
  }

  override fun close() {
    options?.close()
  }
}

private class BoundOptionsScope(value: BoundOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_bound_options = MaplibreNativeC.mln_bound_options_default()

  init {
    var fields = 0
    when (val constraint = value.bounds) {
      is BoundsConstraint.Bounded -> {
        fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_BOUNDS
        options.bounds(latLngBounds(constraint.bounds))
      }
      BoundsConstraint.Unbounded -> {
        fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_UNBOUNDED
      }
      null -> {}
    }
    value.minZoom?.let {
      fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM
      options.min_zoom(it)
    }
    value.maxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM
      options.max_zoom(it)
    }
    value.minPitch?.let {
      fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_MIN_PITCH
      options.min_pitch(it)
    }
    value.maxPitch?.let {
      fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_MAX_PITCH
      options.max_pitch(it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class FreeCameraOptionsScope(value: FreeCameraOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_free_camera_options =
    MaplibreNativeC.mln_free_camera_options_default()

  init {
    var fields = 0
    value.position?.let {
      fields = fields or MaplibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION
      options._position(MaplibreNativeC.mln_vec3().x(it.x).y(it.y).z(it.z))
    }
    value.orientation?.let {
      fields = fields or MaplibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION
      options.orientation(MaplibreNativeC.mln_quaternion().x(it.x).y(it.y).z(it.z).w(it.w))
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class CanonicalTileIdScope(value: CanonicalTileId) : AutoCloseable {
  val tileId: MaplibreNativeC.mln_canonical_tile_id =
    MaplibreNativeC.mln_canonical_tile_id().z(value.z).x(value.x.toInt()).y(value.y.toInt())

  override fun close() {
    tileId.close()
  }
}

internal class CustomGeometrySourceState(private val options: CustomGeometrySourceOptions) :
  AutoCloseable {
  private val gate = CallbackGate("custom geometry callbacks", ::closeNative)
  // JavaCPP passes null for a null void* and drops the upcall if the Kotlin
  // override rejects it, so every parameter stays nullable.
  private val fetchTile =
    object : MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
      override fun call(userData: Pointer?, tileId: MaplibreNativeC.mln_canonical_tile_id?) {
        fetchTile(tileId)
      }
    }
  private val cancelTile =
    object : MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
      override fun call(userData: Pointer?, tileId: MaplibreNativeC.mln_canonical_tile_id?) {
        cancelTile(tileId)
      }
    }
  val descriptor: MaplibreNativeC.mln_custom_geometry_source_options =
    MaplibreNativeC.mln_custom_geometry_source_options_default()

  init {
    descriptor.fetch_tile(fetchTile)
    descriptor.cancel_tile(cancelTile)
    descriptor.user_data(AddressPointer(0))
    var fields = 0
    options.minZoom?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
      descriptor.min_zoom(it)
    }
    options.maxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
      descriptor.max_zoom(it)
    }
    options.tolerance?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
      descriptor.tolerance(it)
    }
    options.tileSize?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
      descriptor.tile_size(it)
    }
    options.buffer?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
      descriptor.buffer(it)
    }
    options.clip?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
      descriptor.clip(it)
    }
    options.wrap?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
      descriptor.wrap(it)
    }
    descriptor.fields(fields)
  }

  override fun close() {
    gate.close()
  }

  internal fun fetchTileForTesting(tileId: CanonicalTileId) {
    val lease = gate.enter() ?: return
    try {
      options.callback.fetchTile(tileId)
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      lease.close()
    }
  }

  internal fun isClosedForTesting(): Boolean = gate.isClosedForTesting()

  private fun fetchTile(tileId: MaplibreNativeC.mln_canonical_tile_id?) {
    if (tileId == null) return
    fetchTileForTesting(canonicalTileId(tileId))
  }

  private fun cancelTile(tileId: MaplibreNativeC.mln_canonical_tile_id?) {
    if (tileId == null) return
    val lease = gate.enter() ?: return
    try {
      options.callback.cancelTile(canonicalTileId(tileId))
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      lease.close()
    }
  }

  private fun closeNative() {
    descriptor.close()
    fetchTile.close()
    cancelTile.close()
  }

  private fun canonicalTileId(tileId: MaplibreNativeC.mln_canonical_tile_id): CanonicalTileId =
    CanonicalTileId(
      tileId.z(),
      Integer.toUnsignedLong(tileId.x()),
      Integer.toUnsignedLong(tileId.y()),
    )
}

private class TileSourceOptionsScope(value: TileSourceOptions?) : AutoCloseable {
  private val attribution: StringViewScope? = value?.attribution?.let(::StringViewScope)
  val options: MaplibreNativeC.mln_style_tile_source_options =
    MaplibreNativeC.mln_style_tile_source_options_default()

  init {
    var fields = 0
    value?.minZoom?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
      options.min_zoom(it)
    }
    value?.maxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
      options.max_zoom(it)
    }
    attribution?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
      options.attribution(it.view)
    }
    value?.scheme?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
      options.scheme(it.nativeValue)
    }
    value?.bounds?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
      options.bounds().southwest().latitude(it.southwest.latitude)
      options.bounds().southwest().longitude(it.southwest.longitude)
      options.bounds().northeast().latitude(it.northeast.latitude)
      options.bounds().northeast().longitude(it.northeast.longitude)
    }
    value?.tileSize?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
      options.tile_size(it)
    }
    value?.vectorEncoding?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
      options.vector_encoding(it.nativeValue)
    }
    value?.rasterDemEncoding?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
      options.raster_encoding(it.nativeValue)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
    attribution?.close()
  }
}

private class GeoJsonSourceOptionsScope(value: GeoJsonSourceOptions?) : AutoCloseable {
  private val clusterProperties: ByteArrayViewScope? =
    value?.clusterPropertiesTransit?.let(::ByteArrayViewScope)
  val options: MaplibreNativeC.mln_geojson_source_options =
    MaplibreNativeC.mln_geojson_source_options_default()

  init {
    var fields = 0
    value?.minZoom?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
      options.min_zoom(it)
    }
    value?.maxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
      options.max_zoom(it)
    }
    value?.tolerance?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
      options.tolerance(it)
    }
    value?.clusterMaxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
      options.cluster_max_zoom(it)
    }
    clusterProperties?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
      options.cluster_properties(it.view)
    }
    value?.tileSize?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
      options.tile_size(it)
    }
    value?.buffer?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_BUFFER
      options.buffer(it)
    }
    value?.clusterRadius?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
      options.cluster_radius(it)
    }
    value?.clusterMinPoints?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
      options.cluster_min_points(it)
    }
    value?.lineMetrics?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS
      options.line_metrics(it)
    }
    value?.cluster?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER
      options.cluster(it)
    }
    value?.synchronousUpdate?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE
      options.synchronous_update(it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
    clusterProperties?.close()
  }
}

private class ViewportOptionsScope(value: ViewportOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_map_viewport_options =
    MaplibreNativeC.mln_map_viewport_options_default()

  init {
    var fields = 0
    value.northOrientation?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown north orientation cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
      options.north_orientation(it.nativeValue)
    }
    value.constrainMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown constrain mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
      options.constrain_mode(it.nativeValue)
    }
    value.viewportMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown viewport mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
      options.viewport_mode(it.nativeValue)
    }
    value.frustumOffset?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
      writeEdgeInsets(options.frustum_offset(), it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class TileOptionsScope(value: TileOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_map_tile_options = MaplibreNativeC.mln_map_tile_options_default()

  init {
    var fields = 0
    value.prefetchZoomDelta?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
      options.prefetch_zoom_delta(it)
    }
    value.lodMinRadius?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
      options.lod_min_radius(it)
    }
    value.lodScale?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE
      options.lod_scale(it)
    }
    value.lodPitchThreshold?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
      options.lod_pitch_threshold(it)
    }
    value.lodZoomShift?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
      options.lod_zoom_shift(it)
    }
    value.lodMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown tile LOD mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE
      options.lod_mode(it.nativeValue)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class ProjectionModeOptionsScope(value: ProjectionModeOptions) : AutoCloseable {
  val mode: MaplibreNativeC.mln_projection_mode = MaplibreNativeC.mln_projection_mode_default()

  init {
    var fields = 0
    value.axonometric?.let {
      fields = fields or MaplibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC
      mode.axonometric(it)
    }
    value.xSkew?.let {
      fields = fields or MaplibreNativeC.MLN_PROJECTION_MODE_X_SKEW
      mode.x_skew(it)
    }
    value.ySkew?.let {
      fields = fields or MaplibreNativeC.MLN_PROJECTION_MODE_Y_SKEW
      mode.y_skew(it)
    }
    mode.fields(fields)
  }

  override fun close() {
    mode.close()
  }
}

private class PremultipliedImageScope(value: PremultipliedRgba8Image) : AutoCloseable {
  private val pixels: BytePointer
  val image: MaplibreNativeC.mln_premultiplied_rgba8_image =
    MaplibreNativeC.mln_premultiplied_rgba8_image_default()

  init {
    val bytes = value.pixels
    pixels = BytePointer(bytes.size.toLong())
    pixels.put(bytes, 0, bytes.size)
    image.width(value.width)
    image.height(value.height)
    image.stride(value.stride)
    image.pixels(pixels)
    image.byte_length(bytes.size.toLong())
  }

  override fun close() {
    image.close()
    pixels.close()
  }
}

private class StyleImageOptionsScope(value: StyleImageOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_style_image_options =
    MaplibreNativeC.mln_style_image_options_default()

  // Native borrows the stretch arrays for the call, so this scope owns them.
  private val stretchX: MaplibreNativeC.mln_image_stretch? = allocStretches(value.stretchX)
  private val stretchY: MaplibreNativeC.mln_image_stretch? = allocStretches(value.stretchY)

  init {
    var fields = 0
    value.pixelRatio?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
      options.pixel_ratio(it)
    }
    value.sdf?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_SDF
      options.sdf(it)
    }
    value.stretchX?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_STRETCH_X
      options.stretch_x(stretchX)
      options.stretch_x_count(it.size.toLong())
    }
    value.stretchY?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_STRETCH_Y
      options.stretch_y(stretchY)
      options.stretch_y_count(it.size.toLong())
    }
    value.content?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_CONTENT
      options.content().left(it.left).top(it.top).right(it.right).bottom(it.bottom)
    }
    value.textFitWidth?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
      options.text_fit_width(it.nativeValue)
    }
    value.textFitHeight?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
      options.text_fit_height(it.nativeValue)
    }
    options.fields(fields)
  }

  override fun close() {
    stretchX?.close()
    stretchY?.close()
    options.close()
  }
}

private class StyleTransitionOptionsScope(value: StyleTransitionOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_style_transition_options =
    MaplibreNativeC.mln_style_transition_options_default()

  init {
    var fields = 0
    value.enablePlacementTransitions?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
      options.enable_placement_transitions(it)
    }
    value.durationMs?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_DURATION
      options.duration_ms(it)
    }
    value.delayMs?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_DELAY
      options.delay_ms(it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private fun styleTransitionOptions(
  options: MaplibreNativeC.mln_style_transition_options
): StyleTransitionOptions =
  StyleTransitionOptions().apply {
    val fields = options.fields()
    durationMs =
      if (fields and MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_DURATION != 0) {
        options.duration_ms()
      } else {
        null
      }
    delayMs =
      if (fields and MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_DELAY != 0) {
        options.delay_ms()
      } else {
        null
      }
    enablePlacementTransitions =
      if (
        fields and MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS != 0
      ) {
        options.enable_placement_transitions()
      } else {
        null
      }
  }

private fun allocStretches(stretches: List<ImageStretch>?): MaplibreNativeC.mln_image_stretch? {
  if (stretches.isNullOrEmpty()) return null
  val array = MaplibreNativeC.mln_image_stretch(stretches.size.toLong())
  stretches.forEachIndexed { index, stretch ->
    array.position(index.toLong()).from(stretch.from).to(stretch.to)
  }
  return array.position(0)
}

private class MapOptionsScope(value: MapOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_map_options = MaplibreNativeC.mln_map_options_default()

  init {
    value.width?.let { options.width(it) }
    value.height?.let { options.height(it) }
    value.scaleFactor?.let { options.scale_factor(it) }
    value.mapMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown map mode cannot be used as input: ${it.nativeValue}"
      }
      options.map_mode(it.nativeValue)
    }
    value.fastPforEnabled?.let { options.fast_pfor_enabled(it) }
  }

  override fun close() {
    options.close()
  }
}

private fun releaseCallbackRoot(root: AutoCloseable?) {
  HandleLeakCleaner.releaseNativeCallbackRoot(root)
  closeQuietly(root)
}

private fun closeQuietly(closeable: AutoCloseable?) {
  try {
    closeable?.close()
  } catch (_: Exception) {}
}

/** A `void*` built from a raw address, for backend-native pointers and user data. */
private class AddressPointer(address: Long) : Pointer(null as Pointer?) {
  init {
    this.address = address
  }
}

/** Direct test seam for the JavaCPP map and style descriptor adapter. */
internal object JavaCppMapStructs {
  fun stringViewRoundTrip(value: String): String =
    StringViewScope(value).use { stringView(it.view) }

  fun cameraOptionsRoundTrip(value: CameraOptions): Pair<Int, CameraOptions> =
    CameraOptionsScope(value).use { it.options.fields() to cameraOptions(it.options) }

  fun animationOptionsSnapshot(value: AnimationOptions): AnimationOptionsSnapshot =
    AnimationOptionsScope(value).use {
      val options = requireNotNull(it.options)
      AnimationOptionsSnapshot(
        options.fields(),
        options.duration_ms(),
        options.velocity(),
        options.transition_id(),
      )
    }

  fun viewportOptionsRoundTrip(value: ViewportOptions): ViewportOptions =
    ViewportOptionsScope(value).use { viewportOptions(it.options) }

  fun tileOptionsRoundTrip(value: TileOptions): TileOptions =
    TileOptionsScope(value).use { tileOptions(it.options) }

  fun projectionModeOptionsRoundTrip(value: ProjectionModeOptions): ProjectionModeOptions =
    ProjectionModeOptionsScope(value).use { projectionModeOptions(it.mode) }

  fun jsonRoundTrip(value: JsonValue): JsonValue = JsonScope(value).use { jsonValue(it.value) }

  fun geoJsonType(value: GeoJson): Int = GeoJsonScope(value).use { it.value.type() }

  fun styleImageOptionsSnapshot(value: StyleImageOptions): StyleImageOptionsSnapshot =
    StyleImageOptionsScope(value).use {
      StyleImageOptionsSnapshot(it.options.fields(), it.options.pixel_ratio(), it.options.sdf())
    }

  fun styleImageInfoSnapshot(byteLength: Long): StyleImageInfo =
    MaplibreNativeC.mln_style_image_info().use {
      it.width(2).height(3).stride(8).byte_length(byteLength).pixel_ratio(2.0f).sdf(true)
      styleImageInfo(it)
    }

  fun sourceInfoSnapshot(type: Int, volatileSource: Boolean): SourceInfo =
    MaplibreNativeC.mln_style_source_info().use {
      it.type(type).is_volatile(volatileSource)
      SourceInfo(
        SourceType.fromNative(it.type()),
        it.is_volatile(),
        null,
        null,
        null,
        null,
        null,
        null,
      )
    }

  fun styleStringListCleanupAfterCopyFailure(): Int {
    var destroys = 0
    try {
      styleStringList(
        1L,
        counter = { _, outCount ->
          outCount.put(Long.MAX_VALUE)
          org.maplibre.nativeffi.error.MaplibreStatus.OK.nativeCode
        },
        getter = { _, _, _ -> org.maplibre.nativeffi.error.MaplibreStatus.OK.nativeCode },
        destroyer = { destroys++ },
      )
    } catch (_: ArithmeticException) {
      return destroys
    }
    error("style list conversion unexpectedly succeeded")
  }

  fun mapOptionsSnapshot(value: MapOptions): MapOptionsSnapshot =
    MapOptionsScope(value).use {
      MapOptionsSnapshot(
        it.options.width(),
        it.options.height(),
        it.options.scale_factor(),
        it.options.map_mode(),
        it.options.fast_pfor_enabled(),
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
}
