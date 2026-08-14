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

/** Owned map handle. Platform actuals own the native map carrier. */
public expect class MapHandle : AutoCloseable {
  public val isClosed: Boolean

  public fun runtime(): RuntimeHandle

  /**
   * Map-originated event types that this map queues, [RuntimeEventMask.ALL] until a host narrows
   * it.
   *
   * The setter reads the [RuntimeEventMask.ALL_MAP_EVENTS] bits and ignores the rest, so a host
   * reads this mask, changes one bit, and writes it back. Narrowing gates later events and keeps
   * queued ones.
   */
  public var eventMask: RuntimeEventMask

  /**
   * Starts loading the style at [url]. The call returns once the request is queued; a load failure
   * reports only as a `MAP_LOADING_FAILED` runtime event.
   *
   * @see org.maplibre.nativeffi.runtime.RuntimeHandle.drainEvents
   */
  public fun setStyleUrl(url: String)

  /**
   * Loads [json] as the map style.
   *
   * Malformed JSON throws [org.maplibre.nativeffi.error.NativeErrorException] and also enqueues a
   * `MAP_LOADING_FAILED` runtime event carrying the same message.
   *
   * @see org.maplibre.nativeffi.runtime.RuntimeHandle.drainEvents
   */
  public fun setStyleJson(json: ByteArray)

  /**
   * Returns the style document this map's style was last parsed from, byte-for-byte, or an empty
   * byte array when no document has been parsed. Runtime mutations do not change it.
   */
  public fun loadedStyleJson(): ByteArray

  /**
   * Returns the URL this map's style was last requested from, recorded when the request is made
   * rather than when it completes, or an empty string when no URL is available.
   */
  public fun styleUrl(): String

  public fun addStyleSourceJson(sourceId: String, sourceJson: ByteArray)

  public fun removeStyleSource(sourceId: String): Boolean

  public fun styleSourceExists(sourceId: String): Boolean

  public fun styleSourceType(sourceId: String): SourceType?

  public fun styleSourceInfo(sourceId: String): SourceInfo?

  public fun styleSourceIds(): List<String>

  public fun addGeoJsonSourceUrl(sourceId: String, url: String, options: GeoJsonSourceOptions?)

  /**
   * Adds a GeoJSON source with prepared inline data. The call borrows [data], and the source adopts
   * the options the data was prepared with, fixed for the lifetime of the source.
   */
  public fun addGeoJsonSourceData(sourceId: String, data: GeoJsonSourceDataHandle)

  public fun setGeoJsonSourceUrl(sourceId: String, url: String)

  /**
   * Updates one GeoJSON source with prepared inline data. The call borrows [data]. The data must
   * have been prepared with options equal to the options the source was added with,
   * `clusterProperties` excepted; a mismatch is rejected.
   */
  public fun setGeoJsonSourceData(sourceId: String, data: GeoJsonSourceDataHandle)

  /**
   * Overrides one GeoJSON source's synchronous tiling at runtime. While [enabled] is true, the
   * source slices requested tiles inline during the update pass, as if the source's options had set
   * [GeoJsonSourceOptions.synchronousTiling]; false restores the option the source was added with.
   */
  public fun setGeoJsonSourceSynchronousTiling(sourceId: String, enabled: Boolean)

  /**
   * Adds a custom geometry source that calls [options] back for tile data.
   *
   * The source belongs to this map's current style. Its callback state lives until the source
   * leaves the style, which happens when [removeStyleSource] removes it, when a style load replaces
   * the style that held it, or when this map closes. Native reports that moment on the map owner
   * thread, and the binding closes the source's callbacks there, waiting for any in-flight tile
   * callback to return.
   */
  public fun addCustomGeometrySource(sourceId: String, options: CustomGeometrySourceOptions)

  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
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

  public fun addStyleLayerJson(layerJson: ByteArray, beforeLayerId: String)

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

  public fun styleLayerJson(layerId: String): ByteArray?

  public fun setStyleLightJson(lightJson: ByteArray)

  public fun setStyleLightProperty(propertyName: String, value: ByteArray)

  public fun styleLightProperty(propertyName: String): ByteArray?

  /**
   * Sets the style's global transition options, replacing rather than merging. Loading a style
   * replaces these options with the ones that style declares, so apply an override after the load.
   */
  public fun setStyleTransitionOptions(options: StyleTransitionOptions)

  public fun styleTransitionOptions(): StyleTransitionOptions

  public fun setLayerProperty(layerId: String, propertyName: String, value: ByteArray)

  public fun layerProperty(layerId: String, propertyName: String): ByteArray?

  public fun setLayerFilter(layerId: String, filter: ByteArray)

  public fun clearLayerFilter(layerId: String)

  public fun layerFilter(layerId: String): ByteArray?

  /** Sets one layer's source-layer ID. Layer types that take no source are rejected. */
  public fun setLayerSourceLayer(layerId: String, sourceLayer: String)

  /** Returns one layer's source-layer ID, empty when the layer carries none. */
  public fun layerSourceLayer(layerId: String): String

  /**
   * Sets one layer's source ID. Layer types that take no source are rejected. The named source need
   * not exist yet.
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

  public fun setLayerVisibility(layerId: String, visibility: StyleLayerVisibility)

  public fun layerVisibility(layerId: String): StyleLayerVisibility

  public fun requestRepaint()

  public fun requestStillImage()

  public var debugOptions: Set<DebugOption>

  public var isRenderingStatsViewEnabled: Boolean

  public val isFullyLoaded: Boolean

  public fun dumpDebugLogs()

  /**
   * The map's logical viewport size in UI pixels and its pixel ratio. The scale factor is fixed for
   * the lifetime of the map and is independent of any render target's scale factor.
   */
  public val size: MapSize

  public var viewportOptions: ViewportOptions

  public var tileOptions: TileOptions

  /** The current camera. [CameraOptions.anchor] is input-only and always reads back as null. */
  public val camera: CameraOptions

  /** Applies [camera] immediately with no transition. */
  public fun jumpTo(camera: CameraOptions)

  /**
   * Transitions to [camera] along an eased path. A null [animation] uses a zero duration, so the
   * camera applies instantly.
   */
  public fun easeTo(camera: CameraOptions, animation: AnimationOptions?)

  /**
   * Transitions to [camera] along a curved flight path. A null [animation] derives a duration from
   * a default velocity of 1.2 screenfuls per second, so the camera animates.
   */
  public fun flyTo(camera: CameraOptions, animation: AnimationOptions?)

  public fun moveBy(deltaX: Double, deltaY: Double)

  /**
   * Pans the camera by a screen-space delta. A null [animation] uses a zero duration, so the move
   * applies instantly.
   */
  public fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions?)

  public fun scaleBy(scale: Double, anchor: ScreenPoint?)

  /**
   * Scales the camera around an optional screen anchor. A null [animation] uses a zero duration, so
   * the scale applies instantly.
   */
  public fun scaleByAnimated(scale: Double, anchor: ScreenPoint?, animation: AnimationOptions?)

  public fun rotateBy(first: ScreenPoint, second: ScreenPoint)

  /**
   * Rotates the camera by the angle between two screen points. A null [animation] uses a zero
   * duration, so the rotation applies instantly.
   */
  public fun rotateByAnimated(first: ScreenPoint, second: ScreenPoint, animation: AnimationOptions?)

  public fun pitchBy(pitch: Double)

  /**
   * Pitches the camera by a delta in degrees. A null [animation] uses a zero duration, so the pitch
   * applies instantly.
   */
  public fun pitchByAnimated(pitch: Double, animation: AnimationOptions?)

  public fun cancelTransitions()

  /**
   * Whether a host-driven gesture is in progress. The flag stays set until the host clears it, so
   * pair every `true` with a `false`.
   */
  public var isGestureInProgress: Boolean

  public fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): CameraOptions

  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): CameraOptions

  public fun cameraForGeometry(geometry: ByteArray, fitOptions: CameraFitOptions?): CameraOptions

  public fun latLngBoundsForCamera(camera: CameraOptions): LatLngBounds

  public fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): LatLngBounds

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
   * not be this map's owner thread. For a graphics API with a thread-current context, call this on
   * the thread where that context is current. Every session call, including close, throws
   * `WrongThreadException` from any other thread.
   *
   * Close the session before closing the map. The map applies its logical size on its own owner
   * thread, so map size and rendering lag until that thread pumps the runtime once after attaching.
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
   * Closing discards this map's queued runtime events. Closing succeeds only once every child
   * wrapper is released; a render session releases its retention when it is detached or closed.
   */
  override fun close()

  public companion object {
    public fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle
  }
}
