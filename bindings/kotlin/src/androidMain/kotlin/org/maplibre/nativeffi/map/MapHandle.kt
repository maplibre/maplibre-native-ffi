package org.maplibre.nativeffi.map

import org.bytedeco.javacpp.BoolPointer
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.bytedeco.javacpp.PointerPointer
import org.bytedeco.javacpp.SizeTPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.json.JsonValue
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
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Owned Android JNI map handle. */
public actual class MapHandle
private constructor(private val runtime: RuntimeHandle, private val handleAddress: Long) :
  AutoCloseable {
  private val runtimeRetention = runtime.retainChild()
  private val core = HandleStateCore("MapHandle", handleAddress)
  private val customGeometrySources = mutableMapOf<String, CustomGeometrySourceState>()

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  public actual fun setStyleUrl(url: String) {
    NativeAccess.ensureLoaded()
    optionalCString(url).use { nativeUrl ->
      Status.check(MaplibreNativeC.mln_map_set_style_url(map(requireLiveAddress()), nativeUrl))
    }
  }

  public actual fun setStyleJson(json: String) {
    NativeAccess.ensureLoaded()
    optionalCString(json).use { nativeJson ->
      Status.check(MaplibreNativeC.mln_map_set_style_json(map(requireLiveAddress()), nativeJson))
    }
    clearCustomGeometrySources()
  }

  public actual fun addStyleSourceJson(sourceId: String, sourceJson: JsonValue) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      JsonScope(sourceJson).use { nativeSourceJson ->
        Status.check(
          MaplibreNativeC.mln_map_add_style_source_json(
            map(requireLiveAddress()),
            nativeSourceId.view,
            nativeSourceJson.value,
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
          map(requireLiveAddress()),
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
          map(requireLiveAddress()),
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
          map(requireLiveAddress()),
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
            map(requireLiveAddress()),
            nativeSourceId.view,
            outInfo,
            outFound,
          )
        )
        if (!outFound[0]) return null
        val attribution =
          if (outInfo.has_attribution())
            copyStyleSourceAttribution(requireLiveAddress(), nativeSourceId, outInfo)
          else null
        return SourceInfo(SourceType.fromNative(outInfo.type()), outInfo.is_volatile(), attribution)
      }
    }
  }

  public actual fun styleSourceIds(): List<String> {
    NativeAccess.ensureLoaded()
    PointerPointer<MaplibreNativeC.mln_style_id_list>(1).use { outList ->
      outList.put(0, null as Pointer?)
      Status.check(
        MaplibreNativeC.mln_map_list_style_source_ids(map(requireLiveAddress()), outList)
      )
      val list = outList.get(MaplibreNativeC.mln_style_id_list::class.java, 0)
      return styleIdList(requireNotNull(list))
    }
  }

  public actual fun addGeoJsonSourceUrl(sourceId: String, url: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewScope(url).use { nativeUrl ->
        Status.check(
          MaplibreNativeC.mln_map_add_geojson_source_url(
            map(requireLiveAddress()),
            nativeSourceId.view,
            nativeUrl.view,
          )
        )
      }
    }
  }

  public actual fun addGeoJsonSourceData(sourceId: String, data: GeoJson) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      GeoJsonScope(data).use { nativeData ->
        Status.check(
          MaplibreNativeC.mln_map_add_geojson_source_data(
            map(requireLiveAddress()),
            nativeSourceId.view,
            nativeData.value,
          )
        )
      }
    }
  }

  public actual fun setGeoJsonSourceUrl(sourceId: String, url: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewScope(url).use { nativeUrl ->
        Status.check(
          MaplibreNativeC.mln_map_set_geojson_source_url(
            map(requireLiveAddress()),
            nativeSourceId.view,
            nativeUrl.view,
          )
        )
      }
    }
  }

  public actual fun setGeoJsonSourceData(sourceId: String, data: GeoJson) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      GeoJsonScope(data).use { nativeData ->
        Status.check(
          MaplibreNativeC.mln_map_set_geojson_source_data(
            map(requireLiveAddress()),
            nativeSourceId.view,
            nativeData.value,
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
    try {
      StringViewScope(sourceId).use { nativeSourceId ->
        Status.check(
          MaplibreNativeC.mln_map_add_custom_geometry_source(
            map(requireLiveAddress()),
            nativeSourceId.view,
            sourceState.descriptor,
          )
        )
      }
      closeQuietly(customGeometrySources.put(sourceId, sourceState))
    } catch (error: Throwable) {
      closeQuietly(sourceState)
      throw error
    }
  }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: GeoJson,
  ) {
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      CanonicalTileIdScope(tileId).use { nativeTileId ->
        GeoJsonScope(data).use { nativeData ->
          Status.check(
            MaplibreNativeC.mln_map_set_custom_geometry_source_tile_data(
              map(requireLiveAddress()),
              nativeSourceId.view,
              nativeTileId.tileId,
              nativeData.value,
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
            map(requireLiveAddress()),
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
          map(requireLiveAddress()),
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
              map(requireLiveAddress()),
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
          map(requireLiveAddress()),
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
          map(requireLiveAddress()),
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
            map(requireLiveAddress()),
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
            map(requireLiveAddress()),
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
              map(requireLiveAddress()),
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
              map(requireLiveAddress()),
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
            map(requireLiveAddress()),
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
            map(requireLiveAddress()),
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
            map(requireLiveAddress()),
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
              map(requireLiveAddress()),
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

  public actual fun addStyleLayerJson(layerJson: JsonValue, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    JsonScope(layerJson).use { nativeLayerJson ->
      StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_add_style_layer_json(
            map(requireLiveAddress()),
            nativeLayerJson.value,
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
              map(requireLiveAddress()),
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
              map(requireLiveAddress()),
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
            map(requireLiveAddress()),
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
            map(requireLiveAddress()),
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
          map(requireLiveAddress()),
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
          map(requireLiveAddress()),
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
            map(requireLiveAddress()),
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
          map(requireLiveAddress()),
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
          map(requireLiveAddress()),
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
    MaplibreNativeC.mln_string_view().use { outType ->
      StringViewScope(layerId).use { nativeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_get_style_layer_type(
            map(requireLiveAddress()),
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
    PointerPointer<MaplibreNativeC.mln_style_id_list>(1).use { outList ->
      outList.put(0, null as Pointer?)
      Status.check(MaplibreNativeC.mln_map_list_style_layer_ids(map(requireLiveAddress()), outList))
      val list = outList.get(MaplibreNativeC.mln_style_id_list::class.java, 0)
      return styleIdList(requireNotNull(list))
    }
  }

  public actual fun moveStyleLayer(layerId: String, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_move_style_layer(
            map(requireLiveAddress()),
            nativeLayerId.view,
            nativeBeforeLayerId.view,
          )
        )
      }
    }
  }

  public actual fun styleLayerJson(layerId: String): JsonValue? {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      PointerPointer<Pointer>(1).use { outSnapshot ->
        BoolPointer(1).use { outFound ->
          outSnapshot.put(0, null as Pointer?)
          Status.check(
            MaplibreNativeC.mln_map_get_style_layer_json(
              map(requireLiveAddress()),
              nativeLayerId.view,
              outSnapshot,
              outFound,
            )
          )
          return if (outFound.get()) jsonSnapshot(outSnapshot) else null
        }
      }
    }
  }

  public actual fun setStyleLightJson(lightJson: JsonValue) {
    NativeAccess.ensureLoaded()
    JsonScope(lightJson).use { nativeLightJson ->
      Status.check(
        MaplibreNativeC.mln_map_set_style_light_json(
          map(requireLiveAddress()),
          nativeLightJson.value,
        )
      )
    }
  }

  public actual fun setStyleLightProperty(propertyName: String, value: JsonValue) {
    NativeAccess.ensureLoaded()
    StringViewScope(propertyName).use { nativePropertyName ->
      JsonScope(value).use { nativeValue ->
        Status.check(
          MaplibreNativeC.mln_map_set_style_light_property(
            map(requireLiveAddress()),
            nativePropertyName.view,
            nativeValue.value,
          )
        )
      }
    }
  }

  public actual fun styleLightProperty(propertyName: String): JsonValue? {
    NativeAccess.ensureLoaded()
    StringViewScope(propertyName).use { nativePropertyName ->
      PointerPointer<Pointer>(1).use { outSnapshot ->
        outSnapshot.put(0, null as Pointer?)
        Status.check(
          MaplibreNativeC.mln_map_get_style_light_property(
            map(requireLiveAddress()),
            nativePropertyName.view,
            outSnapshot,
          )
        )
        return jsonSnapshot(outSnapshot)
      }
    }
  }

  public actual fun setLayerProperty(layerId: String, propertyName: String, value: JsonValue) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(propertyName).use { nativePropertyName ->
        JsonScope(value).use { nativeValue ->
          Status.check(
            MaplibreNativeC.mln_map_set_layer_property(
              map(requireLiveAddress()),
              nativeLayerId.view,
              nativePropertyName.view,
              nativeValue.value,
            )
          )
        }
      }
    }
  }

  public actual fun layerProperty(layerId: String, propertyName: String): JsonValue? {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(propertyName).use { nativePropertyName ->
        PointerPointer<Pointer>(1).use { outSnapshot ->
          outSnapshot.put(0, null as Pointer?)
          Status.check(
            MaplibreNativeC.mln_map_get_layer_property(
              map(requireLiveAddress()),
              nativeLayerId.view,
              nativePropertyName.view,
              outSnapshot,
            )
          )
          return jsonSnapshot(outSnapshot)
        }
      }
    }
  }

  public actual fun setLayerFilter(layerId: String, filter: JsonValue) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      JsonScope(filter).use { nativeFilter ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_filter(
            map(requireLiveAddress()),
            nativeLayerId.view,
            nativeFilter.value,
          )
        )
      }
    }
  }

  public actual fun clearLayerFilter(layerId: String) {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_layer_filter(
          map(requireLiveAddress()),
          nativeLayerId.view,
          null,
        )
      )
    }
  }

  public actual fun layerFilter(layerId: String): JsonValue? {
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      PointerPointer<Pointer>(1).use { outSnapshot ->
        outSnapshot.put(0, null as Pointer?)
        Status.check(
          MaplibreNativeC.mln_map_get_layer_filter(
            map(requireLiveAddress()),
            nativeLayerId.view,
            outSnapshot,
          )
        )
        return jsonSnapshot(outSnapshot)
      }
    }
  }

  public actual fun requestRepaint() {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_request_repaint(map(requireLiveAddress())))
  }

  public actual fun requestStillImage() {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_request_still_image(map(requireLiveAddress())))
  }

  public actual var debugOptions: Set<DebugOption>
    get() {
      NativeAccess.ensureLoaded()
      val outOptions = intArrayOf(0)
      Status.check(MaplibreNativeC.mln_map_get_debug_options(map(requireLiveAddress()), outOptions))
      return debugOptions(outOptions[0])
    }
    set(value) {
      NativeAccess.ensureLoaded()
      Status.check(
        MaplibreNativeC.mln_map_set_debug_options(map(requireLiveAddress()), debugOptionMask(value))
      )
    }

  public actual var isRenderingStatsViewEnabled: Boolean
    get() {
      NativeAccess.ensureLoaded()
      val outEnabled = booleanArrayOf(false)
      Status.check(
        MaplibreNativeC.mln_map_get_rendering_stats_view_enabled(
          map(requireLiveAddress()),
          outEnabled,
        )
      )
      return outEnabled[0]
    }
    set(value) {
      NativeAccess.ensureLoaded()
      Status.check(
        MaplibreNativeC.mln_map_set_rendering_stats_view_enabled(map(requireLiveAddress()), value)
      )
    }

  public actual val isFullyLoaded: Boolean
    get() {
      NativeAccess.ensureLoaded()
      val outLoaded = booleanArrayOf(false)
      Status.check(MaplibreNativeC.mln_map_is_fully_loaded(map(requireLiveAddress()), outLoaded))
      return outLoaded[0]
    }

  public actual fun dumpDebugLogs() {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_dump_debug_logs(map(requireLiveAddress())))
  }

  public actual var viewportOptions: ViewportOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_map_viewport_options_default().use { outOptions ->
        Status.check(
          MaplibreNativeC.mln_map_get_viewport_options(map(requireLiveAddress()), outOptions)
        )
        return viewportOptions(outOptions)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      ViewportOptionsScope(value).use { nativeOptions ->
        Status.check(
          MaplibreNativeC.mln_map_set_viewport_options(
            map(requireLiveAddress()),
            nativeOptions.options,
          )
        )
      }
    }

  public actual var tileOptions: TileOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_map_tile_options_default().use { outOptions ->
        Status.check(
          MaplibreNativeC.mln_map_get_tile_options(map(requireLiveAddress()), outOptions)
        )
        return tileOptions(outOptions)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      TileOptionsScope(value).use { nativeOptions ->
        Status.check(
          MaplibreNativeC.mln_map_set_tile_options(map(requireLiveAddress()), nativeOptions.options)
        )
      }
    }

  public actual val camera: CameraOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_camera_options_default().use { outCamera ->
        Status.check(MaplibreNativeC.mln_map_get_camera(map(requireLiveAddress()), outCamera))
        return cameraOptions(outCamera)
      }
    }

  public actual fun jumpTo(camera: CameraOptions) {
    NativeAccess.ensureLoaded()
    CameraOptionsScope(camera).use { nativeCamera ->
      Status.check(MaplibreNativeC.mln_map_jump_to(map(requireLiveAddress()), nativeCamera.options))
    }
  }

  public actual fun easeTo(camera: CameraOptions, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    CameraOptionsScope(camera).use { nativeCamera ->
      AnimationOptionsScope(animation).use { nativeAnimation ->
        Status.check(
          MaplibreNativeC.mln_map_ease_to(
            map(requireLiveAddress()),
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
            map(requireLiveAddress()),
            nativeCamera.options,
            nativeAnimation.options,
          )
        )
      }
    }
  }

  public actual fun moveBy(deltaX: Double, deltaY: Double) {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_move_by(map(requireLiveAddress()), deltaX, deltaY))
  }

  public actual fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    AnimationOptionsScope(animation).use { nativeAnimation ->
      Status.check(
        MaplibreNativeC.mln_map_move_by_animated(
          map(requireLiveAddress()),
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
      Status.check(
        MaplibreNativeC.mln_map_scale_by(map(requireLiveAddress()), scale, nativeAnchor.point)
      )
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
            map(requireLiveAddress()),
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
        map(requireLiveAddress()),
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
          map(requireLiveAddress()),
          screenPoint(first),
          screenPoint(second),
          nativeAnimation.options,
        )
      )
    }
  }

  public actual fun pitchBy(pitch: Double) {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_pitch_by(map(requireLiveAddress()), pitch))
  }

  public actual fun pitchByAnimated(pitch: Double, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    AnimationOptionsScope(animation).use { nativeAnimation ->
      Status.check(
        MaplibreNativeC.mln_map_pitch_by_animated(
          map(requireLiveAddress()),
          pitch,
          nativeAnimation.options,
        )
      )
    }
  }

  public actual fun cancelTransitions() {
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_cancel_transitions(map(requireLiveAddress())))
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
            map(requireLiveAddress()),
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
              map(requireLiveAddress()),
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
    geometry: Geometry,
    fitOptions: CameraFitOptions?,
  ): CameraOptions {
    NativeAccess.ensureLoaded()
    GeometryScope(geometry).use { nativeGeometry ->
      CameraFitOptionsScope(fitOptions).use { nativeFitOptions ->
        MaplibreNativeC.mln_camera_options_default().use { outCamera ->
          Status.check(
            MaplibreNativeC.mln_map_camera_for_geometry(
              map(requireLiveAddress()),
              nativeGeometry.value,
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
        Status.check(MaplibreNativeC.mln_map_get_bounds(map(requireLiveAddress()), outOptions))
        return boundOptions(outOptions)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      BoundOptionsScope(value).use { nativeOptions ->
        Status.check(
          MaplibreNativeC.mln_map_set_bounds(map(requireLiveAddress()), nativeOptions.options)
        )
      }
    }

  public actual var freeCameraOptions: FreeCameraOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_free_camera_options_default().use { outOptions ->
        Status.check(
          MaplibreNativeC.mln_map_get_free_camera_options(map(requireLiveAddress()), outOptions)
        )
        return freeCameraOptions(outOptions)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      FreeCameraOptionsScope(value).use { nativeOptions ->
        Status.check(
          MaplibreNativeC.mln_map_set_free_camera_options(
            map(requireLiveAddress()),
            nativeOptions.options,
          )
        )
      }
    }

  public actual var projectionMode: ProjectionModeOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_projection_mode_default().use { outMode ->
        Status.check(
          MaplibreNativeC.mln_map_get_projection_mode(map(requireLiveAddress()), outMode)
        )
        return projectionModeOptions(outMode)
      }
    }
    set(value) {
      NativeAccess.ensureLoaded()
      ProjectionModeOptionsScope(value).use { nativeMode ->
        Status.check(
          MaplibreNativeC.mln_map_set_projection_mode(map(requireLiveAddress()), nativeMode.mode)
        )
      }
    }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_screen_point().use { outPoint ->
      Status.check(
        MaplibreNativeC.mln_map_pixel_for_lat_lng(
          map(requireLiveAddress()),
          latLng(coordinate),
          outPoint,
        )
      )
      return screenPoint(outPoint)
    }
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_lat_lng().use { outCoordinate ->
      Status.check(
        MaplibreNativeC.mln_map_lat_lng_for_pixel(
          map(requireLiveAddress()),
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
      Status.check(
        MaplibreNativeC.mln_map_pixels_for_lat_lngs(map(requireLiveAddress()), null, 0L, null)
      )
      return emptyList()
    }
    LatLngArrayScope(coordinateSnapshot).use { nativeCoordinates ->
      ScreenPointArrayScope(nativeCoordinates.count).use { outPoints ->
        Status.check(
          MaplibreNativeC.mln_map_pixels_for_lat_lngs(
            map(requireLiveAddress()),
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
      Status.check(
        MaplibreNativeC.mln_map_lat_lngs_for_pixels(map(requireLiveAddress()), null, 0L, null)
      )
      return emptyList()
    }
    ScreenPointArrayScope(pointSnapshot).use { nativePoints ->
      LatLngArrayScope(nativePoints.count).use { outCoordinates ->
        Status.check(
          MaplibreNativeC.mln_map_lat_lngs_for_pixels(
            map(requireLiveAddress()),
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
    PointerPointer<MaplibreNativeC.mln_map_projection>(1).use { outProjection ->
      outProjection.put(0, null as Pointer?)
      Status.check(
        MaplibreNativeC.mln_map_projection_create(map(requireLiveAddress()), outProjection)
      )
      val projection = outProjection.get(MaplibreNativeC.mln_map_projection::class.java, 0)
      val address = if (projection == null || projection.isNull) 0L else projection.address()
      require(address != 0L) { "mln_map_projection_create returned a null projection" }
      return MapProjectionHandle(address)
    }
  }

  public actual override fun close() {
    core.closeOnce(
      destroy = { MaplibreNativeC.mln_map_destroy(map(handleAddress)) },
      afterSuccess = {
        clearCustomGeometrySources()
        runtime.unregisterMap(this)
        runtimeRetention.close()
      },
    )
  }

  internal fun nativeAddress(): Long = handleAddress

  internal fun retainChild(): HandleStateCore.ChildRetention = core.retainChild()

  internal fun releaseDetachedCustomGeometrySources() {
    val iterator = customGeometrySources.iterator()
    while (iterator.hasNext()) {
      val entry = iterator.next()
      if (styleSourceType(entry.key) != SourceType.CUSTOM_VECTOR) {
        closeQuietly(entry.value)
        iterator.remove()
      }
    }
  }

  private fun addTileSourceUrl(
    function:
      (
        MaplibreNativeC.mln_map,
        MaplibreNativeC.mln_string_view,
        MaplibreNativeC.mln_string_view,
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
              map(requireLiveAddress()),
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
        MaplibreNativeC.mln_map,
        MaplibreNativeC.mln_string_view,
        MaplibreNativeC.mln_string_view?,
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
              map(requireLiveAddress()),
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
    function:
      (
        MaplibreNativeC.mln_map,
        MaplibreNativeC.mln_camera_options,
        MaplibreNativeC.mln_lat_lng_bounds,
      ) -> Int,
    camera: CameraOptions,
  ): LatLngBounds {
    NativeAccess.ensureLoaded()
    CameraOptionsScope(camera).use { nativeCamera ->
      MaplibreNativeC.mln_lat_lng_bounds().use { outBounds ->
        Status.check(function(map(requireLiveAddress()), nativeCamera.options, outBounds))
        return latLngBounds(outBounds)
      }
    }
  }

  public actual companion object {
    public actual fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle {
      NativeAccess.ensureLoaded()
      MapOptionsScope(options).use { nativeOptions ->
        PointerPointer<MaplibreNativeC.mln_map>(1).use { outMap ->
          outMap.put(0, null as Pointer?)
          Status.check(
            MaplibreNativeC.mln_map_create(
              runtime(runtime.nativeAddress()),
              nativeOptions.options,
              outMap,
            )
          )
          val map = outMap.get(MaplibreNativeC.mln_map::class.java, 0)
          val address = if (map == null || map.isNull) 0L else map.address()
          require(address != 0L) { "mln_map_create returned a null map" }
          return MapHandle(runtime, address).also { runtime.registerMap(it) }
        }
      }
    }
  }

  private fun requireLiveAddress(): Long {
    core.requireLive()
    return handleAddress
  }

  private fun closeCustomGeometrySource(sourceId: String) {
    closeQuietly(customGeometrySources.remove(sourceId))
  }

  private fun clearCustomGeometrySources() {
    customGeometrySources.values.forEach(::closeQuietly)
    customGeometrySources.clear()
  }
}

private fun map(address: Long): MaplibreNativeC.mln_map =
  MaplibreNativeC.mln_map(AddressPointer(address))

private fun runtime(address: Long): MaplibreNativeC.mln_runtime =
  MaplibreNativeC.mln_runtime(AddressPointer(address))

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

private fun stringView(view: MaplibreNativeC.mln_string_view): String {
  if (view.size() == 0L || view.data() == null || view.data().isNull) return ""
  val bytes = ByteArray(Math.toIntExact(view.size()))
  view.data().get(bytes, 0, bytes.size)
  return String(bytes, java.nio.charset.StandardCharsets.UTF_8)
}

private fun styleIdList(list: MaplibreNativeC.mln_style_id_list): List<String> =
  try {
    SizeTPointer(1).use { outCount ->
      Status.check(MaplibreNativeC.mln_style_id_list_count(list, outCount))
      List(Math.toIntExact(outCount.get())) { index ->
        MaplibreNativeC.mln_string_view().use { outId ->
          Status.check(MaplibreNativeC.mln_style_id_list_get(list, index.toLong(), outId))
          stringView(outId)
        }
      }
    }
  } finally {
    MaplibreNativeC.mln_style_id_list_destroy(list)
  }

private fun copyStyleSourceAttribution(
  mapAddress: Long,
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
          map(mapAddress),
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

private fun styleImageInfo(info: MaplibreNativeC.mln_style_image_info): StyleImageInfo =
  StyleImageInfo(
    info.width(),
    info.height(),
    info.stride(),
    info.byte_length(),
    info.pixel_ratio(),
    info.sdf(),
  )

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
      bounds = latLngBounds(value.bounds())
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
  val view: MaplibreNativeC.mln_string_view = MaplibreNativeC.mln_string_view()

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
  val views: MaplibreNativeC.mln_string_view? =
    if (strings.isEmpty()) null else MaplibreNativeC.mln_string_view(strings.size.toLong())
  val count: Long = strings.size.toLong()

  init {
    strings.forEachIndexed { index, string ->
      views?.position(index.toLong())?.put<MaplibreNativeC.mln_string_view>(string.view)
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
    value.bounds?.let {
      fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_BOUNDS
      options.bounds(latLngBounds(it))
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

internal class GeometryScope(geometry: Geometry) : AutoCloseable {
  private val scope = GeoDescriptorScope()
  val value: MaplibreNativeC.mln_geometry = scope.geometry(geometry, 0)

  override fun close() {
    scope.close()
  }
}

private class GeoJsonScope(geoJson: GeoJson) : AutoCloseable {
  private val scope = GeoDescriptorScope()
  val value: MaplibreNativeC.mln_geojson = scope.geoJson(geoJson)

  override fun close() {
    scope.close()
  }
}

private class GeoDescriptorScope : AutoCloseable {
  private val owned = mutableListOf<Pointer>()
  private val strings = mutableListOf<StringViewScope>()
  private val jsonValues = mutableListOf<JsonScope>()

  override fun close() {
    jsonValues.asReversed().forEach(JsonScope::close)
    owned.asReversed().forEach(Pointer::close)
    strings.asReversed().forEach(StringViewScope::close)
  }

  fun geometry(value: Geometry, depth: Int): MaplibreNativeC.mln_geometry {
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

  fun geoJson(value: GeoJson): MaplibreNativeC.mln_geojson {
    val out = own(MaplibreNativeC.mln_geojson())
    out.size(out.sizeof())
    when (value) {
      is GeoJson.GeometryValue ->
        out
          .type(MaplibreNativeC.MLN_GEOJSON_TYPE_GEOMETRY)
          .data_geometry(geometry(value.geometry, 0))
      is GeoJson.FeatureValue ->
        out.type(MaplibreNativeC.MLN_GEOJSON_TYPE_FEATURE).data_feature(feature(value.feature, 0))
      is GeoJson.FeatureCollection -> {
        out.type(MaplibreNativeC.MLN_GEOJSON_TYPE_FEATURE_COLLECTION)
        val collection = own(MaplibreNativeC.mln_feature_collection())
        if (value.features.isNotEmpty()) {
          val nativeFeatures = own(MaplibreNativeC.mln_feature(value.features.size.toLong()))
          value.features.forEachIndexed { index, sourceFeature ->
            nativeFeatures
              .position(index.toLong())
              .put<MaplibreNativeC.mln_feature>(feature(sourceFeature, 1))
          }
          nativeFeatures.position(0)
          collection.features(nativeFeatures)
        }
        collection.feature_count(value.features.size.toLong())
        out.data_feature_collection(collection)
      }
    }
    return out
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

  private fun feature(value: Feature, depth: Int): MaplibreNativeC.mln_feature {
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
}

private class CanonicalTileIdScope(value: CanonicalTileId) : AutoCloseable {
  val tileId: MaplibreNativeC.mln_canonical_tile_id =
    MaplibreNativeC.mln_canonical_tile_id().z(value.z).x(value.x.toInt()).y(value.y.toInt())

  override fun close() {
    tileId.close()
  }
}

private class CustomGeometrySourceState(private val options: CustomGeometrySourceOptions) :
  AutoCloseable {
  private val gate = CallbackGate("custom geometry callbacks", ::closeNative)
  private val fetchTile =
    object : MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
      override fun call(userData: Pointer, tileId: MaplibreNativeC.mln_canonical_tile_id) {
        fetchTile(tileId)
      }
    }
  private val cancelTile =
    object : MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
      override fun call(userData: Pointer, tileId: MaplibreNativeC.mln_canonical_tile_id) {
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

  private fun fetchTile(tileId: MaplibreNativeC.mln_canonical_tile_id) {
    val lease = gate.enter() ?: return
    try {
      options.callback.fetchTile(canonicalTileId(tileId))
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      lease.close()
    }
  }

  private fun cancelTile(tileId: MaplibreNativeC.mln_canonical_tile_id) {
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
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
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
  }

  override fun close() {
    options.close()
  }
}

private class AddressPointer(address: Long) : Pointer(null as Pointer?) {
  init {
    this.address = address
  }
}

private fun closeQuietly(closeable: AutoCloseable?) {
  try {
    closeable?.close()
  } catch (_: Exception) {}
}
