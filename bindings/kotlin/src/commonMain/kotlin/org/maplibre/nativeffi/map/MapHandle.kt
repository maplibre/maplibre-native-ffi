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
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Owned map handle. Platform actuals own the native map carrier. */
public expect class MapHandle : AutoCloseable {
  public val isClosed: Boolean

  public fun runtime(): RuntimeHandle

  public fun setStyleUrl(url: String)

  public fun setStyleJson(json: String)

  public fun addStyleSourceJson(sourceId: String, sourceJson: JsonValue)

  public fun removeStyleSource(sourceId: String): Boolean

  public fun styleSourceExists(sourceId: String): Boolean

  public fun styleSourceType(sourceId: String): SourceType?

  public fun styleSourceInfo(sourceId: String): SourceInfo?

  public fun styleSourceIds(): List<String>

  public fun addGeoJsonSourceUrl(sourceId: String, url: String)

  public fun addGeoJsonSourceData(sourceId: String, data: GeoJson)

  public fun setGeoJsonSourceUrl(sourceId: String, url: String)

  public fun setGeoJsonSourceData(sourceId: String, data: GeoJson)

  public fun addCustomGeometrySource(sourceId: String, options: CustomGeometrySourceOptions)

  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: GeoJson,
  )

  public fun invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileId)

  public fun invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds)

  public fun addVectorSourceUrl(sourceId: String, url: String, options: TileSourceOptions?)

  public fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  )

  public fun addRasterSourceUrl(sourceId: String, url: String, options: TileSourceOptions?)

  public fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  )

  public fun addRasterDemSourceUrl(sourceId: String, url: String, options: TileSourceOptions?)

  public fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  )

  public fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  )

  public fun removeStyleImage(imageId: String): Boolean

  public fun styleImageExists(imageId: String): Boolean

  public fun styleImageInfo(imageId: String): StyleImageInfo?

  public fun copyStyleImagePremultipliedRgba8(imageId: String): StyleImage?

  public fun addImageSourceUrl(sourceId: String, coordinates: List<LatLng>, url: String)

  public fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  )

  public fun setImageSourceUrl(sourceId: String, url: String)

  public fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image)

  public fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>)

  public fun imageSourceCoordinates(sourceId: String): List<LatLng>?

  public fun addStyleLayerJson(layerJson: JsonValue, beforeLayerId: String)

  public fun addHillshadeLayer(layerId: String, sourceId: String, beforeLayerId: String)

  public fun addColorReliefLayer(layerId: String, sourceId: String, beforeLayerId: String)

  public fun addLocationIndicatorLayer(layerId: String, beforeLayerId: String)

  public fun setLocationIndicatorLocation(layerId: String, coordinate: LatLng, altitude: Double)

  public fun setLocationIndicatorBearing(layerId: String, bearing: Double)

  public fun setLocationIndicatorAccuracyRadius(layerId: String, radius: Double)

  public fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  )

  public fun removeStyleLayer(layerId: String): Boolean

  public fun styleLayerExists(layerId: String): Boolean

  public fun styleLayerType(layerId: String): String?

  public fun styleLayerIds(): List<String>

  public fun moveStyleLayer(layerId: String, beforeLayerId: String)

  public fun styleLayerJson(layerId: String): JsonValue?

  public fun setStyleLightJson(lightJson: JsonValue)

  public fun setStyleLightProperty(propertyName: String, value: JsonValue)

  public fun styleLightProperty(propertyName: String): JsonValue?

  public fun setLayerProperty(layerId: String, propertyName: String, value: JsonValue)

  public fun layerProperty(layerId: String, propertyName: String): JsonValue?

  public fun setLayerFilter(layerId: String, filter: JsonValue)

  public fun clearLayerFilter(layerId: String)

  public fun layerFilter(layerId: String): JsonValue?

  public fun requestRepaint()

  public fun requestStillImage()

  public var debugOptions: Set<DebugOption>

  public var isRenderingStatsViewEnabled: Boolean

  public val isFullyLoaded: Boolean

  public fun dumpDebugLogs()

  public var viewportOptions: ViewportOptions

  public var tileOptions: TileOptions

  public val camera: CameraOptions

  public fun jumpTo(camera: CameraOptions)

  public fun easeTo(camera: CameraOptions, animation: AnimationOptions?)

  public fun flyTo(camera: CameraOptions, animation: AnimationOptions?)

  public fun moveBy(deltaX: Double, deltaY: Double)

  public fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions?)

  public fun scaleBy(scale: Double, anchor: ScreenPoint?)

  public fun scaleByAnimated(scale: Double, anchor: ScreenPoint?, animation: AnimationOptions?)

  public fun rotateBy(first: ScreenPoint, second: ScreenPoint)

  public fun rotateByAnimated(first: ScreenPoint, second: ScreenPoint, animation: AnimationOptions?)

  public fun pitchBy(pitch: Double)

  public fun pitchByAnimated(pitch: Double, animation: AnimationOptions?)

  public fun cancelTransitions()

  public fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): CameraOptions

  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): CameraOptions

  public fun cameraForGeometry(geometry: Geometry, fitOptions: CameraFitOptions?): CameraOptions

  public fun latLngBoundsForCamera(camera: CameraOptions): LatLngBounds

  public fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): LatLngBounds

  public var bounds: BoundOptions

  public var freeCameraOptions: FreeCameraOptions

  public var projectionMode: ProjectionModeOptions

  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint

  public fun latLngForPixel(point: ScreenPoint): LatLng

  public fun pixelsForLatLngs(coordinates: List<LatLng>): List<ScreenPoint>

  public fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng>

  public fun attachMetalOwnedTexture(descriptor: MetalOwnedTextureDescriptor): RenderSessionHandle

  public fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor
  ): RenderSessionHandle

  public fun attachVulkanOwnedTexture(descriptor: VulkanOwnedTextureDescriptor): RenderSessionHandle

  public fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor
  ): RenderSessionHandle

  public fun attachOpenGLOwnedTexture(descriptor: OpenGLOwnedTextureDescriptor): RenderSessionHandle

  public fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): RenderSessionHandle

  public fun attachMetalSurface(descriptor: MetalSurfaceDescriptor): RenderSessionHandle

  public fun attachVulkanSurface(descriptor: VulkanSurfaceDescriptor): RenderSessionHandle

  public fun attachOpenGLSurface(descriptor: OpenGLSurfaceDescriptor): RenderSessionHandle

  public fun createProjection(): MapProjectionHandle

  override fun close()

  public companion object {
    public fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle
  }
}
