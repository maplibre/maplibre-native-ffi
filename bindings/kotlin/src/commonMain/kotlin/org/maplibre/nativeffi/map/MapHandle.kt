package org.maplibre.nativeffi.map

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraSnapshot
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.render.RenderDriver
import org.maplibre.nativeffi.render.RenderSessionAttachOptions
import org.maplibre.nativeffi.render.RenderSessionAttachment
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.CommandCompletion
import org.maplibre.nativeffi.runtime.CommandDisposition
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

/** Owned map handle. Platform actuals own the native map carrier. */
public expect class MapHandle {
  public val isClosed: Boolean

  public fun runtime(): RuntimeHandle

  /**
   * Changes the map-originated event types queued after this command commits, and publishes them as
   * [MapSnapshot.eventMask].
   *
   * The command reads the [RuntimeEventMask.ALL_MAP_EVENTS] bits and ignores the rest, so a host
   * reads [MapSnapshot.eventMask], changes one bit, and writes it back. Narrowing gates later
   * events and keeps queued ones.
   */
  public fun setEventMask(value: RuntimeEventMask): Deferred<CommandCompletion>

  /**
   * Starts loading the style at [url]. The call returns once the request is queued; a load failure
   * reports only as a `MAP_LOADING_FAILED` runtime event.
   *
   * @see org.maplibre.nativeffi.runtime.RuntimeHandle.drainEvents
   */
  public fun setStyleUrl(url: String): Deferred<CommandCompletion>

  /**
   * Loads [json] as the map style. The bytes are copied before the call returns, and the map worker
   * parses them.
   *
   * Malformed JSON completes with [CommandDisposition.FAILED] and
   * [org.maplibre.nativeffi.error.MaplibreStatus.NATIVE_ERROR], and also enqueues a
   * `MAP_LOADING_FAILED` runtime event carrying the same message.
   *
   * @see org.maplibre.nativeffi.runtime.RuntimeHandle.drainEvents
   */
  public fun setStyleJson(json: ByteArray): Deferred<CommandCompletion>

  /**
   * Submits one copied per-feature-state command. [value] must hold one UTF-8 JSON object. Feature
   * state belongs to this map; render sessions push it into the renderer on the next render update.
   */
  public fun setFeatureState(
    selector: FeatureStateSelector,
    value: ByteArray,
  ): Deferred<CommandCompletion>

  /**
   * Reads per-feature state behind commands accepted before this call. The bytes hold one JSON
   * object; missing feature state reads as an empty object. The read copies this map's store, not
   * the last rendered frame, so it does not require a render session or a loaded source.
   */
  public fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray>

  /**
   * Removes per-feature state. [FeatureStateSelector.featureId] and [FeatureStateSelector.stateKey]
   * narrow the removal: passing both removes one state key from one feature, passing only a feature
   * ID removes all state for that feature, and passing neither removes all feature state for the
   * source and source-layer.
   */
  public fun removeFeatureState(selector: FeatureStateSelector): Deferred<CommandCompletion>

  /**
   * Returns the style document this map's style was last parsed from, byte-for-byte, or an empty
   * byte array when no document has been parsed. Runtime mutations do not change it.
   */
  public fun loadedStyleJson(): Deferred<ByteArray>

  /**
   * Returns the URL this map's style was last requested from, recorded when the request is made
   * rather than when it completes, or an empty string when no URL is available.
   */
  public fun styleUrl(): Deferred<String>

  public fun addStyleSourceJson(
    sourceId: String,
    sourceJson: ByteArray,
  ): Deferred<CommandCompletion>

  /**
   * Removes one style source. The command completes with [CommandDisposition.FAILED] and
   * [org.maplibre.nativeffi.error.MaplibreStatus.NOT_FOUND] when no source has [sourceId], and with
   * [org.maplibre.nativeffi.error.MaplibreStatus.INVALID_STATE] when a layer still uses the source.
   */
  public fun removeStyleSource(sourceId: String): Deferred<CommandCompletion>

  /** Returns one source's copied metadata, or null when no source carries [sourceId]. */
  public fun styleSourceInfo(sourceId: String): Deferred<SourceInfo?>

  /**
   * Sets whether [sourceId] stores fetched tiles in persistent storage. Source types that do not
   * fetch tiles retain the value only for [styleSourceInfo]. The command completes with
   * [CommandDisposition.FAILED] and [org.maplibre.nativeffi.error.MaplibreStatus.NOT_FOUND] when no
   * source has [sourceId].
   */
  public fun setStyleSourceVolatile(
    sourceId: String,
    isVolatile: Boolean,
  ): Deferred<CommandCompletion>

  /**
   * Returns one source's copied attribution, or null when no source carries [sourceId] and when the
   * source carries no attribution.
   */
  public fun styleSourceAttribution(sourceId: String): Deferred<String?>

  /**
   * Returns one source's copied URL, or null when no source carries [sourceId] and when the source
   * carries inline TileJSON instead of a URL.
   */
  public fun styleSourceUrl(sourceId: String): Deferred<String?>

  /**
   * Returns one source's copied inline TileJSON tile URLs. The list is empty when no source carries
   * [sourceId] and when the source loads its TileJSON from a URL.
   */
  public fun styleSourceTileUrls(sourceId: String): Deferred<List<String>>

  public fun styleSourceIds(): Deferred<List<String>>

  public fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ): Deferred<CommandCompletion>

  /**
   * Adds a GeoJSON source with prepared inline data. The call borrows [data], and the source adopts
   * the options the data was prepared with, fixed for the lifetime of the source.
   */
  public fun addGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion>

  public fun setGeoJsonSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion>

  /**
   * Updates one GeoJSON source with prepared inline data. The call borrows [data]. The data must
   * have been prepared with options equal to the options the source was added with,
   * `clusterProperties` excepted; a mismatch is rejected.
   */
  public fun setGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion>

  /**
   * Overrides one GeoJSON source's synchronous tiling at runtime. While [enabled] is true, the
   * source slices requested tiles inline during the update pass, as if the source's options had set
   * [GeoJsonSourceOptions.synchronousTiling]; false restores the option the source was added with.
   */
  public fun setGeoJsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion>

  /**
   * Adds a custom geometry source that calls [options] back for tile data.
   *
   * The source belongs to this map's current style. Its callback state lives until the source
   * leaves the style, which happens when [removeStyleSource] removes it, when a style load replaces
   * the style that held it, or when this map closes. Native reports that moment on the runtime
   * worker, and the binding closes the source's callbacks there, waiting for any in-flight tile
   * callback to return.
   */
  public fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion>

  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion>

  public fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion>

  public fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion>

  /**
   * Adds a custom MVT vector source that calls [options] back for encoded tile bytes.
   *
   * The source belongs to this map's current style. Its callback state lives until the source
   * leaves the style, which happens when [removeStyleSource] removes it, when a style load replaces
   * the style that held it, or when this map closes. Native reports that moment on the runtime
   * worker, and the binding closes the source's callbacks there, waiting for any in-flight tile
   * callback to return.
   */
  public fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
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

  public fun invalidateCustomMvtVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion>

  public fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion>

  public fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion>

  public fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion>

  public fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion>

  public fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion>

  public fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion>

  public fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ): Deferred<CommandCompletion>

  /**
   * Removes one runtime style image. The command completes with [CommandDisposition.FAILED] and
   * [org.maplibre.nativeffi.error.MaplibreStatus.NOT_FOUND] when no image has [imageId].
   */
  public fun removeStyleImage(imageId: String): Deferred<CommandCompletion>

  /** Returns one image's copied metadata, or null when no image carries [imageId]. */
  public fun styleImageInfo(imageId: String): Deferred<StyleImageInfo?>

  /**
   * Returns one runtime style image's stretchable intervals, or null when no image carries
   * [imageId]. The pair holds the horizontal intervals first.
   */
  public fun styleImageStretches(
    imageId: String
  ): Deferred<Pair<List<ImageStretch>, List<ImageStretch>>?>

  /**
   * Returns one runtime style image's tightly packed premultiplied RGBA8 pixels, or null when no
   * image carries [imageId]. [styleImageInfo] reports the dimensions those bytes describe.
   */
  public fun copyStyleImagePremultipliedRgba8(imageId: String): Deferred<ByteArray?>

  public fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion>

  public fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion>

  public fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion>

  public fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion>

  public fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion>

  public fun imageSourceCoordinates(sourceId: String): Deferred<List<LatLng>?>

  public fun addStyleLayerJson(
    layerJson: ByteArray,
    beforeLayerId: String,
  ): Deferred<CommandCompletion>

  public fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion>

  public fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion>

  public fun addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion>

  public fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): Deferred<CommandCompletion>

  public fun setLocationIndicatorBearing(
    layerId: String,
    bearing: Double,
  ): Deferred<CommandCompletion>

  public fun setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double,
  ): Deferred<CommandCompletion>

  public fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): Deferred<CommandCompletion>

  /**
   * Removes one style layer. The command completes with [CommandDisposition.FAILED] and
   * [org.maplibre.nativeffi.error.MaplibreStatus.NOT_FOUND] when no layer has [layerId].
   */
  public fun removeStyleLayer(layerId: String): Deferred<CommandCompletion>

  /** Returns one layer's copied metadata, or null when no layer carries [layerId]. */
  public fun styleLayerInfo(layerId: String): Deferred<LayerInfo?>

  public fun styleLayerIds(): Deferred<List<String>>

  public fun moveStyleLayer(layerId: String, beforeLayerId: String): Deferred<CommandCompletion>

  public fun styleLayerJson(layerId: String): Deferred<ByteArray?>

  public fun setStyleLightJson(lightJson: ByteArray): Deferred<CommandCompletion>

  public fun setStyleLightProperty(
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion>

  public fun styleLightProperty(propertyName: String): Deferred<ByteArray?>

  /**
   * Sets the style's global transition options, replacing rather than merging. Loading a style
   * replaces these options with the ones that style declares, so apply an override after the load.
   */
  public fun setStyleTransitionOptions(options: StyleTransitionOptions): Deferred<CommandCompletion>

  public fun styleTransitionOptions(): Deferred<StyleTransitionOptions>

  public fun setLayerProperty(
    layerId: String,
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion>

  public fun layerProperty(layerId: String, propertyName: String): Deferred<ByteArray?>

  public fun setLayerFilter(layerId: String, filter: ByteArray): Deferred<CommandCompletion>

  public fun clearLayerFilter(layerId: String): Deferred<CommandCompletion>

  public fun layerFilter(layerId: String): Deferred<ByteArray?>

  /** Sets one layer's source-layer ID. Layer types that take no source are rejected. */
  public fun setLayerSourceLayer(layerId: String, sourceLayer: String): Deferred<CommandCompletion>

  /** Returns one layer's source-layer ID, empty when the layer carries none. */
  public fun layerSourceLayer(layerId: String): Deferred<String>

  /**
   * Sets one layer's source ID. Layer types that take no source are rejected. The named source need
   * not exist yet.
   */
  public fun setLayerSourceId(layerId: String, sourceId: String): Deferred<CommandCompletion>

  /** Returns one layer's source ID, empty when the layer carries none. */
  public fun layerSourceId(layerId: String): Deferred<String>

  /** Sets the lowest zoom at which one layer draws. Pass negative infinity for no lower bound. */
  public fun setLayerMinZoom(layerId: String, minZoom: Double): Deferred<CommandCompletion>

  /** Sets the highest zoom at which one layer draws. Pass positive infinity for no upper bound. */
  public fun setLayerMaxZoom(layerId: String, maxZoom: Double): Deferred<CommandCompletion>

  public fun setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility,
  ): Deferred<CommandCompletion>

  /**
   * Submits a repaint command. The returned [Deferred] fails with
   * [org.maplibre.nativeffi.error.InvalidStateException] when the map is not in
   * [MapMode.CONTINUOUS].
   */
  public fun requestRepaint(): Deferred<CommandCompletion>

  /**
   * Submits one noncoalescing still-image request.
   *
   * The map must be in [MapMode.STATIC] or [MapMode.TILE]. A request submitted while another is
   * pending completes with [CommandDisposition.FAILED] and
   * [org.maplibre.nativeffi.error.MaplibreStatus.INVALID_STATE].
   */
  public fun requestStillImage(): Deferred<CommandCompletion>

  /** Copies the latest immutable state generation published by the map worker. */
  public fun snapshot(): MapSnapshot

  /**
   * Submits a debug-overlay command. The committed mask is visible through [snapshot] as
   * [MapSnapshot.debugOptions].
   */
  public fun setDebugOptions(options: Set<DebugOption>): Deferred<CommandCompletion>

  /**
   * Submits a rendering-stats visibility command. The committed value is visible through [snapshot]
   * as [MapSnapshot.renderingStatsViewEnabled].
   */
  public fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion>

  /**
   * Submits a copied viewport-options command. The committed options are visible through [snapshot]
   * as [MapSnapshot.viewportOptions].
   */
  public fun setViewportOptions(options: ViewportOptions): Deferred<CommandCompletion>

  /**
   * Submits a copied tile-options command. The committed options are visible through [snapshot] as
   * [MapSnapshot.tileOptions].
   */
  public fun setTileOptions(options: TileOptions): Deferred<CommandCompletion>

  /**
   * Submits a copied camera-constraint command. The committed constraints are visible through
   * [snapshot] as [MapSnapshot.bounds].
   */
  public fun setBounds(options: BoundOptions): Deferred<CommandCompletion>

  /**
   * Submits a copied free-camera command. The committed options are visible through [snapshot] as
   * [MapSnapshot.freeCameraOptions].
   */
  public fun setFreeCameraOptions(options: FreeCameraOptions): Deferred<CommandCompletion>

  /**
   * Submits a copied projection-mode command. The committed options are visible through [snapshot]
   * as [MapSnapshot.projectionMode].
   */
  public fun setProjectionMode(options: ProjectionModeOptions): Deferred<CommandCompletion>

  /** Submits a command that writes this map's debug state to the log. */
  public fun dumpDebugLogs(): Deferred<CommandCompletion>

  /**
   * Submits the map's logical extent. Only the width and height may change: for a [MapSize] whose
   * [MapSize.scaleFactor] differs from the one the map was created with, the returned [Deferred]
   * fails with [org.maplibre.nativeffi.error.InvalidArgumentException].
   *
   * While a render session is attached, resize through
   * [org.maplibre.nativeffi.render.RenderSessionHandle.resize], which submits this command itself;
   * a direct map resize to a different extent leaves the session waiting for an update the map
   * never publishes.
   */
  public fun resize(size: MapSize): Deferred<CommandCompletion>

  /** Copies the latest camera generation published by the map worker. */
  public fun cameraSnapshot(): CameraSnapshot

  /** Submits one atomic camera update. */
  public fun updateCamera(update: CameraUpdate): Deferred<CommandCompletion>

  /** Submits one relative camera operation. */
  public fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion>

  /**
   * Cancels the camera transitions running when this command commits.
   *
   * A cancelled transition that carried
   * [org.maplibre.nativeffi.camera.AnimationOptions.transitionId] reports its end through
   * [org.maplibre.nativeffi.runtime.RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED], the same way
   * a completed one does. Cancelling with no transition running commits and changes nothing.
   */
  public fun cancelTransitions(): Deferred<CommandCompletion>

  /** Queries an ordered camera observation behind commands accepted before this call. */
  public fun queryCamera(): Deferred<CameraSnapshot>

  /** Queries the camera that fits [bounds] inside the map's viewport. */
  public fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): Deferred<CameraOptions>

  /** Queries the camera that fits [coordinates] inside the map's viewport. */
  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): Deferred<CameraOptions>

  /** Queries the camera that fits GeoJSON Geometry [geometry] inside the map's viewport. */
  public fun cameraForGeometry(
    geometry: ByteArray,
    fitOptions: CameraFitOptions?,
  ): Deferred<CameraOptions>

  /** Queries the bounds [camera] covers, with each longitude wrapped to -180 through 180. */
  public fun latLngBoundsForCamera(camera: CameraOptions): Deferred<LatLngBounds>

  /**
   * Queries the bounds [camera] covers, with longitudes that preserve the visible world copy and
   * may fall outside -180 through 180.
   */
  public fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): Deferred<LatLngBounds>

  /**
   * Converts one coordinate to a screen point.
   *
   * Each conversion queues one ordered map query. A hot path such as a per-pointer-move conversion
   * uses [MapProjectionHandle], whose conversions are synchronous.
   */
  public fun pixelForLatLng(coordinate: LatLng): Deferred<ScreenPoint>

  /**
   * Converts one screen point to a coordinate, with the longitude wrapped to -180 through 180.
   *
   * Each conversion queues one ordered map query.
   */
  public fun latLngForPixel(point: ScreenPoint): Deferred<LatLng>

  /**
   * Converts one screen point to a coordinate whose longitude preserves the visible world copy and
   * may fall outside -180 through 180.
   *
   * Each conversion queues one ordered map query.
   */
  public fun latLngForPixelUnwrapped(point: ScreenPoint): Deferred<LatLng>

  /** Converts coordinates to screen points. The batch queues one ordered map query. */
  public fun pixelsForLatLngs(coordinates: List<LatLng>): Deferred<List<ScreenPoint>>

  /**
   * Converts screen points to coordinates, with each longitude wrapped to -180 through 180. The
   * batch queues one ordered map query.
   */
  public fun latLngsForPixels(points: List<ScreenPoint>): Deferred<List<LatLng>>

  /**
   * Converts screen points to coordinates whose longitudes preserve their visible world copies and
   * may fall outside -180 through 180. The batch queues one ordered map query.
   */
  public fun latLngsForPixelsUnwrapped(points: List<ScreenPoint>): Deferred<List<LatLng>>

  /**
   * Starts attaching a render target and returns its immediately usable session plus completion
   * operation. A caller-graphics-thread session can service attachment work while the operation is
   * pending.
   */
  public fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions = RenderSessionAttachOptions(),
  ): RenderSessionAttachment

  public fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions = RenderSessionAttachOptions(),
  ): RenderSessionAttachment

  public fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions = RenderSessionAttachOptions(),
  ): RenderSessionAttachment

  public fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions = RenderSessionAttachOptions(),
  ): RenderSessionAttachment

  public fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor,
    options: RenderSessionAttachOptions =
      RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD),
  ): RenderSessionAttachment

  public fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions =
      RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD),
  ): RenderSessionAttachment

  /**
   * Attaches a Metal `CAMetalLayer` render target.
   *
   * A null `context.device` is accepted. The session then uses `MTLCreateSystemDefaultDevice()`.
   */
  public fun attachMetalSurface(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions = RenderSessionAttachOptions(),
  ): RenderSessionAttachment

  public fun attachVulkanSurface(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions = RenderSessionAttachOptions(),
  ): RenderSessionAttachment

  public fun attachOpenGLSurface(
    descriptor: OpenGLSurfaceDescriptor,
    options: RenderSessionAttachOptions =
      RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD),
  ): RenderSessionAttachment

  public fun createProjection(): Deferred<MapProjectionHandle>

  /** Reports native map retirement. Queued events keep this map's source ID. */
  public fun close(): Deferred<Unit>

  public companion object {
    /** Creates a map without blocking the caller's coroutine. */
    public fun create(runtime: RuntimeHandle, options: MapOptions): Deferred<MapHandle>
  }
}
