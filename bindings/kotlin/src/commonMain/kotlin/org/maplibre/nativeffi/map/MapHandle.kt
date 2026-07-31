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
import org.maplibre.nativeffi.style.TileSourceOptions

/** Owned map handle. Platform actuals own the native map carrier. */
public expect class MapHandle : AutoCloseable {
  public val isClosed: Boolean

  public fun runtime(): RuntimeHandle

  /**
   * Starts loading the style at [url].
   *
   * Loading is asynchronous and this call returns once the request is queued. A style that fails to
   * load reports only through the runtime event queue as `MAP_LOADING_FAILED`; this method returns
   * normally for an unreachable or malformed remote style.
   *
   * A well-formed style with invalid semantics, such as an unsupported `version` or a layer naming
   * a missing source, is reported as a log record rather than an event or an exception.
   *
   * @see org.maplibre.nativeffi.runtime.RuntimeHandle.pollEvent
   */
  public fun setStyleUrl(url: String)

  /**
   * Loads [json] as the map style.
   *
   * Malformed JSON throws [org.maplibre.nativeffi.error.NativeErrorException] and also enqueues a
   * `MAP_LOADING_FAILED` runtime event carrying the same message, so a caller that both catches and
   * polls observes the failure twice.
   *
   * A well-formed style with invalid semantics, such as an unsupported `version` or a layer naming
   * a missing source, is reported as a log record rather than an event or an exception.
   *
   * @see org.maplibre.nativeffi.runtime.RuntimeHandle.pollEvent
   */
  public fun setStyleJson(json: String)

  public fun addStyleSourceJson(sourceId: String, sourceJson: JsonValue)

  public fun removeStyleSource(sourceId: String): Boolean

  public fun styleSourceExists(sourceId: String): Boolean

  public fun styleSourceType(sourceId: String): SourceType?

  public fun styleSourceInfo(sourceId: String): SourceInfo?

  public fun styleSourceIds(): List<String>

  public fun addGeoJsonSourceUrl(sourceId: String, url: String, options: GeoJsonSourceOptions?)

  public fun addGeoJsonSourceData(sourceId: String, data: GeoJson, options: GeoJsonSourceOptions?)

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

  /**
   * Returns one runtime style image's stretchable intervals, or null when no image carries
   * [imageId]. The pair holds the horizontal intervals first.
   */
  public fun styleImageStretches(imageId: String): Pair<List<ImageStretch>, List<ImageStretch>>?

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

  /**
   * Sets one layer's source-layer ID.
   *
   * Layer types that take no source, such as background, are rejected.
   */
  public fun setLayerSourceLayer(layerId: String, sourceLayer: String)

  /** Returns one layer's source-layer ID, empty when the layer carries none. */
  public fun layerSourceLayer(layerId: String): String

  /**
   * Sets one layer's source ID.
   *
   * Layer types that take no source, such as background, are rejected. The named source need not
   * exist yet.
   */
  public fun setLayerSourceId(layerId: String, sourceId: String)

  /** Returns one layer's source ID, empty when the layer carries none. */
  public fun layerSourceId(layerId: String): String

  /** Sets the lowest zoom at which one layer draws. Pass negative infinity for no lower bound. */
  public fun setLayerMinZoom(layerId: String, minZoom: Double)

  /**
   * Returns the lowest zoom at which one layer draws. A layer with no lower bound reports negative
   * infinity.
   */
  public fun layerMinZoom(layerId: String): Double

  /** Sets the highest zoom at which one layer draws. Pass positive infinity for no upper bound. */
  public fun setLayerMaxZoom(layerId: String, maxZoom: Double)

  /**
   * Returns the highest zoom at which one layer draws. A layer with no upper bound reports positive
   * infinity.
   */
  public fun layerMaxZoom(layerId: String): Double

  /** Sets whether one layer draws. */
  public fun setLayerVisibility(layerId: String, visibility: StyleLayerVisibility)

  /** Returns whether one layer draws. */
  public fun layerVisibility(layerId: String): StyleLayerVisibility

  public fun requestRepaint()

  public fun requestStillImage()

  public var debugOptions: Set<DebugOption>

  public var isRenderingStatsViewEnabled: Boolean

  public val isFullyLoaded: Boolean

  public fun dumpDebugLogs()

  /**
   * The map's logical viewport size in UI pixels and its pixel ratio.
   *
   * The size starts at the creation width and height, and follows the attach and resize rules
   * documented on [MapOptions]. The scale factor is fixed for the lifetime of the map and is
   * independent of any render target's scale factor.
   */
  public val size: MapSize

  public var viewportOptions: ViewportOptions

  public var tileOptions: TileOptions

  /**
   * The current camera snapshot.
   *
   * Snapshots report position, zoom, bearing, and pitch. [CameraOptions.anchor] is input-only and
   * always reads back as null.
   *
   * @see org.maplibre.nativeffi.camera.CameraOptions
   */
  public val camera: CameraOptions

  /**
   * Applies [camera] immediately with no transition.
   *
   * @see org.maplibre.nativeffi.camera.CameraOptions
   */
  public fun jumpTo(camera: CameraOptions)

  /**
   * Transitions to [camera] along an eased path.
   *
   * A null [animation] uses a zero duration, so the camera applies instantly. Pass an
   * [AnimationOptions] with a duration to animate. This differs from [flyTo], which derives a
   * duration from a default velocity when [animation] is null.
   *
   * @see org.maplibre.nativeffi.camera.CameraOptions
   * @see org.maplibre.nativeffi.camera.AnimationOptions
   */
  public fun easeTo(camera: CameraOptions, animation: AnimationOptions?)

  /**
   * Transitions to [camera] along a curved flight path.
   *
   * A null [animation] derives a duration from a default velocity of 1.2 screenfuls per second, so
   * the camera genuinely animates. This differs from [easeTo] and the `*Animated` methods, which
   * default to a zero duration and apply instantly.
   *
   * @see org.maplibre.nativeffi.camera.CameraOptions
   * @see org.maplibre.nativeffi.camera.AnimationOptions
   */
  public fun flyTo(camera: CameraOptions, animation: AnimationOptions?)

  public fun moveBy(deltaX: Double, deltaY: Double)

  /**
   * Pans the camera by a screen-space delta.
   *
   * A null [animation] uses a zero duration, so the move applies instantly. This method delegates
   * to [easeTo] and shares its defaults rather than the curved defaults of [flyTo].
   *
   * @see org.maplibre.nativeffi.camera.AnimationOptions
   */
  public fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions?)

  public fun scaleBy(scale: Double, anchor: ScreenPoint?)

  /**
   * Scales the camera around an optional screen anchor.
   *
   * A null [animation] uses a zero duration, so the scale applies instantly. This method delegates
   * to [easeTo] and shares its defaults rather than the curved defaults of [flyTo].
   *
   * @see org.maplibre.nativeffi.camera.AnimationOptions
   */
  public fun scaleByAnimated(scale: Double, anchor: ScreenPoint?, animation: AnimationOptions?)

  public fun rotateBy(first: ScreenPoint, second: ScreenPoint)

  /**
   * Rotates the camera by the angle between two screen points.
   *
   * A null [animation] uses a zero duration, so the rotation applies instantly. This method
   * delegates to [easeTo] and shares its defaults rather than the curved defaults of [flyTo].
   *
   * @see org.maplibre.nativeffi.camera.AnimationOptions
   */
  public fun rotateByAnimated(first: ScreenPoint, second: ScreenPoint, animation: AnimationOptions?)

  public fun pitchBy(pitch: Double)

  /**
   * Pitches the camera by a delta in degrees.
   *
   * A null [animation] uses a zero duration, so the pitch applies instantly. This method delegates
   * to [easeTo] and shares its defaults rather than the curved defaults of [flyTo].
   *
   * @see org.maplibre.nativeffi.camera.AnimationOptions
   */
  public fun pitchByAnimated(pitch: Double, animation: AnimationOptions?)

  public fun cancelTransitions()

  /**
   * Whether a host-driven gesture is in progress.
   *
   * A host that decodes its own pointer gestures sets this to `true` when a gesture starts and back
   * to `false` when it ends, so the camera commands issued in between belong to one live gesture.
   * The flag stays set until the host clears it, so pair every `true` with a `false`.
   */
  public var isGestureInProgress: Boolean

  /**
   * Computes the camera that fits [bounds].
   *
   * @see org.maplibre.nativeffi.camera.CameraFitOptions
   * @see org.maplibre.nativeffi.camera.EdgeInsets
   */
  public fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): CameraOptions

  /**
   * Computes the camera that fits [coordinates].
   *
   * @see org.maplibre.nativeffi.camera.CameraFitOptions
   * @see org.maplibre.nativeffi.camera.EdgeInsets
   */
  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): CameraOptions

  /**
   * Computes the camera that fits [geometry].
   *
   * @see org.maplibre.nativeffi.camera.CameraFitOptions
   * @see org.maplibre.nativeffi.camera.EdgeInsets
   */
  public fun cameraForGeometry(geometry: Geometry, fitOptions: CameraFitOptions?): CameraOptions

  public fun latLngBoundsForCamera(camera: CameraOptions): LatLngBounds

  public fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): LatLngBounds

  /**
   * The camera bounds constraint applied to this map.
   *
   * @see org.maplibre.nativeffi.camera.BoundOptions
   */
  public var bounds: BoundOptions

  public var freeCameraOptions: FreeCameraOptions

  public var projectionMode: ProjectionModeOptions

  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint

  public fun latLngForPixel(point: ScreenPoint): LatLng

  public fun pixelsForLatLngs(coordinates: List<LatLng>): List<ScreenPoint>

  public fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng>

  /**
   * Attaches a render target to this map, returning the map's one live render session.
   *
   * The calling thread becomes the session's owner thread for the session's lifetime, and it need
   * not be this map's owner thread. Call this on the thread that will drive the session, which for
   * a graphics API with a thread-current context is the thread where that context is current. Every
   * session call, including close, reports the wrong-thread error from any other thread.
   *
   * The session does not keep this handle alive on the Kotlin side. Native keeps the map alive by
   * refusing to destroy a map that still has a session attached, so close the session before
   * closing the map.
   *
   * The map applies its logical size on its own owner thread, so the map size and rendering lag
   * until that thread pumps the runtime at least once after attaching.
   */
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

  /**
   * Releases the native map on its owner thread.
   *
   * Closing discards this map's queued runtime events and its recorded loading failure. There is no
   * flush and no terminal event, so snapshot any mirrored state synchronously before closing and
   * treat teardown as complete once this call returns.
   *
   * Closing succeeds once every child wrapper is released. A render session releases its retention
   * when it is detached or closed, whichever happens first.
   */
  override fun close()

  public companion object {
    public fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle
  }
}
