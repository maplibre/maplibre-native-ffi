package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraSnapshot
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
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
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
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

/** Owned map handle. Platform actuals own the native map carrier. */
public expect class MapHandle {
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
  public fun setStyleUrl(url: String): ULong

  /**
   * Loads [json] as the map style.
   *
   * Malformed JSON throws [org.maplibre.nativeffi.error.NativeErrorException] and also enqueues a
   * `MAP_LOADING_FAILED` runtime event carrying the same message.
   *
   * @see org.maplibre.nativeffi.runtime.RuntimeHandle.drainEvents
   */
  public fun setStyleJson(json: ByteArray): ULong

  /**
   * Returns the style document this map's style was last parsed from, byte-for-byte, or an empty
   * byte array when no document has been parsed. Runtime mutations do not change it.
   */
  public suspend fun loadedStyleJson(): ByteArray

  /**
   * Returns the URL this map's style was last requested from, recorded when the request is made
   * rather than when it completes, or an empty string when no URL is available.
   */
  public suspend fun styleUrl(): String

  public fun addStyleSourceJson(sourceId: String, sourceJson: ByteArray): ULong

  /**
   * Submits a command that removes one style source. Its `COMMAND_FINISHED` event reports
   * [org.maplibre.nativeffi.error.MaplibreStatus.NOT_FOUND] when no source has [sourceId] and
   * [org.maplibre.nativeffi.error.MaplibreStatus.INVALID_STATE] when a layer still uses the source.
   */
  public fun removeStyleSource(sourceId: String): ULong

  /** Returns one source's copied metadata, or null when no source carries [sourceId]. */
  public suspend fun styleSourceInfo(sourceId: String): SourceInfo?

  public suspend fun styleSourceIds(): List<String>

  public fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ): ULong

  /**
   * Adds a GeoJSON source with prepared inline data. The call borrows [data], and the source adopts
   * the options the data was prepared with, fixed for the lifetime of the source.
   */
  public fun addGeoJsonSourceData(sourceId: String, data: GeoJsonSourceDataHandle): ULong

  public fun setGeoJsonSourceUrl(sourceId: String, url: String): ULong

  /**
   * Updates one GeoJSON source with prepared inline data. The call borrows [data]. The data must
   * have been prepared with options equal to the options the source was added with,
   * `clusterProperties` excepted; a mismatch is rejected.
   */
  public fun setGeoJsonSourceData(sourceId: String, data: GeoJsonSourceDataHandle): ULong

  /**
   * Overrides one GeoJSON source's synchronous tiling at runtime. While [enabled] is true, the
   * source slices requested tiles inline during the update pass, as if the source's options had set
   * [GeoJsonSourceOptions.synchronousTiling]; false restores the option the source was added with.
   */
  public fun setGeoJsonSourceSynchronousTiling(sourceId: String, enabled: Boolean): ULong

  /**
   * Adds a custom geometry source that calls [options] back for tile data.
   *
   * The source belongs to this map's current style. Its callback state lives until the source
   * leaves the style, which happens when [removeStyleSource] removes it, when a style load replaces
   * the style that held it, or when this map closes. Native reports that moment on the runtime
   * worker, and the binding closes the source's callbacks there, waiting for any in-flight tile
   * callback to return.
   */
  public fun addCustomGeometrySource(sourceId: String, options: CustomGeometrySourceOptions): ULong

  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): ULong

  public fun invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileId): ULong

  public fun invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds): ULong

  public fun addVectorSourceUrl(sourceId: String, url: String, options: TileSourceOptions?): ULong

  public fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): ULong

  public fun addRasterSourceUrl(sourceId: String, url: String, options: TileSourceOptions?): ULong

  public fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): ULong

  public fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): ULong

  public fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): ULong

  public fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ): ULong

  /**
   * Submits a command that removes one runtime style image. Its `COMMAND_FINISHED` event reports
   * [org.maplibre.nativeffi.error.MaplibreStatus.NOT_FOUND] when no image has [imageId].
   */
  public fun removeStyleImage(imageId: String): ULong

  /** Returns one image's copied metadata, or null when no image carries [imageId]. */
  public suspend fun styleImageInfo(imageId: String): StyleImageInfo?

  /**
   * Returns one runtime style image's stretchable intervals, or null when no image carries
   * [imageId]. The pair holds the horizontal intervals first.
   */
  public suspend fun styleImageStretches(
    imageId: String
  ): Pair<List<ImageStretch>, List<ImageStretch>>?

  public suspend fun copyStyleImagePremultipliedRgba8(imageId: String): StyleImage?

  public fun addImageSourceUrl(sourceId: String, coordinates: List<LatLng>, url: String): ULong

  public fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): ULong

  public fun setImageSourceUrl(sourceId: String, url: String): ULong

  public fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image): ULong

  public fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>): ULong

  public suspend fun imageSourceCoordinates(sourceId: String): List<LatLng>?

  public fun addStyleLayerJson(layerJson: ByteArray, beforeLayerId: String): ULong

  public fun addHillshadeLayer(layerId: String, sourceId: String, beforeLayerId: String): ULong

  public fun addColorReliefLayer(layerId: String, sourceId: String, beforeLayerId: String): ULong

  public fun addLocationIndicatorLayer(layerId: String, beforeLayerId: String): ULong

  public fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): ULong

  public fun setLocationIndicatorBearing(layerId: String, bearing: Double): ULong

  public fun setLocationIndicatorAccuracyRadius(layerId: String, radius: Double): ULong

  public fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): ULong

  /**
   * Submits a command that removes one style layer. Its `COMMAND_FINISHED` event reports
   * [org.maplibre.nativeffi.error.MaplibreStatus.NOT_FOUND] when no layer has [layerId].
   */
  public fun removeStyleLayer(layerId: String): ULong

  /** Returns one layer's copied metadata, or null when no layer carries [layerId]. */
  public suspend fun styleLayerInfo(layerId: String): LayerInfo?

  public suspend fun styleLayerIds(): List<String>

  public fun moveStyleLayer(layerId: String, beforeLayerId: String): ULong

  public suspend fun styleLayerJson(layerId: String): ByteArray?

  public fun setStyleLightJson(lightJson: ByteArray): ULong

  public fun setStyleLightProperty(propertyName: String, value: ByteArray): ULong

  public suspend fun styleLightProperty(propertyName: String): ByteArray?

  /**
   * Sets the style's global transition options, replacing rather than merging. Loading a style
   * replaces these options with the ones that style declares, so apply an override after the load.
   */
  public fun setStyleTransitionOptions(options: StyleTransitionOptions): ULong

  public suspend fun styleTransitionOptions(): StyleTransitionOptions

  public fun setLayerProperty(layerId: String, propertyName: String, value: ByteArray): ULong

  public suspend fun layerProperty(layerId: String, propertyName: String): ByteArray?

  public fun setLayerFilter(layerId: String, filter: ByteArray): ULong

  public fun clearLayerFilter(layerId: String): ULong

  public suspend fun layerFilter(layerId: String): ByteArray?

  /** Sets one layer's source-layer ID. Layer types that take no source are rejected. */
  public fun setLayerSourceLayer(layerId: String, sourceLayer: String): ULong

  /** Returns one layer's source-layer ID, empty when the layer carries none. */
  public suspend fun layerSourceLayer(layerId: String): String

  /**
   * Sets one layer's source ID. Layer types that take no source are rejected. The named source need
   * not exist yet.
   */
  public fun setLayerSourceId(layerId: String, sourceId: String): ULong

  /** Returns one layer's source ID, empty when the layer carries none. */
  public suspend fun layerSourceId(layerId: String): String

  /** Sets the lowest zoom at which one layer draws. Pass negative infinity for no lower bound. */
  public fun setLayerMinZoom(layerId: String, minZoom: Double): ULong

  /** Sets the highest zoom at which one layer draws. Pass positive infinity for no upper bound. */
  public fun setLayerMaxZoom(layerId: String, maxZoom: Double): ULong

  public fun setLayerVisibility(layerId: String, visibility: StyleLayerVisibility): ULong

  /** Submits a repaint command and returns its runtime-wide command ID. */
  public fun requestRepaint(): Long

  /** Suspends until one noncoalescing still-image request completes. */
  public suspend fun requestStillImage()

  /** Copies the latest immutable state generation published by the map worker. */
  public fun snapshot(): MapSnapshot

  /**
   * Submits a debug-overlay command and returns its runtime-wide command ID. The committed mask is
   * visible through [snapshot] as [MapSnapshot.debugOptions].
   */
  public fun setDebugOptions(options: Set<DebugOption>): Long

  /**
   * Submits a rendering-stats visibility command and returns its runtime-wide command ID. The
   * committed value is visible through [snapshot] as [MapSnapshot.renderingStatsViewEnabled].
   */
  public fun setRenderingStatsViewEnabled(enabled: Boolean): Long

  /**
   * Submits a copied viewport-options command and returns its runtime-wide command ID. The
   * committed options are visible through [snapshot] as [MapSnapshot.viewportOptions].
   */
  public fun setViewportOptions(options: ViewportOptions): Long

  /**
   * Submits a copied tile-options command and returns its runtime-wide command ID. The committed
   * options are visible through [snapshot] as [MapSnapshot.tileOptions].
   */
  public fun setTileOptions(options: TileOptions): Long

  /**
   * Submits a copied camera-constraint command and returns its runtime-wide command ID. The
   * committed constraints are visible through [snapshot] as [MapSnapshot.bounds].
   */
  public fun setBounds(options: BoundOptions): Long

  /**
   * Submits a copied free-camera command and returns its runtime-wide command ID. The committed
   * options are visible through [snapshot] as [MapSnapshot.freeCameraOptions].
   */
  public fun setFreeCameraOptions(options: FreeCameraOptions): Long

  /** Submits the map's logical extent and returns its runtime-wide command ID. */
  public fun resize(size: MapSize): Long

  /** Copies the latest camera generation published by the map worker. */
  public fun cameraSnapshot(): CameraSnapshot

  /** Submits one atomic camera update and returns its runtime-wide command ID. */
  public fun updateCamera(update: CameraUpdate): Long

  /** Suspends for an ordered camera observation behind commands accepted before this call. */
  public suspend fun queryCamera(): CameraSnapshot

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

  public suspend fun createProjection(): MapProjectionHandle

  /** Suspends until native map retirement completes. Queued events keep this map's source ID. */
  public suspend fun close()

  public companion object {
    /** Creates a map without blocking the caller's coroutine. */
    public suspend fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle
  }
}
