// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.map

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.generated.BoundOptions
import org.maplibre.nativeffi.generated.CameraDelta
import org.maplibre.nativeffi.generated.CameraFitOptions
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraQueryResult
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.CanonicalTileId
import org.maplibre.nativeffi.generated.CustomGeometrySourceOptions
import org.maplibre.nativeffi.generated.CustomMvtVectorSourceOptions
import org.maplibre.nativeffi.generated.FeatureStateSelector
import org.maplibre.nativeffi.generated.FreeCameraOptions
import org.maplibre.nativeffi.generated.GeojsonSourceOptions
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.LatLngBounds
import org.maplibre.nativeffi.generated.LocationIndicatorImageKind
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapDebugOption
import org.maplibre.nativeffi.generated.MapSnapshot
import org.maplibre.nativeffi.generated.MapTileOptions
import org.maplibre.nativeffi.generated.MapViewportOptions
import org.maplibre.nativeffi.generated.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.MetalSurfaceDescriptor
import org.maplibre.nativeffi.generated.OpenglBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglSurfaceDescriptor
import org.maplibre.nativeffi.generated.PremultipliedRgba8Image
import org.maplibre.nativeffi.generated.ProjectionMode
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RuntimeEventMask
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.generated.StyleImageOptions
import org.maplibre.nativeffi.generated.StyleImageResult
import org.maplibre.nativeffi.generated.StyleImageStretchesResult
import org.maplibre.nativeffi.generated.StyleLayerEntry
import org.maplibre.nativeffi.generated.StyleLayerResult
import org.maplibre.nativeffi.generated.StyleLayerVisibility
import org.maplibre.nativeffi.generated.StyleSourceResult
import org.maplibre.nativeffi.generated.StyleSourceTileUrlsResult
import org.maplibre.nativeffi.generated.StyleTileSourceOptions
import org.maplibre.nativeffi.generated.StyleTransitionOptions
import org.maplibre.nativeffi.generated.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.generated.WebgpuBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.WebgpuOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.WebgpuSurfaceDescriptor
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.runtime.CommandCompletion

