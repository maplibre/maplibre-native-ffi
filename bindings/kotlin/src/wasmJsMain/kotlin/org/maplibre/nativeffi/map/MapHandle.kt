package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.ScreenPoint
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

/**
 * Scaffold for the browser map handle.
 *
 * Every member throws. The actual exists so the `wasmJs` source set compiles while the browser
 * binding is filled in one file at a time; nothing here is finished work.
 */
public actual class MapHandle private constructor() : AutoCloseable {
  public actual val isClosed: Boolean
    get() = throw NotImplementedError("wasmJs MapHandle.isClosed is not implemented yet")

  public actual fun runtime(): RuntimeHandle =
    throw NotImplementedError("wasmJs MapHandle.runtime is not implemented yet")

  public actual fun setStyleUrl(url: String) {
    throw NotImplementedError("wasmJs MapHandle.setStyleUrl is not implemented yet")
  }

  public actual fun setStyleJson(json: String) {
    throw NotImplementedError("wasmJs MapHandle.setStyleJson is not implemented yet")
  }

  public actual fun loadedStyleJson(): String =
    throw NotImplementedError("wasmJs MapHandle.loadedStyleJson is not implemented yet")

  public actual fun styleUrl(): String =
    throw NotImplementedError("wasmJs MapHandle.styleUrl is not implemented yet")

  public actual fun addStyleSourceJson(sourceId: String, sourceJson: JsonValue) {
    throw NotImplementedError("wasmJs MapHandle.addStyleSourceJson is not implemented yet")
  }

  public actual fun removeStyleSource(sourceId: String): Boolean =
    throw NotImplementedError("wasmJs MapHandle.removeStyleSource is not implemented yet")

  public actual fun styleSourceExists(sourceId: String): Boolean =
    throw NotImplementedError("wasmJs MapHandle.styleSourceExists is not implemented yet")

  public actual fun styleSourceType(sourceId: String): SourceType? =
    throw NotImplementedError("wasmJs MapHandle.styleSourceType is not implemented yet")

  public actual fun styleSourceInfo(sourceId: String): SourceInfo? =
    throw NotImplementedError("wasmJs MapHandle.styleSourceInfo is not implemented yet")

  public actual fun styleSourceIds(): List<String> =
    throw NotImplementedError("wasmJs MapHandle.styleSourceIds is not implemented yet")

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ) {
    throw NotImplementedError("wasmJs MapHandle.addGeoJsonSourceUrl is not implemented yet")
  }

  public actual fun addGeoJsonSourceData(
    sourceId: String,
    data: GeoJson,
    options: GeoJsonSourceOptions?,
  ) {
    throw NotImplementedError("wasmJs MapHandle.addGeoJsonSourceData is not implemented yet")
  }

  public actual fun setGeoJsonSourceUrl(sourceId: String, url: String) {
    throw NotImplementedError("wasmJs MapHandle.setGeoJsonSourceUrl is not implemented yet")
  }

  public actual fun setGeoJsonSourceData(sourceId: String, data: GeoJson) {
    throw NotImplementedError("wasmJs MapHandle.setGeoJsonSourceData is not implemented yet")
  }

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ) {
    throw NotImplementedError("wasmJs MapHandle.addCustomGeometrySource is not implemented yet")
  }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: GeoJson,
  ) {
    throw NotImplementedError(
      "wasmJs MapHandle.setCustomGeometrySourceTileData is not implemented yet"
    )
  }

  public actual fun invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileId) {
    throw NotImplementedError(
      "wasmJs MapHandle.invalidateCustomGeometrySourceTile is not implemented yet"
    )
  }

  public actual fun invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds) {
    throw NotImplementedError(
      "wasmJs MapHandle.invalidateCustomGeometrySourceRegion is not implemented yet"
    )
  }

  public actual fun addVectorSourceUrl(sourceId: String, url: String, options: TileSourceOptions?) {
    throw NotImplementedError("wasmJs MapHandle.addVectorSourceUrl is not implemented yet")
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    throw NotImplementedError("wasmJs MapHandle.addVectorSourceTiles is not implemented yet")
  }

  public actual fun addRasterSourceUrl(sourceId: String, url: String, options: TileSourceOptions?) {
    throw NotImplementedError("wasmJs MapHandle.addRasterSourceUrl is not implemented yet")
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    throw NotImplementedError("wasmJs MapHandle.addRasterSourceTiles is not implemented yet")
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    throw NotImplementedError("wasmJs MapHandle.addRasterDemSourceUrl is not implemented yet")
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    throw NotImplementedError("wasmJs MapHandle.addRasterDemSourceTiles is not implemented yet")
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ) {
    throw NotImplementedError("wasmJs MapHandle.setStyleImage is not implemented yet")
  }

  public actual fun removeStyleImage(imageId: String): Boolean =
    throw NotImplementedError("wasmJs MapHandle.removeStyleImage is not implemented yet")

  public actual fun styleImageExists(imageId: String): Boolean =
    throw NotImplementedError("wasmJs MapHandle.styleImageExists is not implemented yet")

  public actual fun styleImageInfo(imageId: String): StyleImageInfo? =
    throw NotImplementedError("wasmJs MapHandle.styleImageInfo is not implemented yet")

  public actual fun styleImageStretches(
    imageId: String
  ): Pair<List<ImageStretch>, List<ImageStretch>>? =
    throw NotImplementedError("wasmJs MapHandle.styleImageStretches is not implemented yet")

  public actual fun copyStyleImagePremultipliedRgba8(imageId: String): StyleImage? =
    throw NotImplementedError(
      "wasmJs MapHandle.copyStyleImagePremultipliedRgba8 is not implemented yet"
    )

  public actual fun addImageSourceUrl(sourceId: String, coordinates: List<LatLng>, url: String) {
    throw NotImplementedError("wasmJs MapHandle.addImageSourceUrl is not implemented yet")
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ) {
    throw NotImplementedError("wasmJs MapHandle.addImageSourceImage is not implemented yet")
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String) {
    throw NotImplementedError("wasmJs MapHandle.setImageSourceUrl is not implemented yet")
  }

  public actual fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image) {
    throw NotImplementedError("wasmJs MapHandle.setImageSourceImage is not implemented yet")
  }

  public actual fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>) {
    throw NotImplementedError("wasmJs MapHandle.setImageSourceCoordinates is not implemented yet")
  }

  public actual fun imageSourceCoordinates(sourceId: String): List<LatLng>? =
    throw NotImplementedError("wasmJs MapHandle.imageSourceCoordinates is not implemented yet")

  public actual fun addStyleLayerJson(layerJson: JsonValue, beforeLayerId: String) {
    throw NotImplementedError("wasmJs MapHandle.addStyleLayerJson is not implemented yet")
  }

  public actual fun addHillshadeLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    throw NotImplementedError("wasmJs MapHandle.addHillshadeLayer is not implemented yet")
  }

  public actual fun addColorReliefLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    throw NotImplementedError("wasmJs MapHandle.addColorReliefLayer is not implemented yet")
  }

  public actual fun addLocationIndicatorLayer(layerId: String, beforeLayerId: String) {
    throw NotImplementedError("wasmJs MapHandle.addLocationIndicatorLayer is not implemented yet")
  }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ) {
    throw NotImplementedError(
      "wasmJs MapHandle.setLocationIndicatorLocation is not implemented yet"
    )
  }

  public actual fun setLocationIndicatorBearing(layerId: String, bearing: Double) {
    throw NotImplementedError("wasmJs MapHandle.setLocationIndicatorBearing is not implemented yet")
  }

  public actual fun setLocationIndicatorAccuracyRadius(layerId: String, radius: Double) {
    throw NotImplementedError(
      "wasmJs MapHandle.setLocationIndicatorAccuracyRadius is not implemented yet"
    )
  }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ) {
    throw NotImplementedError(
      "wasmJs MapHandle.setLocationIndicatorImageName is not implemented yet"
    )
  }

  public actual fun removeStyleLayer(layerId: String): Boolean =
    throw NotImplementedError("wasmJs MapHandle.removeStyleLayer is not implemented yet")

  public actual fun styleLayerExists(layerId: String): Boolean =
    throw NotImplementedError("wasmJs MapHandle.styleLayerExists is not implemented yet")

  public actual fun styleLayerType(layerId: String): String? =
    throw NotImplementedError("wasmJs MapHandle.styleLayerType is not implemented yet")

  public actual fun styleLayerIds(): List<String> =
    throw NotImplementedError("wasmJs MapHandle.styleLayerIds is not implemented yet")

  public actual fun moveStyleLayer(layerId: String, beforeLayerId: String) {
    throw NotImplementedError("wasmJs MapHandle.moveStyleLayer is not implemented yet")
  }

  public actual fun styleLayerJson(layerId: String): JsonValue? =
    throw NotImplementedError("wasmJs MapHandle.styleLayerJson is not implemented yet")

  public actual fun setStyleLightJson(lightJson: JsonValue) {
    throw NotImplementedError("wasmJs MapHandle.setStyleLightJson is not implemented yet")
  }

  public actual fun setStyleLightProperty(propertyName: String, value: JsonValue) {
    throw NotImplementedError("wasmJs MapHandle.setStyleLightProperty is not implemented yet")
  }

  public actual fun styleLightProperty(propertyName: String): JsonValue? =
    throw NotImplementedError("wasmJs MapHandle.styleLightProperty is not implemented yet")

  public actual fun setStyleTransitionOptions(options: StyleTransitionOptions) {
    throw NotImplementedError("wasmJs MapHandle.setStyleTransitionOptions is not implemented yet")
  }

  public actual fun styleTransitionOptions(): StyleTransitionOptions =
    throw NotImplementedError("wasmJs MapHandle.styleTransitionOptions is not implemented yet")

  public actual fun setLayerProperty(layerId: String, propertyName: String, value: JsonValue) {
    throw NotImplementedError("wasmJs MapHandle.setLayerProperty is not implemented yet")
  }

  public actual fun layerProperty(layerId: String, propertyName: String): JsonValue? =
    throw NotImplementedError("wasmJs MapHandle.layerProperty is not implemented yet")

  public actual fun setLayerFilter(layerId: String, filter: JsonValue) {
    throw NotImplementedError("wasmJs MapHandle.setLayerFilter is not implemented yet")
  }

  public actual fun clearLayerFilter(layerId: String) {
    throw NotImplementedError("wasmJs MapHandle.clearLayerFilter is not implemented yet")
  }

  public actual fun layerFilter(layerId: String): JsonValue? =
    throw NotImplementedError("wasmJs MapHandle.layerFilter is not implemented yet")

  public actual fun setLayerSourceLayer(layerId: String, sourceLayer: String) {
    throw NotImplementedError("wasmJs MapHandle.setLayerSourceLayer is not implemented yet")
  }

  public actual fun layerSourceLayer(layerId: String): String =
    throw NotImplementedError("wasmJs MapHandle.layerSourceLayer is not implemented yet")

  public actual fun setLayerSourceId(layerId: String, sourceId: String) {
    throw NotImplementedError("wasmJs MapHandle.setLayerSourceId is not implemented yet")
  }

  public actual fun layerSourceId(layerId: String): String =
    throw NotImplementedError("wasmJs MapHandle.layerSourceId is not implemented yet")

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double) {
    throw NotImplementedError("wasmJs MapHandle.setLayerMinZoom is not implemented yet")
  }

  public actual fun layerMinZoom(layerId: String): Double =
    throw NotImplementedError("wasmJs MapHandle.layerMinZoom is not implemented yet")

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double) {
    throw NotImplementedError("wasmJs MapHandle.setLayerMaxZoom is not implemented yet")
  }

  public actual fun layerMaxZoom(layerId: String): Double =
    throw NotImplementedError("wasmJs MapHandle.layerMaxZoom is not implemented yet")

  public actual fun setLayerVisibility(layerId: String, visibility: StyleLayerVisibility) {
    throw NotImplementedError("wasmJs MapHandle.setLayerVisibility is not implemented yet")
  }

  public actual fun layerVisibility(layerId: String): StyleLayerVisibility =
    throw NotImplementedError("wasmJs MapHandle.layerVisibility is not implemented yet")

  public actual fun requestRepaint() {
    throw NotImplementedError("wasmJs MapHandle.requestRepaint is not implemented yet")
  }

  public actual fun requestStillImage() {
    throw NotImplementedError("wasmJs MapHandle.requestStillImage is not implemented yet")
  }

  public actual var debugOptions: Set<DebugOption>
    get() = throw NotImplementedError("wasmJs MapHandle.debugOptions is not implemented yet")
    set(value) {
      throw NotImplementedError("wasmJs MapHandle.debugOptions is not implemented yet")
    }

  public actual var isRenderingStatsViewEnabled: Boolean
    get() =
      throw NotImplementedError(
        "wasmJs MapHandle.isRenderingStatsViewEnabled is not implemented yet"
      )
    set(value) {
      throw NotImplementedError(
        "wasmJs MapHandle.isRenderingStatsViewEnabled is not implemented yet"
      )
    }

  public actual val isFullyLoaded: Boolean
    get() = throw NotImplementedError("wasmJs MapHandle.isFullyLoaded is not implemented yet")

  public actual fun dumpDebugLogs() {
    throw NotImplementedError("wasmJs MapHandle.dumpDebugLogs is not implemented yet")
  }

  public actual val size: MapSize
    get() = throw NotImplementedError("wasmJs MapHandle.size is not implemented yet")

  public actual var viewportOptions: ViewportOptions
    get() = throw NotImplementedError("wasmJs MapHandle.viewportOptions is not implemented yet")
    set(value) {
      throw NotImplementedError("wasmJs MapHandle.viewportOptions is not implemented yet")
    }

  public actual var tileOptions: TileOptions
    get() = throw NotImplementedError("wasmJs MapHandle.tileOptions is not implemented yet")
    set(value) {
      throw NotImplementedError("wasmJs MapHandle.tileOptions is not implemented yet")
    }

  public actual val camera: CameraOptions
    get() = throw NotImplementedError("wasmJs MapHandle.camera is not implemented yet")

  public actual fun jumpTo(camera: CameraOptions) {
    throw NotImplementedError("wasmJs MapHandle.jumpTo is not implemented yet")
  }

  public actual fun easeTo(camera: CameraOptions, animation: AnimationOptions?) {
    throw NotImplementedError("wasmJs MapHandle.easeTo is not implemented yet")
  }

  public actual fun flyTo(camera: CameraOptions, animation: AnimationOptions?) {
    throw NotImplementedError("wasmJs MapHandle.flyTo is not implemented yet")
  }

  public actual fun moveBy(deltaX: Double, deltaY: Double) {
    throw NotImplementedError("wasmJs MapHandle.moveBy is not implemented yet")
  }

  public actual fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions?) {
    throw NotImplementedError("wasmJs MapHandle.moveByAnimated is not implemented yet")
  }

  public actual fun scaleBy(scale: Double, anchor: ScreenPoint?) {
    throw NotImplementedError("wasmJs MapHandle.scaleBy is not implemented yet")
  }

  public actual fun scaleByAnimated(
    scale: Double,
    anchor: ScreenPoint?,
    animation: AnimationOptions?,
  ) {
    throw NotImplementedError("wasmJs MapHandle.scaleByAnimated is not implemented yet")
  }

  public actual fun rotateBy(first: ScreenPoint, second: ScreenPoint) {
    throw NotImplementedError("wasmJs MapHandle.rotateBy is not implemented yet")
  }

  public actual fun rotateByAnimated(
    first: ScreenPoint,
    second: ScreenPoint,
    animation: AnimationOptions?,
  ) {
    throw NotImplementedError("wasmJs MapHandle.rotateByAnimated is not implemented yet")
  }

  public actual fun pitchBy(pitch: Double) {
    throw NotImplementedError("wasmJs MapHandle.pitchBy is not implemented yet")
  }

  public actual fun pitchByAnimated(pitch: Double, animation: AnimationOptions?) {
    throw NotImplementedError("wasmJs MapHandle.pitchByAnimated is not implemented yet")
  }

  public actual fun cancelTransitions() {
    throw NotImplementedError("wasmJs MapHandle.cancelTransitions is not implemented yet")
  }

  public actual var isGestureInProgress: Boolean
    get() = throw NotImplementedError("wasmJs MapHandle.isGestureInProgress is not implemented yet")
    set(value) {
      throw NotImplementedError("wasmJs MapHandle.isGestureInProgress is not implemented yet")
    }

  public actual fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): CameraOptions =
    throw NotImplementedError("wasmJs MapHandle.cameraForLatLngBounds is not implemented yet")

  public actual fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): CameraOptions =
    throw NotImplementedError("wasmJs MapHandle.cameraForLatLngs is not implemented yet")

  public actual fun cameraForGeometry(
    geometry: Geometry,
    fitOptions: CameraFitOptions?,
  ): CameraOptions =
    throw NotImplementedError("wasmJs MapHandle.cameraForGeometry is not implemented yet")

  public actual fun latLngBoundsForCamera(camera: CameraOptions): LatLngBounds =
    throw NotImplementedError("wasmJs MapHandle.latLngBoundsForCamera is not implemented yet")

  public actual fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): LatLngBounds =
    throw NotImplementedError(
      "wasmJs MapHandle.latLngBoundsForCameraUnwrapped is not implemented yet"
    )

  public actual var bounds: BoundOptions
    get() = throw NotImplementedError("wasmJs MapHandle.bounds is not implemented yet")
    set(value) {
      throw NotImplementedError("wasmJs MapHandle.bounds is not implemented yet")
    }

  public actual var freeCameraOptions: FreeCameraOptions
    get() = throw NotImplementedError("wasmJs MapHandle.freeCameraOptions is not implemented yet")
    set(value) {
      throw NotImplementedError("wasmJs MapHandle.freeCameraOptions is not implemented yet")
    }

  public actual var projectionMode: ProjectionModeOptions
    get() = throw NotImplementedError("wasmJs MapHandle.projectionMode is not implemented yet")
    set(value) {
      throw NotImplementedError("wasmJs MapHandle.projectionMode is not implemented yet")
    }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint =
    throw NotImplementedError("wasmJs MapHandle.pixelForLatLng is not implemented yet")

  public actual fun latLngForPixel(point: ScreenPoint): LatLng =
    throw NotImplementedError("wasmJs MapHandle.latLngForPixel is not implemented yet")

  public actual fun pixelsForLatLngs(coordinates: List<LatLng>): List<ScreenPoint> =
    throw NotImplementedError("wasmJs MapHandle.pixelsForLatLngs is not implemented yet")

  public actual fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng> =
    throw NotImplementedError("wasmJs MapHandle.latLngsForPixels is not implemented yet")

  public actual fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor
  ): RenderSessionHandle =
    throw NotImplementedError("wasmJs MapHandle.attachMetalOwnedTexture is not implemented yet")

  public actual fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor
  ): RenderSessionHandle =
    throw NotImplementedError("wasmJs MapHandle.attachMetalBorrowedTexture is not implemented yet")

  public actual fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor
  ): RenderSessionHandle =
    throw NotImplementedError("wasmJs MapHandle.attachVulkanOwnedTexture is not implemented yet")

  public actual fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor
  ): RenderSessionHandle =
    throw NotImplementedError("wasmJs MapHandle.attachVulkanBorrowedTexture is not implemented yet")

  public actual fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor
  ): RenderSessionHandle =
    throw NotImplementedError("wasmJs MapHandle.attachOpenGLOwnedTexture is not implemented yet")

  public actual fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): RenderSessionHandle =
    throw NotImplementedError("wasmJs MapHandle.attachOpenGLBorrowedTexture is not implemented yet")

  public actual fun attachMetalSurface(descriptor: MetalSurfaceDescriptor): RenderSessionHandle =
    throw NotImplementedError("wasmJs MapHandle.attachMetalSurface is not implemented yet")

  public actual fun attachVulkanSurface(descriptor: VulkanSurfaceDescriptor): RenderSessionHandle =
    throw NotImplementedError("wasmJs MapHandle.attachVulkanSurface is not implemented yet")

  public actual fun attachOpenGLSurface(descriptor: OpenGLSurfaceDescriptor): RenderSessionHandle =
    throw NotImplementedError("wasmJs MapHandle.attachOpenGLSurface is not implemented yet")

  public actual fun createProjection(): MapProjectionHandle =
    throw NotImplementedError("wasmJs MapHandle.createProjection is not implemented yet")

  public actual override fun close() {
    throw NotImplementedError("wasmJs MapHandle.close is not implemented yet")
  }

  public actual companion object {
    public actual fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle =
      throw NotImplementedError("wasmJs MapHandle.create is not implemented yet")
  }
}
