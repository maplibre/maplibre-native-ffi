package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.internal.loader.NativeAccess
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
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceDataHandle
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.ImageStretch
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Owned JVM FFM map handle. */
public actual class MapHandle
private constructor(private val runtime: RuntimeHandle, private val handle: NativeMap) :
  AutoCloseable {
  private val runtimeRetention = runtime.retainChild("MapHandle")
  private val core = HandleStateCore("MapHandle", handle.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private val customGeometrySources =
    CustomGeometrySourceRegistry<CustomGeometrySourceState>(::releaseCallbackRoot)

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  public actual var eventMask: RuntimeEventMask
    get() {
      NativeAccess.ensureLoaded()
      return RuntimeEventMask(NativeAccess.mapEventMask(requireLiveHandle()))
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setMapEventMask(requireLiveHandle(), value.nativeValue)
    }

  public actual fun setStyleUrl(url: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.setMapStyleUrl(requireLiveHandle(), url)
  }

  public actual fun setStyleJson(json: ByteArray) {
    NativeAccess.ensureLoaded()
    NativeAccess.setMapStyleJson(requireLiveHandle(), json)
  }

  public actual fun loadedStyleJson(): ByteArray {
    NativeAccess.ensureLoaded()
    return NativeAccess.loadedStyleJson(requireLiveHandle())
  }

  public actual fun styleUrl(): String {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleUrl(requireLiveHandle())
  }

  public actual fun addStyleSourceJson(sourceId: String, sourceJson: ByteArray) {
    NativeAccess.ensureLoaded()
    NativeAccess.addStyleSourceJson(requireLiveHandle(), sourceId, sourceJson)
  }

  public actual fun removeStyleSource(sourceId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeStyleSource(requireLiveHandle(), sourceId)
  }

  public actual fun styleSourceExists(sourceId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceExists(requireLiveHandle(), sourceId)
  }

  public actual fun styleSourceType(sourceId: String): SourceType? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceType(requireLiveHandle(), sourceId)
  }

  public actual fun styleSourceInfo(sourceId: String): SourceInfo? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceInfo(requireLiveHandle(), sourceId)
  }

  public actual fun styleSourceIds(): List<String> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceIds(requireLiveHandle())
  }

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.addGeoJsonSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addGeoJsonSourceData(sourceId: String, data: GeoJsonSourceDataHandle) {
    NativeAccess.ensureLoaded()
    data.withNativeHandle { nativeData ->
      NativeAccess.addGeoJsonSourceData(requireLiveHandle(), sourceId, nativeData)
    }
  }

  public actual fun setGeoJsonSourceUrl(sourceId: String, url: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.setGeoJsonSourceUrl(requireLiveHandle(), sourceId, url)
  }

  public actual fun setGeoJsonSourceData(sourceId: String, data: GeoJsonSourceDataHandle) {
    NativeAccess.ensureLoaded()
    data.withNativeHandle { nativeData ->
      NativeAccess.setGeoJsonSourceData(requireLiveHandle(), sourceId, nativeData)
    }
  }

  public actual fun setGeoJsonSourceSynchronousTiling(sourceId: String, enabled: Boolean) {
    NativeAccess.ensureLoaded()
    NativeAccess.setGeoJsonSourceSynchronousTiling(requireLiveHandle(), sourceId, enabled)
  }

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ) {
    NativeAccess.ensureLoaded()
    // The release callback captures the registry rather than this map, so a map a
    // host leaks with a live source still reports as leaked.
    val registry = customGeometrySources
    val sourceState = CustomGeometrySourceState(options) { registry.remove(sourceId) }
    registry.install(sourceId, sourceState) {
      NativeAccess.addCustomGeometrySource(requireLiveHandle(), sourceId, sourceState.descriptor())
      HandleLeakCleaner.retainNativeCallbackRoot(sourceState)
    }
  }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.setCustomGeometrySourceTileData(requireLiveHandle(), sourceId, tileId, data)
  }

  public actual fun invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileId) {
    NativeAccess.ensureLoaded()
    NativeAccess.invalidateCustomGeometrySourceTile(requireLiveHandle(), sourceId, tileId)
  }

  public actual fun invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds) {
    NativeAccess.ensureLoaded()
    NativeAccess.invalidateCustomGeometrySourceRegion(requireLiveHandle(), sourceId, bounds)
  }

  public actual fun addVectorSourceUrl(sourceId: String, url: String, options: TileSourceOptions?) {
    NativeAccess.ensureLoaded()
    NativeAccess.addVectorSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.addVectorSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun addRasterSourceUrl(sourceId: String, url: String, options: TileSourceOptions?) {
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterDemSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterDemSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleImage(requireLiveHandle(), imageId, image, options)
  }

  public actual fun removeStyleImage(imageId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeStyleImage(requireLiveHandle(), imageId)
  }

  public actual fun styleImageExists(imageId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleImageExists(requireLiveHandle(), imageId)
  }

  public actual fun styleImageInfo(imageId: String): StyleImageInfo? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleImageInfo(requireLiveHandle(), imageId)
  }

  public actual fun copyStyleImagePremultipliedRgba8(imageId: String): StyleImage? {
    NativeAccess.ensureLoaded()
    return NativeAccess.copyStyleImagePremultipliedRgba8(requireLiveHandle(), imageId)
  }

  public actual fun addImageSourceUrl(sourceId: String, coordinates: List<LatLng>, url: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.addImageSourceUrl(requireLiveHandle(), sourceId, coordinates, url)
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.addImageSourceImage(requireLiveHandle(), sourceId, coordinates, image)
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.setImageSourceUrl(requireLiveHandle(), sourceId, url)
  }

  public actual fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image) {
    NativeAccess.ensureLoaded()
    NativeAccess.setImageSourceImage(requireLiveHandle(), sourceId, image)
  }

  public actual fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>) {
    NativeAccess.ensureLoaded()
    NativeAccess.setImageSourceCoordinates(requireLiveHandle(), sourceId, coordinates)
  }

  public actual fun imageSourceCoordinates(sourceId: String): List<LatLng>? {
    NativeAccess.ensureLoaded()
    return NativeAccess.imageSourceCoordinates(requireLiveHandle(), sourceId)
  }

  public actual fun addStyleLayerJson(layerJson: ByteArray, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.addStyleLayerJson(requireLiveHandle(), layerJson, beforeLayerId)
  }

  public actual fun addHillshadeLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.addHillshadeLayer(requireLiveHandle(), layerId, sourceId, beforeLayerId)
  }

  public actual fun addColorReliefLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.addColorReliefLayer(requireLiveHandle(), layerId, sourceId, beforeLayerId)
  }

  public actual fun addLocationIndicatorLayer(layerId: String, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.addLocationIndicatorLayer(requireLiveHandle(), layerId, beforeLayerId)
  }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorLocation(requireLiveHandle(), layerId, coordinate, altitude)
  }

  public actual fun setLocationIndicatorBearing(layerId: String, bearing: Double) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorBearing(requireLiveHandle(), layerId, bearing)
  }

  public actual fun setLocationIndicatorAccuracyRadius(layerId: String, radius: Double) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorAccuracyRadius(requireLiveHandle(), layerId, radius)
  }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorImageName(requireLiveHandle(), layerId, imageKind, imageId)
  }

  public actual fun removeStyleLayer(layerId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeStyleLayer(requireLiveHandle(), layerId)
  }

  public actual fun styleLayerExists(layerId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerExists(requireLiveHandle(), layerId)
  }

  public actual fun styleLayerType(layerId: String): String? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerType(requireLiveHandle(), layerId)
  }

  public actual fun styleLayerIds(): List<String> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerIds(requireLiveHandle())
  }

  public actual fun moveStyleLayer(layerId: String, beforeLayerId: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.moveStyleLayer(requireLiveHandle(), layerId, beforeLayerId)
  }

  public actual fun styleLayerJson(layerId: String): ByteArray? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerJson(requireLiveHandle(), layerId)
  }

  public actual fun setStyleLightJson(lightJson: ByteArray) {
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleLightJson(requireLiveHandle(), lightJson)
  }

  public actual fun setStyleLightProperty(propertyName: String, value: ByteArray) {
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleLightProperty(requireLiveHandle(), propertyName, value)
  }

  public actual fun styleLightProperty(propertyName: String): ByteArray? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLightProperty(requireLiveHandle(), propertyName)
  }

  public actual fun setStyleTransitionOptions(options: StyleTransitionOptions) {
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleTransitionOptions(requireLiveHandle(), options)
  }

  public actual fun styleTransitionOptions(): StyleTransitionOptions {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleTransitionOptions(requireLiveHandle())
  }

  public actual fun setLayerProperty(layerId: String, propertyName: String, value: ByteArray) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerProperty(requireLiveHandle(), layerId, propertyName, value)
  }

  public actual fun layerProperty(layerId: String, propertyName: String): ByteArray? {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerProperty(requireLiveHandle(), layerId, propertyName)
  }

  public actual fun setLayerFilter(layerId: String, filter: ByteArray) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerFilter(requireLiveHandle(), layerId, filter)
  }

  public actual fun clearLayerFilter(layerId: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.clearLayerFilter(requireLiveHandle(), layerId)
  }

  public actual fun layerFilter(layerId: String): ByteArray? {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerFilter(requireLiveHandle(), layerId)
  }

  public actual fun styleImageStretches(
    imageId: String
  ): Pair<List<ImageStretch>, List<ImageStretch>>? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleImageStretches(requireLiveHandle(), imageId)
  }

  public actual fun setLayerSourceLayer(layerId: String, sourceLayer: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerSourceLayer(requireLiveHandle(), layerId, sourceLayer)
  }

  public actual fun layerSourceLayer(layerId: String): String {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerSourceLayer(requireLiveHandle(), layerId)
  }

  public actual fun setLayerSourceId(layerId: String, sourceId: String) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerSourceId(requireLiveHandle(), layerId, sourceId)
  }

  public actual fun layerSourceId(layerId: String): String {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerSourceId(requireLiveHandle(), layerId)
  }

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerMinZoom(requireLiveHandle(), layerId, minZoom)
  }

  public actual fun layerMinZoom(layerId: String): Double {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerMinZoom(requireLiveHandle(), layerId)
  }

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerMaxZoom(requireLiveHandle(), layerId, maxZoom)
  }

  public actual fun layerMaxZoom(layerId: String): Double {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerMaxZoom(requireLiveHandle(), layerId)
  }

  public actual fun setLayerVisibility(layerId: String, visibility: StyleLayerVisibility) {
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerVisibility(requireLiveHandle(), layerId, visibility.nativeValue)
  }

  public actual fun layerVisibility(layerId: String): StyleLayerVisibility {
    NativeAccess.ensureLoaded()
    return StyleLayerVisibility.fromNative(
      NativeAccess.layerVisibility(requireLiveHandle(), layerId)
    )
  }

  public actual fun requestRepaint() {
    NativeAccess.ensureLoaded()
    NativeAccess.requestRepaint(requireLiveHandle())
  }

  public actual fun requestStillImage() {
    NativeAccess.ensureLoaded()
    NativeAccess.requestStillImage(requireLiveHandle())
  }

  public actual var debugOptions: Set<DebugOption>
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.debugOptions(requireLiveHandle())
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setDebugOptions(requireLiveHandle(), value)
    }

  public actual var isRenderingStatsViewEnabled: Boolean
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.renderingStatsViewEnabled(requireLiveHandle())
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setRenderingStatsViewEnabled(requireLiveHandle(), value)
    }

  public actual val isFullyLoaded: Boolean
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.isFullyLoaded(requireLiveHandle())
    }

  public actual fun dumpDebugLogs() {
    NativeAccess.ensureLoaded()
    NativeAccess.dumpDebugLogs(requireLiveHandle())
  }

  public actual val size: MapSize
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.mapSize(requireLiveHandle())
    }

  public actual var viewportOptions: ViewportOptions
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.viewportOptions(requireLiveHandle())
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setViewportOptions(requireLiveHandle(), value)
    }

  public actual var tileOptions: TileOptions
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.tileOptions(requireLiveHandle())
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setTileOptions(requireLiveHandle(), value)
    }

  public actual val camera: CameraOptions
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.camera(requireLiveHandle())
    }

  public actual fun jumpTo(camera: CameraOptions) {
    NativeAccess.ensureLoaded()
    NativeAccess.jumpTo(requireLiveHandle(), camera)
  }

  public actual fun easeTo(camera: CameraOptions, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    NativeAccess.easeTo(requireLiveHandle(), camera, animation)
  }

  public actual fun flyTo(camera: CameraOptions, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    NativeAccess.flyTo(requireLiveHandle(), camera, animation)
  }

  public actual fun moveBy(deltaX: Double, deltaY: Double) {
    NativeAccess.ensureLoaded()
    NativeAccess.moveBy(requireLiveHandle(), deltaX, deltaY)
  }

  public actual fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    NativeAccess.moveByAnimated(requireLiveHandle(), deltaX, deltaY, animation)
  }

  public actual fun scaleBy(scale: Double, anchor: ScreenPoint?) {
    NativeAccess.ensureLoaded()
    NativeAccess.scaleBy(requireLiveHandle(), scale, anchor)
  }

  public actual fun scaleByAnimated(
    scale: Double,
    anchor: ScreenPoint?,
    animation: AnimationOptions?,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.scaleByAnimated(requireLiveHandle(), scale, anchor, animation)
  }

  public actual fun rotateBy(first: ScreenPoint, second: ScreenPoint) {
    NativeAccess.ensureLoaded()
    NativeAccess.rotateBy(requireLiveHandle(), first, second)
  }

  public actual fun rotateByAnimated(
    first: ScreenPoint,
    second: ScreenPoint,
    animation: AnimationOptions?,
  ) {
    NativeAccess.ensureLoaded()
    NativeAccess.rotateByAnimated(requireLiveHandle(), first, second, animation)
  }

  public actual fun pitchBy(pitch: Double) {
    NativeAccess.ensureLoaded()
    NativeAccess.pitchBy(requireLiveHandle(), pitch)
  }

  public actual fun pitchByAnimated(pitch: Double, animation: AnimationOptions?) {
    NativeAccess.ensureLoaded()
    NativeAccess.pitchByAnimated(requireLiveHandle(), pitch, animation)
  }

  public actual fun cancelTransitions() {
    NativeAccess.ensureLoaded()
    NativeAccess.cancelTransitions(requireLiveHandle())
  }

  public actual var isGestureInProgress: Boolean
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.isGestureInProgress(requireLiveHandle())
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setGestureInProgress(requireLiveHandle(), value)
    }

  public actual fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): CameraOptions {
    NativeAccess.ensureLoaded()
    return NativeAccess.cameraForLatLngBounds(requireLiveHandle(), bounds, fitOptions)
  }

  public actual fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): CameraOptions {
    NativeAccess.ensureLoaded()
    return NativeAccess.cameraForLatLngs(requireLiveHandle(), coordinates, fitOptions)
  }

  public actual fun cameraForGeometry(
    geometry: ByteArray,
    fitOptions: CameraFitOptions?,
  ): CameraOptions {
    NativeAccess.ensureLoaded()
    return NativeAccess.cameraForGeometry(requireLiveHandle(), geometry, fitOptions)
  }

  public actual fun latLngBoundsForCamera(camera: CameraOptions): LatLngBounds {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngBoundsForCamera(requireLiveHandle(), camera)
  }

  public actual fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): LatLngBounds {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngBoundsForCameraUnwrapped(requireLiveHandle(), camera)
  }

  public actual var bounds: BoundOptions
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.bounds(requireLiveHandle())
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setBounds(requireLiveHandle(), value)
    }

  public actual var freeCameraOptions: FreeCameraOptions
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.freeCameraOptions(requireLiveHandle())
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setFreeCameraOptions(requireLiveHandle(), value)
    }

  public actual var projectionMode: ProjectionModeOptions
    get() {
      NativeAccess.ensureLoaded()
      return NativeAccess.projectionMode(requireLiveHandle())
    }
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setProjectionMode(requireLiveHandle(), value)
    }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    NativeAccess.ensureLoaded()
    return NativeAccess.pixelForLatLng(requireLiveHandle(), coordinate)
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngForPixel(requireLiveHandle(), point)
  }

  public actual fun pixelsForLatLngs(coordinates: List<LatLng>): List<ScreenPoint> {
    NativeAccess.ensureLoaded()
    return NativeAccess.pixelsForLatLngs(requireLiveHandle(), coordinates)
  }

  public actual fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng> {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngsForPixels(requireLiveHandle(), points)
  }

  public actual fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor
  ): RenderSessionHandle {
    NativeAccess.ensureLoaded()
    return RenderSessionHandle(
      this,
      NativeAccess.attachMetalOwnedTexture(requireLiveHandle(), descriptor),
    )
  }

  public actual fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor
  ): RenderSessionHandle {
    NativeAccess.ensureLoaded()
    return RenderSessionHandle(
      this,
      NativeAccess.attachMetalBorrowedTexture(requireLiveHandle(), descriptor),
    )
  }

  public actual fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor
  ): RenderSessionHandle {
    NativeAccess.ensureLoaded()
    return RenderSessionHandle(
      this,
      NativeAccess.attachVulkanOwnedTexture(requireLiveHandle(), descriptor),
    )
  }

  public actual fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor
  ): RenderSessionHandle {
    NativeAccess.ensureLoaded()
    return RenderSessionHandle(
      this,
      NativeAccess.attachVulkanBorrowedTexture(requireLiveHandle(), descriptor),
    )
  }

  public actual fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor
  ): RenderSessionHandle {
    NativeAccess.ensureLoaded()
    return RenderSessionHandle(
      this,
      NativeAccess.attachOpenGLOwnedTexture(requireLiveHandle(), descriptor),
    )
  }

  public actual fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): RenderSessionHandle {
    NativeAccess.ensureLoaded()
    return RenderSessionHandle(
      this,
      NativeAccess.attachOpenGLBorrowedTexture(requireLiveHandle(), descriptor),
    )
  }

  public actual fun attachMetalSurface(descriptor: MetalSurfaceDescriptor): RenderSessionHandle {
    NativeAccess.ensureLoaded()
    return RenderSessionHandle(
      this,
      NativeAccess.attachMetalSurface(requireLiveHandle(), descriptor),
    )
  }

  public actual fun attachVulkanSurface(descriptor: VulkanSurfaceDescriptor): RenderSessionHandle {
    NativeAccess.ensureLoaded()
    return RenderSessionHandle(
      this,
      NativeAccess.attachVulkanSurface(requireLiveHandle(), descriptor),
    )
  }

  public actual fun attachOpenGLSurface(descriptor: OpenGLSurfaceDescriptor): RenderSessionHandle {
    NativeAccess.ensureLoaded()
    return RenderSessionHandle(
      this,
      NativeAccess.attachOpenGLSurface(requireLiveHandle(), descriptor),
    )
  }

  public actual fun createProjection(): MapProjectionHandle {
    NativeAccess.ensureLoaded()
    return MapProjectionHandle(NativeAccess.createMapProjection(requireLiveHandle()))
  }

  public actual override fun close() {
    core.closeOnce(
      destroy = { NativeAccess.destroyMap(handle) },
      afterSuccess = {
        runtime.unregisterMap(this)
        runtimeRetention.close()
      },
    )
  }

  public actual companion object {
    public actual fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle {
      NativeAccess.ensureLoaded()
      return MapHandle(runtime, NativeAccess.createMap(runtime.nativeHandle(), options)).also {
        runtime.registerMap(it)
      }
    }
  }

  internal fun nativeHandleId(): Long = handle.raw

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    core.retainChild(childTypeName)

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