public expect abstract class GeneratedMapOperations internal constructor() {
  public fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion>

  public fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion>

  public fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
  ): Deferred<CommandCompletion>

  public fun addGeojsonSourceData(
    sourceId: String,
    data: org.maplibre.nativeffi.style.GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion>

  public fun addGeojsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeojsonSourceOptions? = null,
  ): Deferred<CommandCompletion>

  public fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion>

  public fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion>

  public fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion>

  public fun addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion>

  public fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion>

  public fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion>

  public fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion>

  public fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion>

  public fun addStyleLayerJson(
    layerJson: ByteArray,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion>

  public fun addStyleSourceJson(
    sourceId: String,
    sourceJson: ByteArray,
  ): Deferred<CommandCompletion>

  public fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion>

  public fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion>

  public fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion>

  public fun cameraForGeometry(
    geometry: ByteArray,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions>

  public fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions>

  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions>

  public fun cameraQuery(): Deferred<CameraQueryResult>

  public fun cameraSnapshotGet(): MapCameraSnapshotGetResult

  public fun cancelTransitions(): Deferred<CommandCompletion>

  public fun copyLayerSourceId(layerId: String): Deferred<String?>

  public fun copyLayerSourceLayer(layerId: String): Deferred<String?>

  public fun copyStyleImagePremultipliedRgba8(imageId: String): Deferred<ByteArray?>

  public fun copyStyleImageStretches(imageId: String): Deferred<StyleImageStretchesResult?>

  public fun copyStyleSourceAttribution(sourceId: String): Deferred<String?>

  public fun copyStyleSourceUrl(sourceId: String): Deferred<String?>

  public fun dispose(): Unit

  public fun dumpDebugLogs(): Deferred<CommandCompletion>

  public fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray>

  public fun getGlobalState(): Deferred<ByteArray>

  public fun getImageSourceCoordinates(sourceId: String): Deferred<List<LatLng>?>

  public fun getLayerFilter(layerId: String): Deferred<ByteArray?>

  public fun getLayerProperty(layerId: String, propertyName: String): Deferred<ByteArray?>

  public fun getStyleImageInfo(imageId: String): Deferred<StyleImageResult?>

  public fun getStyleLayerInfo(layerId: String): Deferred<StyleLayerResult?>

  public fun getStyleLayerJson(layerId: String): Deferred<ByteArray?>

  public fun getStyleLightProperty(propertyName: String): Deferred<ByteArray?>

  public fun getStyleSourceInfo(sourceId: String): Deferred<StyleSourceResult?>

  public fun getStyleSourceTileUrls(sourceId: String): Deferred<StyleSourceTileUrlsResult?>

  public fun getStyleTransitionOptions(): Deferred<StyleTransitionOptions>

  public fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion>

  public fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion>

  public fun invalidateCustomMvtVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion>

  public fun latLngBoundsForCamera(camera: CameraOptions): Deferred<LatLngBounds>

  public fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): Deferred<LatLngBounds>

  public fun latLngForPixel(point: ScreenPoint): Deferred<LatLng>

  public fun latLngForPixelUnwrapped(point: ScreenPoint): Deferred<LatLng>

  public fun latLngsForPixels(points: List<ScreenPoint>): Deferred<List<LatLng>>

  public fun latLngsForPixelsUnwrapped(points: List<ScreenPoint>): Deferred<List<LatLng>>

  public fun listStyleLayerIds(): Deferred<List<String>>

  public fun listStyleLayers(): Deferred<List<StyleLayerEntry>>

  public fun listStyleSourceIds(): Deferred<List<String>>

  public fun loadedStyleJson(): Deferred<ByteArray>

  public fun metersPerPixelAtLatitude(latitude: Double): Deferred<Double>

  public fun moveStyleLayer(
    layerId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion>

  public fun pixelForLatLng(coordinate: LatLng): Deferred<ScreenPoint>

  public fun pixelsForLatLngs(coordinates: List<LatLng>): Deferred<List<ScreenPoint>>

  public fun projectionCreate(): Deferred<org.maplibre.nativeffi.map.MapProjectionHandle>

  public fun release(): Deferred<Unit>

  public fun removeFeatureState(selector: FeatureStateSelector): Deferred<CommandCompletion>

  public fun removeStyleImage(imageId: String): Deferred<CommandCompletion>

  public fun removeStyleLayer(layerId: String): Deferred<CommandCompletion>

  public fun removeStyleSource(sourceId: String): Deferred<CommandCompletion>

  public fun requestRepaint(): Deferred<CommandCompletion>

  public fun requestStillImage(): Deferred<Unit>

  public fun resize(extent: LogicalExtent): Deferred<CommandCompletion>

  public fun setBounds(options: BoundOptions): Deferred<CommandCompletion>

  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion>

  public fun setCustomMvtVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion>

  public fun setCustomMvtVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion>

  public fun setDebugOptions(options: MapDebugOption): Deferred<CommandCompletion>

  public fun setEventMask(mask: RuntimeEventMask): Deferred<CommandCompletion>

  public fun setFeatureState(
    selector: FeatureStateSelector,
    state: ByteArray,
  ): Deferred<CommandCompletion>

  public fun setFreeCameraOptions(options: FreeCameraOptions): Deferred<CommandCompletion>

  public fun setGeojsonSourceData(
    sourceId: String,
    data: org.maplibre.nativeffi.style.GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion>

  public fun setGeojsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion>

  public fun setGeojsonSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion>

  public fun setGlobalStateProperty(
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion>

  public fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion>

  public fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion>

  public fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion>

  public fun setLayerFilter(layerId: String, filter: ByteArray? = null): Deferred<CommandCompletion>

  public fun setLayerMaxZoom(layerId: String, maxZoom: Double): Deferred<CommandCompletion>

  public fun setLayerMinZoom(layerId: String, minZoom: Double): Deferred<CommandCompletion>

  public fun setLayerProperty(
    layerId: String,
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion>

  public fun setLayerSourceId(layerId: String, sourceId: String): Deferred<CommandCompletion>

  public fun setLayerSourceLayer(
    layerId: String,
    sourceLayer: String? = null,
  ): Deferred<CommandCompletion>

  public fun setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility,
  ): Deferred<CommandCompletion>

  public fun setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double,
  ): Deferred<CommandCompletion>

  public fun setLocationIndicatorBearing(
    layerId: String,
    bearing: Double,
  ): Deferred<CommandCompletion>

  public fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): Deferred<CommandCompletion>

  public fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): Deferred<CommandCompletion>

  public fun setProjectionMode(mode: ProjectionMode): Deferred<CommandCompletion>

  public fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion>

  public fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions? = null,
  ): Deferred<CommandCompletion>

  public fun setStyleJson(json: ByteArray): Deferred<CommandCompletion>

  public fun setStyleLightJson(lightJson: ByteArray): Deferred<CommandCompletion>

  public fun setStyleLightProperty(
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion>

  public fun setStyleSourceVolatile(
    sourceId: String,
    isVolatile: Boolean,
  ): Deferred<CommandCompletion>

  public fun setStyleTransitionOptions(options: StyleTransitionOptions): Deferred<CommandCompletion>

  public fun setStyleUrl(url: String): Deferred<CommandCompletion>

  public fun setTileOptions(options: MapTileOptions): Deferred<CommandCompletion>

  public fun setViewportOptions(options: MapViewportOptions): Deferred<CommandCompletion>

  public fun snapshotGet(): MapSnapshot

  public fun styleUrl(): Deferred<String>

  public fun updateCamera(update: CameraUpdate): Deferred<CommandCompletion>

  public fun metalBorrowedTextureAttach(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun metalOwnedTextureAttach(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun metalSurfaceAttach(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun openglBorrowedTextureAttach(
    descriptor: OpenglBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun openglOwnedTextureAttach(
    descriptor: OpenglOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun openglSurfaceAttach(
    descriptor: OpenglSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun vulkanBorrowedTextureAttach(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun vulkanOwnedTextureAttach(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun vulkanSurfaceAttach(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun webgpuBorrowedTextureAttach(
    descriptor: WebgpuBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun webgpuOwnedTextureAttach(
    descriptor: WebgpuOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment

  public fun webgpuSurfaceAttach(
    descriptor: WebgpuSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment
}
