package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraSnapshot
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
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
import org.maplibre.nativeffi.render.RenderSessionAttachOptions
import org.maplibre.nativeffi.render.RenderSessionAttachment
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.RuntimeEventMask
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

/** Owned JVM FFM map handle. */
public actual class MapHandle
private constructor(
  private val runtime: RuntimeHandle,
  private val handle: NativeMap,
  cachedEventMask: RuntimeEventMask,
) {
  private val runtimeRetention = runtime.retainChild("MapHandle")
  private val core = HandleStateCore("MapHandle", handle.raw)
  private var cachedEventMask =
    RuntimeEventMask(cachedEventMask.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private val customGeometrySources =
    CustomGeometrySourceRegistry<CustomGeometrySourceState>(::releaseCallbackRoot)

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  public actual var eventMask: RuntimeEventMask
    get() = cachedEventMask
    set(value) {
      NativeAccess.ensureLoaded()
      NativeAccess.setMapEventMask(requireLiveHandle(), value.nativeValue)
      cachedEventMask =
        RuntimeEventMask(value.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)
    }

  public actual fun setStyleUrl(url: String): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.setMapStyleUrl(requireLiveHandle(), url)
  }

  public actual fun setStyleJson(json: ByteArray): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.setMapStyleJson(requireLiveHandle(), json)
  }

  public actual suspend fun loadedStyleJson(): ByteArray {
    NativeAccess.ensureLoaded()
    return NativeAccess.loadedStyleJson(requireLiveHandle(), runtime::awaitOperation)
  }

  public actual suspend fun styleUrl(): String {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleUrl(requireLiveHandle(), runtime::awaitOperation)
  }

  public actual fun addStyleSourceJson(sourceId: String, sourceJson: ByteArray): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.addStyleSourceJson(requireLiveHandle(), sourceId, sourceJson)
    }

  public actual suspend fun removeStyleSource(sourceId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeStyleSource(requireLiveHandle(), sourceId, runtime::awaitOperation)
  }

  public actual suspend fun styleSourceExists(sourceId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceExists(requireLiveHandle(), sourceId, runtime::awaitOperation)
  }

  public actual suspend fun styleSourceType(sourceId: String): SourceType? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceType(requireLiveHandle(), sourceId, runtime::awaitOperation)
  }

  public actual suspend fun styleSourceInfo(sourceId: String): SourceInfo? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceInfo(requireLiveHandle(), sourceId, runtime::awaitOperation)
  }

  public actual suspend fun styleSourceIds(): List<String> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleSourceIds(requireLiveHandle(), runtime::awaitOperation)
  }

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addGeoJsonSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addGeoJsonSourceData(
    sourceId: String,
    data: ByteArray,
    options: GeoJsonSourceOptions?,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addGeoJsonSourceData(requireLiveHandle(), sourceId, data, options)
  }

  public actual fun setGeoJsonSourceUrl(sourceId: String, url: String): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setGeoJsonSourceUrl(requireLiveHandle(), sourceId, url)
    }

  public actual fun setGeoJsonSourceData(sourceId: String, data: ByteArray): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setGeoJsonSourceData(requireLiveHandle(), sourceId, data)
    }

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): ULong {
    NativeAccess.ensureLoaded()
    val registry = customGeometrySources
    val sourceState = CustomGeometrySourceState(options) { registry.remove(sourceId) }
    var commandId = 0L
    registry.install(sourceId, sourceState) {
      commandId =
        NativeAccess.addCustomGeometrySource(
          requireLiveHandle(),
          sourceId,
          sourceState.descriptor(),
        )
      HandleLeakCleaner.retainNativeCallbackRoot(sourceState)
    }
    return commandId.toULong()
  }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.setCustomGeometrySourceTileData(requireLiveHandle(), sourceId, tileId, data)
  }

  public actual fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.invalidateCustomGeometrySourceTile(requireLiveHandle(), sourceId, tileId)
  }

  public actual fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.invalidateCustomGeometrySourceRegion(requireLiveHandle(), sourceId, bounds)
  }

  public actual fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addVectorSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addVectorSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterDemSourceUrl(requireLiveHandle(), sourceId, url, options)
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addRasterDemSourceTiles(requireLiveHandle(), sourceId, tiles, options)
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleImage(requireLiveHandle(), imageId, image, options)
  }

  public actual suspend fun removeStyleImage(imageId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeStyleImage(requireLiveHandle(), imageId, runtime::awaitOperation)
  }

  public actual suspend fun styleImageExists(imageId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleImageExists(requireLiveHandle(), imageId, runtime::awaitOperation)
  }

  public actual suspend fun styleImageInfo(imageId: String): StyleImageInfo? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleImageInfo(requireLiveHandle(), imageId, runtime::awaitOperation)
  }

  public actual suspend fun copyStyleImagePremultipliedRgba8(imageId: String): StyleImage? {
    NativeAccess.ensureLoaded()
    return NativeAccess.copyStyleImagePremultipliedRgba8(
      requireLiveHandle(),
      imageId,
      runtime::awaitOperation,
    )
  }

  public actual fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addImageSourceUrl(requireLiveHandle(), sourceId, coordinates, url)
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addImageSourceImage(requireLiveHandle(), sourceId, coordinates, image)
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setImageSourceUrl(requireLiveHandle(), sourceId, url)
    }

  public actual fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setImageSourceImage(requireLiveHandle(), sourceId, image)
    }

  public actual fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setImageSourceCoordinates(requireLiveHandle(), sourceId, coordinates)
    }

  public actual suspend fun imageSourceCoordinates(sourceId: String): List<LatLng>? {
    NativeAccess.ensureLoaded()
    return NativeAccess.imageSourceCoordinates(
      requireLiveHandle(),
      sourceId,
      runtime::awaitOperation,
    )
  }

  public actual fun addStyleLayerJson(layerJson: ByteArray, beforeLayerId: String): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.addStyleLayerJson(requireLiveHandle(), layerJson, beforeLayerId)
    }

  public actual fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addHillshadeLayer(requireLiveHandle(), layerId, sourceId, beforeLayerId)
  }

  public actual fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.addColorReliefLayer(requireLiveHandle(), layerId, sourceId, beforeLayerId)
  }

  public actual fun addLocationIndicatorLayer(layerId: String, beforeLayerId: String): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.addLocationIndicatorLayer(requireLiveHandle(), layerId, beforeLayerId)
    }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorLocation(requireLiveHandle(), layerId, coordinate, altitude)
  }

  public actual fun setLocationIndicatorBearing(layerId: String, bearing: Double): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setLocationIndicatorBearing(requireLiveHandle(), layerId, bearing)
    }

  public actual fun setLocationIndicatorAccuracyRadius(layerId: String, radius: Double): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setLocationIndicatorAccuracyRadius(requireLiveHandle(), layerId, radius)
    }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.setLocationIndicatorImageName(requireLiveHandle(), layerId, imageKind, imageId)
  }

  public actual suspend fun removeStyleLayer(layerId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.removeStyleLayer(requireLiveHandle(), layerId, runtime::awaitOperation)
  }

  public actual suspend fun styleLayerExists(layerId: String): Boolean {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerExists(requireLiveHandle(), layerId, runtime::awaitOperation)
  }

  public actual suspend fun styleLayerType(layerId: String): String? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerType(requireLiveHandle(), layerId, runtime::awaitOperation)
  }

  public actual suspend fun styleLayerIds(): List<String> {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerIds(requireLiveHandle(), runtime::awaitOperation)
  }

  public actual fun moveStyleLayer(layerId: String, beforeLayerId: String): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.moveStyleLayer(requireLiveHandle(), layerId, beforeLayerId)
    }

  public actual suspend fun styleLayerJson(layerId: String): ByteArray? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLayerJson(requireLiveHandle(), layerId, runtime::awaitOperation)
  }

  public actual fun setStyleLightJson(lightJson: ByteArray): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.setStyleLightJson(requireLiveHandle(), lightJson)
  }

  public actual fun setStyleLightProperty(propertyName: String, value: ByteArray): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setStyleLightProperty(requireLiveHandle(), propertyName, value)
    }

  public actual suspend fun styleLightProperty(propertyName: String): ByteArray? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleLightProperty(
      requireLiveHandle(),
      propertyName,
      runtime::awaitOperation,
    )
  }

  public actual fun setStyleTransitionOptions(options: StyleTransitionOptions): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setStyleTransitionOptions(requireLiveHandle(), options)
    }

  public actual suspend fun styleTransitionOptions(): StyleTransitionOptions {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleTransitionOptions(requireLiveHandle(), runtime::awaitOperation)
  }

  public actual fun setLayerProperty(
    layerId: String,
    propertyName: String,
    value: ByteArray,
  ): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.setLayerProperty(requireLiveHandle(), layerId, propertyName, value)
  }

  public actual suspend fun layerProperty(layerId: String, propertyName: String): ByteArray? {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerProperty(
      requireLiveHandle(),
      layerId,
      propertyName,
      runtime::awaitOperation,
    )
  }

  public actual fun setLayerFilter(layerId: String, filter: ByteArray): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setLayerFilter(requireLiveHandle(), layerId, filter)
    }

  public actual fun clearLayerFilter(layerId: String): ULong = command { outCommandId ->
    NativeAccess.ensureLoaded()
    NativeAccess.clearLayerFilter(requireLiveHandle(), layerId)
  }

  public actual suspend fun layerFilter(layerId: String): ByteArray? {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerFilter(requireLiveHandle(), layerId, runtime::awaitOperation)
  }

  public actual suspend fun styleImageStretches(
    imageId: String
  ): Pair<List<ImageStretch>, List<ImageStretch>>? {
    NativeAccess.ensureLoaded()
    return NativeAccess.styleImageStretches(requireLiveHandle(), imageId, runtime::awaitOperation)
  }

  public actual fun setLayerSourceLayer(layerId: String, sourceLayer: String): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setLayerSourceLayer(requireLiveHandle(), layerId, sourceLayer)
    }

  public actual suspend fun layerSourceLayer(layerId: String): String {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerSourceLayer(requireLiveHandle(), layerId, runtime::awaitOperation)
  }

  public actual fun setLayerSourceId(layerId: String, sourceId: String): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setLayerSourceId(requireLiveHandle(), layerId, sourceId)
    }

  public actual suspend fun layerSourceId(layerId: String): String {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerSourceId(requireLiveHandle(), layerId, runtime::awaitOperation)
  }

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setLayerMinZoom(requireLiveHandle(), layerId, minZoom)
    }

  public actual suspend fun layerMinZoom(layerId: String): Double {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerMinZoom(requireLiveHandle(), layerId, runtime::awaitOperation)
  }

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setLayerMaxZoom(requireLiveHandle(), layerId, maxZoom)
    }

  public actual suspend fun layerMaxZoom(layerId: String): Double {
    NativeAccess.ensureLoaded()
    return NativeAccess.layerMaxZoom(requireLiveHandle(), layerId, runtime::awaitOperation)
  }

  public actual fun setLayerVisibility(layerId: String, visibility: StyleLayerVisibility): ULong =
    command { outCommandId ->
      NativeAccess.ensureLoaded()
      NativeAccess.setLayerVisibility(requireLiveHandle(), layerId, visibility.nativeValue)
    }

  public actual suspend fun layerVisibility(layerId: String): StyleLayerVisibility {
    NativeAccess.ensureLoaded()
    return StyleLayerVisibility.fromNative(
      NativeAccess.layerVisibility(requireLiveHandle(), layerId, runtime::awaitOperation)
    )
  }

  public actual fun requestRepaint(): Long {
    NativeAccess.ensureLoaded()
    return NativeAccess.requestRepaint(requireLiveHandle())
  }

  public actual suspend fun requestStillImage() {
    NativeAccess.ensureLoaded()
    val operation = NativeAccess.startStillImage(requireLiveHandle())
    try {
      runtime.awaitOperation(operation)
    } finally {
      NativeAccess.releaseOperation(operation)
    }
  }

  public actual fun snapshot(): MapSnapshot {
    NativeAccess.ensureLoaded()
    return NativeAccess.mapSnapshot(requireLiveHandle())
  }

  public actual fun resize(size: MapSize): Long {
    NativeAccess.ensureLoaded()
    return NativeAccess.resizeMap(requireLiveHandle(), size)
  }

  public actual fun cameraSnapshot(): CameraSnapshot {
    NativeAccess.ensureLoaded()
    return NativeAccess.cameraSnapshot(requireLiveHandle())
  }

  public actual fun updateCamera(update: CameraUpdate): Long {
    NativeAccess.ensureLoaded()
    return NativeAccess.updateCamera(requireLiveHandle(), update)
  }

  public actual suspend fun queryCamera(): CameraSnapshot {
    NativeAccess.ensureLoaded()
    val operation = NativeAccess.startCameraQuery(requireLiveHandle())
    try {
      runtime.awaitOperation(operation)
      return NativeAccess.takeCameraQuery(operation)
    } finally {
      NativeAccess.releaseOperation(operation)
    }
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

  public actual suspend fun createProjection(): MapProjectionHandle {
    NativeAccess.ensureLoaded()
    val operation = NativeAccess.startCreateMapProjection(requireLiveHandle())
    try {
      runtime.awaitOperation(operation)
      return MapProjectionHandle(runtime, NativeAccess.takeCreatedMapProjection(operation))
    } finally {
      NativeAccess.releaseOperation(operation)
    }
  }

  public actual suspend fun close() {
    if (!core.beginClose()) return
    val operation =
      try {
        NativeAccess.startMapClose(handle)
      } catch (error: Throwable) {
        core.abortClose()
        throw error
      }
    try {
      runtime.awaitOperation(operation)
    } catch (error: Throwable) {
      core.abortClose()
      throw error
    } finally {
      NativeAccess.releaseOperation(operation)
    }
    core.completeClose {
      runtime.unregisterMap(this)
      runtimeRetention.close()
    }
  }

  public actual companion object {
    public actual suspend fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle {
      NativeAccess.ensureLoaded()
      val operation = NativeAccess.startCreateMap(runtime.nativeHandle(), options)
      try {
        runtime.awaitOperation(operation)
        return MapHandle(runtime, NativeAccess.takeCreatedMap(operation), options.eventMask).also {
          runtime.registerMap(it)
        }
      } finally {
        NativeAccess.releaseOperation(operation)
      }
    }
  }

  internal fun nativeHandleId(): Long = handle.raw

  internal fun nativeHandle(): NativeMap = requireLiveHandle()

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    core.retainChild(childTypeName)

  internal fun customGeometrySourceCountForTesting(): Int = customGeometrySources.size

  private fun requireLiveHandle(): NativeMap {
    core.requireLive()
    return handle
  }
}

private inline fun command(call: (Long) -> Long): ULong = call(0L).toULong()

private fun releaseCallbackRoot(root: AutoCloseable?) {
  HandleLeakCleaner.releaseNativeCallbackRoot(root)
  closeQuietly(root)
}

private fun closeQuietly(closeable: AutoCloseable?) {
  try {
    closeable?.close()
  } catch (_: Exception) {}
}
