package org.maplibre.nativeffi.map

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraSnapshot
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.async.mapDeferred
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.internal.loader.NativeAccess
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
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Owned JVM FFM map handle. */
public actual class MapHandle
private constructor(
  private val runtime: RuntimeHandle,
  private val handle: NativeMap,
  cachedEventMask: RuntimeEventMask,
) {
  private val core = HandleStateCore("MapHandle", handle.raw)
  private var cachedEventMask =
    RuntimeEventMask(cachedEventMask.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)

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

  public actual val eventMask: RuntimeEventMask
    get() = cachedEventMask

  public actual fun setEventMask(value: RuntimeEventMask): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return NativeAccess.setMapEventMask(requireLiveHandle(), value.nativeValue).mapDeferred {
      cachedEventMask =
        RuntimeEventMask(value.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)
      it
    }
  }

  public actual fun setStyleUrl(url: String): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setMapStyleUrl(requireLiveHandle(), url)
  }

  public actual fun setStyleJson(json: ByteArray): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setMapStyleJson(requireLiveHandle(), json)
  }

  public actual fun setFeatureState(
    selector: FeatureStateSelector,
    value: ByteArray,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setMapFeatureState(requireLiveHandle(), selector, value)
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray> {
    NativeAccess.ensureLoaded()
    return NativeAccess.getMapFeatureState(requireLiveHandle(), selector)
  }

  public actual fun removeFeatureState(
    selector: FeatureStateSelector
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.removeMapFeatureState(requireLiveHandle(), selector)
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
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addStyleSourceJson(requireLiveHandle(), sourceId, sourceJson)
  }

  public actual fun removeStyleSource(sourceId: String): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.removeStyleSource(requireLiveHandle(), sourceId)
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

  public actual fun styleSourceIds(): Deferred<List<String>> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceIds(requireLiveHandle())
  }

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addGeoJsonSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    data.withNativeHandle { nativeData ->
      NativeAccess.addGeoJsonSourceData(requireLiveHandle(), sourceId, nativeData)
    }
  }

  public actual fun setGeoJsonSourceUrl(
    sourceId: String,
    url: String,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setGeoJsonSourceUrl(requireLiveHandle(), sourceId, url)
  }

  public actual fun setGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    data.withNativeHandle { nativeData ->
      NativeAccess.setGeoJsonSourceData(requireLiveHandle(), sourceId, nativeData)
    }
  }

  public actual fun setGeoJsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setGeoJsonSourceSynchronousTiling(requireLiveHandle(), sourceId, enabled)
  }

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    val registry = customGeometrySources
    val sourceState = CustomGeometrySourceState(options) { registry.remove(sourceId) }
    lateinit var completed: Deferred<CommandCompletion>
    registry.install(sourceId, sourceState) {
      completed =
        NativeAccess.addCustomGeometrySource(
          requireLiveHandle(),
          sourceId,
          sourceState.descriptor(),
        )
      HandleLeakCleaner.retainNativeCallbackRoot(sourceState)
    }
    return completed
  }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setCustomGeometrySourceTileData(requireLiveHandle(), sourceId, tileId, data)
  }

  public actual fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.invalidateCustomGeometrySourceTile(requireLiveHandle(), sourceId, tileId)
  }

  public actual fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.invalidateCustomGeometrySourceRegion(requireLiveHandle(), sourceId, bounds)
  }

  public actual fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
  ): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    val registry = customMvtVectorSources
    val sourceState = CustomMvtVectorSourceState(options) { registry.remove(sourceId) }
    lateinit var completed: Deferred<CommandCompletion>
    registry.install(sourceId, sourceState) {
      completed =
        NativeAccess.addCustomMvtVectorSource(
          requireLiveHandle(),
          sourceId,
          sourceState.descriptor(),
        )
      HandleLeakCleaner.retainNativeCallbackRoot(sourceState)
    }
    return completed
  }

  public actual fun setCustomMvtVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setCustomMvtVectorSourceTileData(requireLiveHandle(), sourceId, tileId, data)
  }

  public actual fun setCustomMvtVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setCustomMvtVectorSourceTileError(requireLiveHandle(), sourceId, tileId, message)
  }

  public actual fun invalidateCustomMvtVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.invalidateCustomMvtVectorSourceTile(requireLiveHandle(), sourceId, tileId)
  }

  public actual fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addVectorSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addVectorSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterDemSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterDemSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleImage(requireLiveHandle(), imageId, image, options)
  }

  public actual fun removeStyleImage(imageId: String): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.removeStyleImage(requireLiveHandle(), imageId)
  }

  public actual fun styleImageInfo(imageId: String): Deferred<StyleImageInfo?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleImageInfo(requireLiveHandle(), imageId)
  }

  public actual fun copyStyleImagePremultipliedRgba8(imageId: String): Deferred<StyleImage?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.copyStyleImagePremultipliedRgba8(requireLiveHandle(), imageId)
  }

  public actual fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addImageSourceUrl(requireLiveHandle(), sourceId, coordinates, url)
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addImageSourceImage(requireLiveHandle(), sourceId, coordinates, image)
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    command {
      NativeAccess.ensureLoaded()
      NativeAccess.setImageSourceUrl(requireLiveHandle(), sourceId, url)
    }

  public actual fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setImageSourceImage(requireLiveHandle(), sourceId, image)
  }

  public actual fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setImageSourceCoordinates(requireLiveHandle(), sourceId, coordinates)
  }

  public actual fun imageSourceCoordinates(sourceId: String): Deferred<List<LatLng>?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.imageSourceCoordinates(requireLiveHandle(), sourceId)
  }

  public actual fun addStyleLayerJson(
    layerJson: ByteArray,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addStyleLayerJson(requireLiveHandle(), layerJson, beforeLayerId)
  }

  public actual fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addHillshadeLayer(requireLiveHandle(), layerId, sourceId, beforeLayerId)
  }

  public actual fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addColorReliefLayer(requireLiveHandle(), layerId, sourceId, beforeLayerId)
  }

  public actual fun addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.addLocationIndicatorLayer(requireLiveHandle(), layerId, beforeLayerId)
  }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorLocation(requireLiveHandle(), layerId, coordinate, altitude)
  }

  public actual fun setLocationIndicatorBearing(
    layerId: String,
    bearing: Double,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorBearing(requireLiveHandle(), layerId, bearing)
  }

  public actual fun setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorAccuracyRadius(requireLiveHandle(), layerId, radius)
  }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorImageName(requireLiveHandle(), layerId, imageKind, imageId)
  }

  public actual fun removeStyleLayer(layerId: String): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.removeStyleLayer(requireLiveHandle(), layerId)
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
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.moveStyleLayer(requireLiveHandle(), layerId, beforeLayerId)
  }

  public actual fun styleLayerJson(layerId: String): Deferred<ByteArray?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerJson(requireLiveHandle(), layerId)
  }

  public actual fun setStyleLightJson(lightJson: ByteArray): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleLightJson(requireLiveHandle(), lightJson)
  }

  public actual fun setStyleLightProperty(
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleLightProperty(requireLiveHandle(), propertyName, value)
  }

  public actual fun styleLightProperty(propertyName: String): Deferred<ByteArray?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLightProperty(requireLiveHandle(), propertyName)
  }

  public actual fun setStyleTransitionOptions(
    options: StyleTransitionOptions
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleTransitionOptions(requireLiveHandle(), options)
  }

  public actual fun styleTransitionOptions(): Deferred<StyleTransitionOptions> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleTransitionOptions(requireLiveHandle())
  }

  public actual fun setLayerProperty(
    layerId: String,
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerProperty(requireLiveHandle(), layerId, propertyName, value)
  }

  public actual fun layerProperty(layerId: String, propertyName: String): Deferred<ByteArray?> {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerProperty(requireLiveHandle(), layerId, propertyName)
  }

  public actual fun setLayerFilter(
    layerId: String,
    filter: ByteArray,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerFilter(requireLiveHandle(), layerId, filter)
  }

  public actual fun clearLayerFilter(layerId: String): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.clearLayerFilter(requireLiveHandle(), layerId)
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
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerSourceLayer(requireLiveHandle(), layerId, sourceLayer)
  }

  public actual fun layerSourceLayer(layerId: String): Deferred<String> {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerSourceLayer(requireLiveHandle(), layerId)
  }

  public actual fun setLayerSourceId(
    layerId: String,
    sourceId: String,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerSourceId(requireLiveHandle(), layerId, sourceId)
  }

  public actual fun layerSourceId(layerId: String): Deferred<String> {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerSourceId(requireLiveHandle(), layerId)
  }

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double): Deferred<CommandCompletion> =
    command {
      NativeAccess.ensureLoaded()
      NativeAccess.setLayerMinZoom(requireLiveHandle(), layerId, minZoom)
    }

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double): Deferred<CommandCompletion> =
    command {
      NativeAccess.ensureLoaded()
      NativeAccess.setLayerMaxZoom(requireLiveHandle(), layerId, maxZoom)
    }

  public actual fun setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility,
  ): Deferred<CommandCompletion> = command {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerVisibility(requireLiveHandle(), layerId, visibility.nativeValue)
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

  public actual fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion> =
    NativeAccess.applyCameraDelta(requireLiveHandle(), delta)

  public actual fun queryCamera(): Deferred<CameraSnapshot> {
    NativeAccess.ensureLoaded()
    return NativeAccess.queryCamera(requireLiveHandle())
  }

  public actual fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachMetalOwnedTexture(this, descriptor, options)

  public actual fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachMetalBorrowedTexture(this, descriptor, options)

  public actual fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachVulkanOwnedTexture(this, descriptor, options)

  public actual fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachVulkanBorrowedTexture(this, descriptor, options)

  public actual fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachOpenGLOwnedTexture(this, descriptor, options)

  public actual fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachOpenGLBorrowedTexture(this, descriptor, options)

  public actual fun attachMetalSurface(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment = RenderSessionHandle.attachMetalSurface(this, descriptor, options)

  public actual fun attachVulkanSurface(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment = RenderSessionHandle.attachVulkanSurface(this, descriptor, options)

  public actual fun attachOpenGLSurface(
    descriptor: OpenGLSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment = RenderSessionHandle.attachOpenGLSurface(this, descriptor, options)

  public actual fun createProjection(): Deferred<MapProjectionHandle> {
    NativeAccess.ensureLoaded()
    return NativeAccess.createMapProjection(requireLiveHandle()).mapDeferred(::MapProjectionHandle)
  }

  public actual fun close(): Deferred<Unit> {
    if (!core.beginClose()) return kotlinx.coroutines.CompletableDeferred(Unit)
    val completed =
      try {
        NativeAccess.releaseMap(handle)
      } catch (error: Throwable) {
        core.abortClose()
        throw error
      }
    core.completeClose { runtime.unregisterMap(this) }
    return completed
  }

  public actual companion object {
    public actual fun create(runtime: RuntimeHandle, options: MapOptions): Deferred<MapHandle> {
      NativeAccess.ensureLoaded()
      return NativeAccess.createMap(runtime.nativeHandle(), options).mapDeferred { handle ->
        MapHandle(runtime, handle, options.eventMask).also { runtime.registerMap(it) }
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

private inline fun command(call: () -> Deferred<CommandCompletion>): Deferred<CommandCompletion> =
  call()

private fun releaseCallbackRoot(root: AutoCloseable?) {
  HandleLeakCleaner.releaseNativeCallbackRoot(root)
  closeQuietly(root)
}

private fun closeQuietly(closeable: AutoCloseable?) {
  try {
    closeable?.close()
  } catch (_: Exception) {}
}
