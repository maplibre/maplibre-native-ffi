package org.maplibre.nativeffi.map

import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraSnapshot
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.async.mapHandleDeferred
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.internal.loader.CompletionBridge
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.render.RenderSessionAttachOptions
import org.maplibre.nativeffi.render.RenderSessionAttachment
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.CommandCompletion
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.CustomMvtVectorSourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceDataHandle
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.ImageStretch
import org.maplibre.nativeffi.style.LayerInfo
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Owned JVM FFM map handle. */
public actual class MapHandle
private constructor(private val runtime: RuntimeHandle, private val handle: NativeMap) {
  private val core = HandleStateCore("MapHandle", handle.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private val customGeometrySources =
    CustomGeometrySourceRegistry<CustomGeometrySourceState>(::releaseCallbackRoot)
  private val customMvtVectorSources =
    CustomGeometrySourceRegistry<CustomMvtVectorSourceState>(::releaseCallbackRoot)

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  public actual fun setEventMask(value: RuntimeEventMask): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setMapEventMask(requireLiveHandle(), value.nativeValue)
  }

  public actual fun setStyleUrl(url: String): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setMapStyleUrl(requireLiveHandle(), url)
  }

  public actual fun setStyleJson(json: ByteArray): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setMapStyleJson(requireLiveHandle(), json)
  }

  public actual fun setFeatureState(
    selector: FeatureStateSelector,
    value: ByteArray,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setMapFeatureState(requireLiveHandle(), selector, value)
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray> {
    NativeAccess.ensureLoaded()
    return NativeAccess.getMapFeatureState(requireLiveHandle(), selector)
  }

  public actual fun removeFeatureState(
    selector: FeatureStateSelector
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeMapFeatureState(requireLiveHandle(), selector)
  }

  public actual fun loadedStyleJson(): Deferred<ByteArray> {
    NativeAccess.ensureLoaded()
    return NativeAccess.loadedStyleJson(requireLiveHandle())
  }

  public actual fun styleUrl(): Deferred<String> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleUrl(requireLiveHandle())
  }

  public actual fun addStyleSourceJson(
    sourceId: String,
    sourceJson: ByteArray,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addStyleSourceJson(requireLiveHandle(), sourceId, sourceJson)
  }

  public actual fun removeStyleSource(sourceId: String): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeStyleSource(requireLiveHandle(), sourceId)
  }

  public actual fun styleSourceInfo(sourceId: String): Deferred<SourceInfo?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceInfo(requireLiveHandle(), sourceId)
  }

  public actual fun setStyleSourceVolatile(
    sourceId: String,
    isVolatile: Boolean,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setStyleSourceVolatile(requireLiveHandle(), sourceId, isVolatile)
  }

  public actual fun styleSourceAttribution(sourceId: String): Deferred<String?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceAttribution(requireLiveHandle(), sourceId)
  }

  public actual fun styleSourceUrl(sourceId: String): Deferred<String?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceUrl(requireLiveHandle(), sourceId)
  }

  public actual fun styleSourceTileUrls(sourceId: String): Deferred<List<String>?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceTileUrls(requireLiveHandle(), sourceId)
  }

  public actual fun styleSourceIds(): Deferred<List<String>> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceIds(requireLiveHandle())
  }

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addGeoJsonSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return data.withNativeHandle { nativeData ->
      NativeAccess.addGeoJsonSourceData(requireLiveHandle(), sourceId, nativeData)
    }
  }

  public actual fun setGeoJsonSourceUrl(
    sourceId: String,
    url: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setGeoJsonSourceUrl(requireLiveHandle(), sourceId, url)
  }

  public actual fun setGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return data.withNativeHandle { nativeData ->
      NativeAccess.setGeoJsonSourceData(requireLiveHandle(), sourceId, nativeData)
    }
  }

  public actual fun setGeoJsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setGeoJsonSourceSynchronousTiling(requireLiveHandle(), sourceId, enabled)
  }

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return CompletionBridge.command { completion ->
      // The release callback captures the registry rather than this map, so a map a host leaks
      // with a live source still reports as leaked.
      val registry = customGeometrySources
      val sourceState = CustomGeometrySourceState(options) { registry.remove(sourceId) }
      registry.install(sourceId, sourceState) {
        HandleLeakCleaner.retainNativeCallbackRoot(sourceState)
        Status.check(
          NativeAccess.addCustomGeometrySource(
            requireLiveHandle(),
            sourceId,
            sourceState.descriptor(),
            completion,
          )
        )
      }
      MaplibreStatus.OK.nativeCode
    }
  }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setCustomGeometrySourceTileData(requireLiveHandle(), sourceId, tileId, data)
  }

  public actual fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.invalidateCustomGeometrySourceTile(requireLiveHandle(), sourceId, tileId)
  }

  public actual fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.invalidateCustomGeometrySourceRegion(requireLiveHandle(), sourceId, bounds)
  }

  public actual fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return CompletionBridge.command { completion ->
      // The release callback captures the registry rather than this map, so a map a host leaks
      // with a live source still reports as leaked.
      val registry = customMvtVectorSources
      val sourceState = CustomMvtVectorSourceState(options) { registry.remove(sourceId) }
      registry.install(sourceId, sourceState) {
        HandleLeakCleaner.retainNativeCallbackRoot(sourceState)
        Status.check(
          NativeAccess.addCustomMvtVectorSource(
            requireLiveHandle(),
            sourceId,
            sourceState.descriptor(),
            completion,
          )
        )
      }
      MaplibreStatus.OK.nativeCode
    }
  }

  public actual fun setCustomMvtVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setCustomMvtVectorSourceTileData(
      requireLiveHandle(),
      sourceId,
      tileId,
      data,
    )
  }

  public actual fun setCustomMvtVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setCustomMvtVectorSourceTileError(
      requireLiveHandle(),
      sourceId,
      tileId,
      message,
    )
  }

  public actual fun invalidateCustomMvtVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.invalidateCustomMvtVectorSourceTile(requireLiveHandle(), sourceId, tileId)
  }

  public actual fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addVectorSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addVectorSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addRasterSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addRasterSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addRasterDemSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addRasterDemSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setStyleImage(requireLiveHandle(), imageId, image, options)
  }

  public actual fun removeStyleImage(imageId: String): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeStyleImage(requireLiveHandle(), imageId)
  }

  public actual fun styleImageInfo(imageId: String): Deferred<StyleImageInfo?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleImageInfo(requireLiveHandle(), imageId)
  }

  public actual fun copyStyleImagePremultipliedRgba8(imageId: String): Deferred<ByteArray?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.copyStyleImagePremultipliedRgba8(requireLiveHandle(), imageId)
  }

  public actual fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addImageSourceUrl(requireLiveHandle(), sourceId, coordinates, url)
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addImageSourceImage(requireLiveHandle(), sourceId, coordinates, image)
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setImageSourceUrl(requireLiveHandle(), sourceId, url)
  }

  public actual fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setImageSourceImage(requireLiveHandle(), sourceId, image)
  }

  public actual fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setImageSourceCoordinates(requireLiveHandle(), sourceId, coordinates)
  }

  public actual fun imageSourceCoordinates(sourceId: String): Deferred<List<LatLng>?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.imageSourceCoordinates(requireLiveHandle(), sourceId)
  }

  public actual fun addStyleLayerJson(
    layerJson: ByteArray,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addStyleLayerJson(requireLiveHandle(), layerJson, beforeLayerId)
  }

  public actual fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addHillshadeLayer(requireLiveHandle(), layerId, sourceId, beforeLayerId)
  }

  public actual fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addColorReliefLayer(requireLiveHandle(), layerId, sourceId, beforeLayerId)
  }

  public actual fun addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.addLocationIndicatorLayer(requireLiveHandle(), layerId, beforeLayerId)
  }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLocationIndicatorLocation(
      requireLiveHandle(),
      layerId,
      coordinate,
      altitude,
    )
  }

  public actual fun setLocationIndicatorBearing(
    layerId: String,
    bearing: Double,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLocationIndicatorBearing(requireLiveHandle(), layerId, bearing)
  }

  public actual fun setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLocationIndicatorAccuracyRadius(requireLiveHandle(), layerId, radius)
  }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLocationIndicatorImageName(
      requireLiveHandle(),
      layerId,
      imageKind,
      imageId,
    )
  }

  public actual fun removeStyleLayer(layerId: String): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeStyleLayer(requireLiveHandle(), layerId)
  }

  public actual fun styleLayerInfo(layerId: String): Deferred<LayerInfo?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerInfo(requireLiveHandle(), layerId)
  }

  public actual fun styleLayerIds(): Deferred<List<String>> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerIds(requireLiveHandle())
  }

  public actual fun moveStyleLayer(
    layerId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.moveStyleLayer(requireLiveHandle(), layerId, beforeLayerId)
  }

  public actual fun styleLayerJson(layerId: String): Deferred<ByteArray?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerJson(requireLiveHandle(), layerId)
  }

  public actual fun setStyleLightJson(lightJson: ByteArray): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setStyleLightJson(requireLiveHandle(), lightJson)
  }

  public actual fun setStyleLightProperty(
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setStyleLightProperty(requireLiveHandle(), propertyName, value)
  }

  public actual fun styleLightProperty(propertyName: String): Deferred<ByteArray?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLightProperty(requireLiveHandle(), propertyName)
  }

  public actual fun setStyleTransitionOptions(
    options: StyleTransitionOptions
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setStyleTransitionOptions(requireLiveHandle(), options)
  }

  public actual fun styleTransitionOptions(): Deferred<StyleTransitionOptions> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleTransitionOptions(requireLiveHandle())
  }

  public actual fun setLayerProperty(
    layerId: String,
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLayerProperty(requireLiveHandle(), layerId, propertyName, value)
  }

  public actual fun layerProperty(layerId: String, propertyName: String): Deferred<ByteArray?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerProperty(requireLiveHandle(), layerId, propertyName)
  }

  public actual fun setLayerFilter(
    layerId: String,
    filter: ByteArray,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLayerFilter(requireLiveHandle(), layerId, filter)
  }

  public actual fun clearLayerFilter(layerId: String): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.clearLayerFilter(requireLiveHandle(), layerId)
  }

  public actual fun layerFilter(layerId: String): Deferred<ByteArray?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerFilter(requireLiveHandle(), layerId)
  }

  public actual fun styleImageStretches(
    imageId: String
  ): Deferred<Pair<List<ImageStretch>, List<ImageStretch>>?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleImageStretches(requireLiveHandle(), imageId)
  }

  public actual fun setLayerSourceLayer(
    layerId: String,
    sourceLayer: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLayerSourceLayer(requireLiveHandle(), layerId, sourceLayer)
  }

  public actual fun layerSourceLayer(layerId: String): Deferred<String> {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerSourceLayer(requireLiveHandle(), layerId)
  }

  public actual fun setLayerSourceId(
    layerId: String,
    sourceId: String,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLayerSourceId(requireLiveHandle(), layerId, sourceId)
  }

  public actual fun layerSourceId(layerId: String): Deferred<String> {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerSourceId(requireLiveHandle(), layerId)
  }

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLayerMinZoom(requireLiveHandle(), layerId, minZoom)
  }

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLayerMaxZoom(requireLiveHandle(), layerId, maxZoom)
  }

  public actual fun setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setLayerVisibility(requireLiveHandle(), layerId, visibility.nativeValue)
  }

  public actual fun requestRepaint(): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.requestRepaint(requireLiveHandle())
  }

  public actual fun requestStillImage(): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.requestStillImage(requireLiveHandle())
  }

  public actual fun snapshot(): MapSnapshot {
    NativeAccess.ensureLoaded()
    return NativeAccess.mapSnapshot(requireLiveHandle())
  }

  public actual fun setDebugOptions(options: Set<DebugOption>): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setDebugOptions(requireLiveHandle(), options)
  }

  public actual fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setRenderingStatsViewEnabled(requireLiveHandle(), enabled)
  }

  public actual fun setViewportOptions(options: ViewportOptions): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setViewportOptions(requireLiveHandle(), options)
  }

  public actual fun setTileOptions(options: TileOptions): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setTileOptions(requireLiveHandle(), options)
  }

  public actual fun setBounds(options: BoundOptions): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setBounds(requireLiveHandle(), options)
  }

  public actual fun setFreeCameraOptions(options: FreeCameraOptions): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setFreeCameraOptions(requireLiveHandle(), options)
  }

  public actual fun setProjectionMode(options: ProjectionModeOptions): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setProjectionMode(requireLiveHandle(), options)
  }

  public actual fun dumpDebugLogs(): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.dumpDebugLogs(requireLiveHandle())
  }

  public actual fun resize(size: MapSize): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.resizeMap(requireLiveHandle(), size)
  }

  public actual fun cameraSnapshot(): CameraSnapshot {
    NativeAccess.ensureLoaded()
    return NativeAccess.cameraSnapshot(requireLiveHandle())
  }

  public actual fun updateCamera(update: CameraUpdate): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.updateCamera(requireLiveHandle(), update)
  }

  public actual fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.applyCameraDelta(requireLiveHandle(), delta)
  }

  public actual fun cancelTransitions(): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.cancelTransitions(requireLiveHandle())
  }

  public actual fun queryCamera(): Deferred<CameraSnapshot> {
    NativeAccess.ensureLoaded()
    return NativeAccess.queryCamera(requireLiveHandle())
  }

  public actual fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): Deferred<CameraOptions> {
    NativeAccess.ensureLoaded()
    return NativeAccess.cameraForLatLngBounds(requireLiveHandle(), bounds, fitOptions)
  }

  public actual fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): Deferred<CameraOptions> {
    NativeAccess.ensureLoaded()
    return NativeAccess.cameraForLatLngs(requireLiveHandle(), coordinates, fitOptions)
  }

  public actual fun cameraForGeometry(
    geometry: ByteArray,
    fitOptions: CameraFitOptions?,
  ): Deferred<CameraOptions> {
    NativeAccess.ensureLoaded()
    return NativeAccess.cameraForGeometry(requireLiveHandle(), geometry, fitOptions)
  }

  public actual fun latLngBoundsForCamera(camera: CameraOptions): Deferred<LatLngBounds> {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngBoundsForCamera(requireLiveHandle(), camera, unwrapped = false)
  }

  public actual fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): Deferred<LatLngBounds> {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngBoundsForCamera(requireLiveHandle(), camera, unwrapped = true)
  }

  public actual fun pixelForLatLng(coordinate: LatLng): Deferred<ScreenPoint> {
    NativeAccess.ensureLoaded()
    return NativeAccess.pixelForLatLng(requireLiveHandle(), coordinate)
  }

  public actual fun latLngForPixel(point: ScreenPoint): Deferred<LatLng> {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngForPixel(requireLiveHandle(), point, unwrapped = false)
  }

  public actual fun latLngForPixelUnwrapped(point: ScreenPoint): Deferred<LatLng> {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngForPixel(requireLiveHandle(), point, unwrapped = true)
  }

  public actual fun pixelsForLatLngs(coordinates: List<LatLng>): Deferred<List<ScreenPoint>> {
    NativeAccess.ensureLoaded()
    return NativeAccess.pixelsForLatLngs(requireLiveHandle(), coordinates)
  }

  public actual fun latLngsForPixels(points: List<ScreenPoint>): Deferred<List<LatLng>> {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngsForPixels(requireLiveHandle(), points, unwrapped = false)
  }

  public actual fun latLngsForPixelsUnwrapped(points: List<ScreenPoint>): Deferred<List<LatLng>> {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngsForPixels(requireLiveHandle(), points, unwrapped = true)
  }

  public actual fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    renderSessionAttachment(
      NativeAccess.attachMetalOwnedTexture(requireLiveHandle(), descriptor, options)
    )

  public actual fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    renderSessionAttachment(
      NativeAccess.attachMetalBorrowedTexture(requireLiveHandle(), descriptor, options)
    )

  public actual fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    renderSessionAttachment(
      NativeAccess.attachVulkanOwnedTexture(requireLiveHandle(), descriptor, options)
    )

  public actual fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    renderSessionAttachment(
      NativeAccess.attachVulkanBorrowedTexture(requireLiveHandle(), descriptor, options)
    )

  public actual fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    renderSessionAttachment(
      NativeAccess.attachOpenGLOwnedTexture(requireLiveHandle(), descriptor, options)
    )

  public actual fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    renderSessionAttachment(
      NativeAccess.attachOpenGLBorrowedTexture(requireLiveHandle(), descriptor, options)
    )

  public actual fun attachMetalSurface(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    renderSessionAttachment(
      NativeAccess.attachMetalSurface(requireLiveHandle(), descriptor, options)
    )

  public actual fun attachVulkanSurface(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    renderSessionAttachment(
      NativeAccess.attachVulkanSurface(requireLiveHandle(), descriptor, options)
    )

  public actual fun attachOpenGLSurface(
    descriptor: OpenGLSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    renderSessionAttachment(
      NativeAccess.attachOpenGLSurface(requireLiveHandle(), descriptor, options)
    )

  private fun renderSessionAttachment(
    native: Pair<NativeRenderSession, Deferred<Unit>>
  ): RenderSessionAttachment =
    RenderSessionAttachment(RenderSessionHandle(this, native.first), native.second)

  public actual fun createProjection(): Deferred<MapProjectionHandle> {
    NativeAccess.ensureLoaded()
    return NativeAccess.createMapProjection(requireLiveHandle())
      .mapHandleDeferred(MapProjectionHandle::close, ::MapProjectionHandle)
  }

  public actual fun close(): Deferred<Unit> {
    val claim = CompletableDeferred<Unit>()
    val retirement = core.claimRetirement(claim)
    if (retirement !== claim) return retirement
    val completed =
      try {
        NativeAccess.releaseMap(handle)
      } catch (error: Throwable) {
        core.abortClose()
        core.abandonRetirement(claim)
        throw error
      }
    core.completeClose { runtime.unregisterMap(this) }
    completed.invokeOnCompletion { failure ->
      if (failure == null) claim.complete(Unit) else claim.completeExceptionally(failure)
    }
    return claim
  }

  public actual companion object {
    public actual fun create(runtime: RuntimeHandle, options: MapOptions): Deferred<MapHandle> {
      NativeAccess.ensureLoaded()
      return NativeAccess.createMap(runtime.nativeHandle(), options).mapHandleDeferred({ dropped ->
        dropped.close()
      }) { handle ->
        MapHandle(runtime, handle).also { runtime.registerMap(it) }
      }
    }
  }

  internal fun nativeHandleId(): Long = handle.raw

  internal fun nativeHandle(): NativeMap = requireLiveHandle()

  internal fun customGeometrySourceCountForTesting(): Int = customGeometrySources.size

  private fun requireLiveHandle(): NativeMap {
    core.requireLive()
    return handle
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
