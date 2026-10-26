// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlin.jvm.JvmInline
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A map, which holds map state independent of any render target.
 *
 * See `mln_map` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class MapHandle
internal constructor(
  handle: Long,
  @Suppress("unused") private val parent: RuntimeHandle,
) : AsyncReleasable, AutoCloseable {
  internal val binding = HandleState("MapHandle", handle, GeneratedOwnerDisposal::map)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * close, so events drained later still match this handle.
   */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public override val isClosed: Boolean
    get() = binding.isReleased

  public override fun close() {
    dispose()
  }

  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Adds a custom geometry source. The callback may run on any native worker thread, concurrently
   * with runtime and map calls.
   *
   * See `mln_map_add_custom_geometry_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(
        C.mln_map_add_custom_geometry_source(
          handle,
          view(sourceId),
          writeCustomGeometrySourceOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Adds a custom MVT vector source. The callback may run on any native worker thread, concurrently
   * with runtime and map calls.
   *
   * See `mln_map_add_custom_mvt_vector_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(
        C.mln_map_add_custom_mvt_vector_source(
          handle,
          view(sourceId),
          writeCustomMvtVectorSourceOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Adds a GeoJSON source with prepared inline data.
   *
   * See `mln_map_add_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_add_geo_json_source_data(
          handle,
          view(sourceId),
          argument(data, data.binding),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Adds a GeoJSON source with URL data.
   *
   * See `mln_map_add_geo_json_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_add_geo_json_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeGeoJsonSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Adds an image source with inline image pixels.
   *
   * See `mln_map_add_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_add_image_source_image(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          writePremultipliedRgba8Image(image),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Adds an image source that loads its image from a URL.
   *
   * See `mln_map_add_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_add_image_source_url(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          view(url),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Adds one style layer from a full style-spec layer JSON object.
   *
   * See `mln_map_add_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addStyleLayerJson(
    layerJson: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_add_style_layer_json(
          handle,
          view(layerJson),
          view(beforeLayerId),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Adds one style source from a style-spec source JSON object.
   *
   * See `mln_map_add_style_source_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addStyleSourceJson(sourceId: String, sourceJson: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_add_style_source_json(
          handle,
          view(sourceId),
          view(sourceJson),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Submits one atomic relative camera update.
   *
   * See `mln_map_apply_camera_delta` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(C.mln_map_apply_camera_delta(handle, writeCameraDelta(delta), completion, diagnostic))
    }

  /**
   * Starts attachment of a texture target that renders on a graphics device.
   *
   * See `mln_map_attach_graphics_device_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
   */
  public fun attachGraphicsDeviceTexture(
    device: GraphicsDeviceHandle,
    descriptor: GraphicsDeviceTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession =
    nativeCall(this, binding) {
      attach(
        { out, completion ->
          C.mln_map_attach_graphics_device_texture(
            handle,
            argument(device, device.binding),
            writeGraphicsDeviceTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::NewRenderSession,
      )
    }

  /**
   * Starts attachment of a ring of caller-owned OpenGL textures.
   *
   * See `mln_map_attach_open_gl_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachOpenGlBorrowedTexture(
    descriptor: OpenGlBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession =
    nativeCall(this, binding) {
      attach(
        { out, completion ->
          C.mln_map_attach_open_gl_borrowed_texture(
            handle,
            writeOpenGlBorrowedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::NewRenderSession,
      )
    }

  /**
   * Starts attachment of a session-owned OpenGL texture ring.
   *
   * See `mln_map_attach_open_gl_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachOpenGlOwnedTexture(
    descriptor: OpenGlOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession =
    nativeCall(this, binding) {
      attach(
        { out, completion ->
          C.mln_map_attach_open_gl_owned_texture(
            handle,
            writeOpenGlOwnedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::NewRenderSession,
      )
    }

  /**
   * Starts attachment of an OpenGL surface target.
   *
   * See `mln_map_attach_open_gl_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun attachOpenGlSurface(
    descriptor: OpenGlSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession =
    nativeCall(this, binding) {
      attach(
        { out, completion ->
          C.mln_map_attach_open_gl_surface(
            handle,
            writeOpenGlSurfaceDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::NewRenderSession,
      )
    }

  /**
   * Begins a command group, which holds this map's render updates until the group ends.
   *
   * See `mln_map_begin_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun beginCommandGroup(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_begin_command_group(handle, completion, diagnostic))
    }

  /**
   * Starts an ordered query for a camera that fits a GeoJSON geometry.
   *
   * See `mln_map_camera_for_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraForGeometry(
    geometry: String,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(this, binding, { result -> readCameraOptions(CompletionBridge.value(result)) }) {
      check(
        C.mln_map_camera_for_geometry(
          handle,
          view(geometry),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Starts an ordered query for a camera that fits geographic bounds.
   *
   * See `mln_map_camera_for_lat_lng_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(this, binding, { result -> readCameraOptions(CompletionBridge.value(result)) }) {
      check(
        C.mln_map_camera_for_lat_lng_bounds(
          handle,
          writeLatLngBounds(bounds),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Starts an ordered query for a camera that fits geographic coordinates.
   *
   * See `mln_map_camera_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(this, binding, { result -> readCameraOptions(CompletionBridge.value(result)) }) {
      check(
        C.mln_map_camera_for_lat_lngs(
          handle,
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Cancels the camera transitions of the commands whose animation carried transition_id, and
   * leaves every other transition running.
   *
   * See `mln_map_cancel_camera_transition` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cancelCameraTransition(transitionId: ULong): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_cancel_camera_transition(handle, transitionId.toLong(), completion, diagnostic),
      )
    }

  /**
   * Cancels the camera transitions running when this command commits.
   *
   * See `mln_map_cancel_transitions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cancelTransitions(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_cancel_transitions(handle, completion, diagnostic))
    }

  /**
   * Consumes a map handle without observing its asynchronous retirement.
   *
   * See `mln_map_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding) { check(C.mln_map_dispose(handle, diagnostic)) }

  /**
   * Submits an ordered debug-log command.
   *
   * See `mln_map_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun dumpDebugLogs(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_dump_debug_logs(handle, completion, diagnostic))
    }

  /**
   * Ends the innermost command group that `MapHandle.beginCommandGroup()` began.
   *
   * See `mln_map_end_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun endCommandGroup(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_end_command_group(handle, completion, diagnostic))
    }

  /**
   * Starts an ordered camera read.
   *
   * See `mln_map_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun getCamera(): Deferred<CameraQueryResult> =
    nativeSubmit(
      this,
      binding,
      { result -> readCameraQueryResult(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_get_camera(handle, completion, diagnostic))
    }

  /**
   * Starts an ordered read of per-feature state from this map.
   *
   * See `mln_map_get_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getFeatureState(selector: FeatureStateSelector): Deferred<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
      check(
        C.mln_map_get_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Queries the global-state JSON object, including style defaults.
   *
   * See `mln_map_get_global_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getGlobalState(): Deferred<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
      check(C.mln_map_get_global_state(handle, completion, diagnostic))
    }

  /**
   * Copies image source coordinates.
   *
   * See `mln_map_get_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getImageSourceCoordinates(sourceId: String): Deferred<List<LatLng>> =
    nativeSubmit(
      this,
      binding,
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          16,
        ) {
          readLatLng(it)
        }
      },
    ) {
      check(C.mln_map_get_image_source_coordinates(handle, view(sourceId), completion, diagnostic))
    }

  /**
   * Starts an ordered copy of the last successfully parsed style document.
   *
   * See `mln_map_get_loaded_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getLoadedStyleJson(): Deferred<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
      check(C.mln_map_get_loaded_style_json(handle, completion, diagnostic))
    }

  /**
   * Copies the map's latest published snapshot.
   *
   * See `mln_map_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getSnapshot(): MapSnapshot =
    nativeCall(this, binding) {
      val out = sized(440, 8)
      check(C.mln_map_get_snapshot(handle, out, diagnostic))
      readMapSnapshot(out)
    }

  /**
   * Copies one complete runtime style image.
   *
   * See `mln_map_get_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleImage(imageId: String): Deferred<StyleImageInfo?> =
    nativeSubmit(
      this,
      binding,
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleImageInfo(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_image(handle, view(imageId), completion, diagnostic))
    }

  /**
   * Copies the complete metadata of one style layer.
   *
   * See `mln_map_get_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleLayer(layerId: String): Deferred<StyleLayerInfo?> =
    nativeSubmit(
      this,
      binding,
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleLayerInfo(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_layer(handle, view(layerId), completion, diagnostic))
    }

  /**
   * Serializes one style layer as a full style-spec layer JSON object.
   *
   * See `mln_map_get_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleLayerJson(layerId: String): Deferred<String?> =
    nativeSubmit(
      this,
      binding,
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readViewString(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_layer_json(handle, view(layerId), completion, diagnostic))
    }

  /**
   * Serializes one layer property as a style-spec JSON value.
   *
   * See `mln_map_get_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleLayerProperty(layerId: String, propertyName: String): Deferred<String?> =
    nativeSubmit(
      this,
      binding,
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readViewString(CompletionBridge.value(result))
      },
    ) {
      check(
        C.mln_map_get_style_layer_property(
          handle,
          view(layerId),
          view(propertyName),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Serializes one style light property as a style-spec JSON value.
   *
   * See `mln_map_get_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleLightProperty(propertyName: String): Deferred<String?> =
    nativeSubmit(
      this,
      binding,
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readViewString(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_light_property(handle, view(propertyName), completion, diagnostic))
    }

  /**
   * Copies the complete metadata of one style source.
   *
   * See `mln_map_get_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleSource(sourceId: String): Deferred<StyleSourceInfo?> =
    nativeSubmit(
      this,
      binding,
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleSourceInfo(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_source(handle, view(sourceId), completion, diagnostic))
    }

  /**
   * Reads the style's global transition options.
   *
   * See `mln_map_get_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleTransitionOptions(): Deferred<StyleTransitionOptions> =
    nativeSubmit(
      this,
      binding,
      { result -> readStyleTransitionOptions(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_get_style_transition_options(handle, completion, diagnostic))
    }

  /**
   * Starts an ordered copy of the last requested style URL.
   *
   * See `mln_map_get_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getStyleUrl(): Deferred<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
      check(C.mln_map_get_style_url(handle, completion, diagnostic))
    }

  /**
   * Invalidates custom geometry source data inside one geographic region.
   *
   * See `mln_map_invalidate_custom_geometry_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_invalidate_custom_geometry_source_region(
          handle,
          view(sourceId),
          writeLatLngBounds(bounds),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Invalidates custom geometry source data for one canonical tile.
   *
   * See `mln_map_invalidate_custom_geometry_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_invalidate_custom_geometry_source_tile(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Invalidates custom MVT vector source data for one canonical tile.
   *
   * See `mln_map_invalidate_custom_mvt_vector_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun invalidateCustomMvtVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_invalidate_custom_mvt_vector_source_tile(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Starts an ordered wrapped-bounds query for a copied camera.
   *
   * See `mln_map_lat_lng_bounds_for_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun latLngBoundsForCamera(camera: CameraOptions): Deferred<LatLngBounds> =
    nativeSubmit(this, binding, { result -> readLatLngBounds(CompletionBridge.value(result)) }) {
      check(
        C.mln_map_lat_lng_bounds_for_camera(
          handle,
          writeCameraOptions(camera),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Starts an ordered unwrapped-bounds query for a copied camera.
   *
   * See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): Deferred<LatLngBounds> =
    nativeSubmit(this, binding, { result -> readLatLngBounds(CompletionBridge.value(result)) }) {
      check(
        C.mln_map_lat_lng_bounds_for_camera_unwrapped(
          handle,
          writeCameraOptions(camera),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Starts an ordered query of every style layer in style order.
   *
   * See `mln_map_list_style_layers` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun listStyleLayers(): Deferred<List<StyleLayerInfo>> =
    nativeSubmit(
      this,
      binding,
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(56, 88),
        ) {
          readStyleLayerInfo(it)
        }
      },
    ) {
      check(C.mln_map_list_style_layers(handle, completion, diagnostic))
    }

  /**
   * Lists every style source in style order.
   *
   * See `mln_map_list_style_sources` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun listStyleSources(): Deferred<List<StyleSourceInfo>> =
    nativeSubmit(
      this,
      binding,
      { result ->
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          w(120, 152),
        ) {
          readStyleSourceInfo(it)
        }
      },
    ) {
      check(C.mln_map_list_style_sources(handle, completion, diagnostic))
    }

  /**
   * Moves one style layer before another layer or to the top.
   *
   * See `mln_map_move_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun moveStyleLayer(layerId: String, beforeLayerId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_move_style_layer(
          handle,
          view(layerId),
          view(beforeLayerId),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Releases a map asynchronously.
   *
   * See `mln_map_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public override fun release(): Deferred<Unit> =
    nativeRetire(this, binding) { check(C.mln_map_release(handle, completion, diagnostic)) }

  /**
   * Removes per-feature state from this map.
   *
   * See `mln_map_remove_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun removeFeatureState(selector: FeatureStateSelector): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_remove_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Removes one runtime style image by ID.
   *
   * See `mln_map_remove_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun removeStyleImage(imageId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_remove_style_image(handle, view(imageId), completion, diagnostic))
    }

  /**
   * Removes one style layer by ID.
   *
   * See `mln_map_remove_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun removeStyleLayer(layerId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_remove_style_layer(handle, view(layerId), completion, diagnostic))
    }

  /**
   * Removes one style source by ID.
   *
   * See `mln_map_remove_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun removeStyleSource(sourceId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_remove_style_source(handle, view(sourceId), completion, diagnostic))
    }

  /**
   * Requests a repaint for a continuous map.
   *
   * See `mln_map_request_repaint` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun requestRepaint(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_request_repaint(handle, completion, diagnostic))
    }

  /**
   * Requests one still image for a static or tile map, which renders itself.
   *
   * See `mln_map_request_still_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun requestStillImage(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_map_request_still_image(handle, completion, diagnostic))
    }

  /**
   * Submits the sole post-creation logical extent update.
   *
   * See `mln_map_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun resize(extent: LogicalExtent): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_resize(handle, writeLogicalExtent(extent), completion, diagnostic))
    }

  /**
   * Submits a copied camera-constraint command.
   *
   * See `mln_map_set_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setBounds(options: BoundOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_bounds(handle, writeBoundOptions(options), completion, diagnostic))
    }

  /**
   * Sets custom geometry source data for one canonical tile.
   *
   * See `mln_map_set_custom_geometry_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_custom_geometry_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(data),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Reports a custom geometry source error for one canonical tile.
   *
   * See `mln_map_set_custom_geometry_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setCustomGeometrySourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_custom_geometry_source_tile_error(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(message),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Sets custom MVT vector source data for one canonical tile.
   *
   * See `mln_map_set_custom_mvt_vector_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setCustomMvtVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_custom_mvt_vector_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(data),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Reports a custom MVT vector source error for one canonical tile.
   *
   * See `mln_map_set_custom_mvt_vector_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setCustomMvtVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_custom_mvt_vector_source_tile_error(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(message),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Submits a debug-overlay command.
   *
   * See `mln_map_set_debug_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setDebugOptions(options: MapDebugFlag): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_debug_options(handle, options.rawValue.toInt(), completion, diagnostic))
    }

  /**
   * Selects which map-originated event types this map queues.
   *
   * See `mln_map_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setEventMask(mask: RuntimeEventFlag): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_event_mask(handle, mask.rawValue.toLong(), completion, diagnostic))
    }

  /**
   * Submits a copied per-feature-state command.
   *
   * See `mln_map_set_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setFeatureState(
    selector: FeatureStateSelector,
    state: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          view(state),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Submits a copied free-camera command.
   *
   * See `mln_map_set_free_camera_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setFreeCameraOptions(options: FreeCameraOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_free_camera_options(
          handle,
          writeFreeCameraOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Updates one GeoJSON source with prepared inline data.
   *
   * See `mln_map_set_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_geo_json_source_data(
          handle,
          view(sourceId),
          argument(data, data.binding),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Overrides one GeoJSON source's synchronous tiling at runtime.
   *
   * See `mln_map_set_geo_json_source_synchronous_tiling` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setGeoJsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_geo_json_source_synchronous_tiling(
          handle,
          view(sourceId),
          enabled,
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Updates one GeoJSON source to load data from a URL.
   *
   * See `mln_map_set_geo_json_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setGeoJsonSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_geo_json_source_url(
          handle,
          view(sourceId),
          view(url),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Submits a global-state JSON value. JSON null restores the style default.
   *
   * See `mln_map_set_global_state_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setGlobalStateProperty(
    propertyName: String,
    value: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_global_state_property(
          handle,
          view(propertyName),
          view(value),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Updates image source coordinates.
   *
   * See `mln_map_set_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_image_source_coordinates(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Updates an image source with inline image pixels.
   *
   * See `mln_map_set_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_image_source_image(
          handle,
          view(sourceId),
          writePremultipliedRgba8Image(image),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Updates an image source to load its image from a URL.
   *
   * See `mln_map_set_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_image_source_url(handle, view(sourceId), view(url), completion, diagnostic),
      )
    }

  /**
   * Submits axonometric rendering options.
   *
   * See `mln_map_set_projection_mode` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setProjectionMode(mode: ProjectionMode): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_projection_mode(handle, writeProjectionMode(mode), completion, diagnostic),
      )
    }

  /**
   * Submits a rendering-stats visibility command.
   *
   * See `mln_map_set_rendering_stats_view_enabled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_rendering_stats_view_enabled(handle, enabled, completion, diagnostic))
    }

  /**
   * Sets one runtime style image.
   *
   * See `mln_map_set_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_style_image(
          handle,
          view(imageId),
          writePremultipliedRgba8Image(image),
          options?.let { writeStyleImageOptions(it) } ?: 0L,
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Queues an inline style JSON command. A committed command means the map applied the JSON, not
   * that the style loaded: style loading events report the resource and parse results that follow.
   *
   * See `mln_map_set_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setStyleJson(json: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_style_json(handle, view(json), completion, diagnostic))
    }

  /**
   * Sets one layer property using its MapLibre style-spec property name.
   *
   * See `mln_map_set_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLayerProperty(
    layerId: String,
    propertyName: String,
    value: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_style_layer_property(
          handle,
          view(layerId),
          view(propertyName),
          view(value),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Sets the style light from a style-spec light JSON object.
   *
   * See `mln_map_set_style_light_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLightJson(lightJson: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_style_light_json(handle, view(lightJson), completion, diagnostic))
    }

  /**
   * Sets one style light property using its MapLibre style-spec property name.
   *
   * See `mln_map_set_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLightProperty(
    propertyName: String,
    value: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_style_light_property(
          handle,
          view(propertyName),
          view(value),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Sets whether one style source stores fetched tiles in the persistent cache.
   *
   * See `mln_map_set_style_source_volatile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleSourceVolatile(
    sourceId: String,
    isVolatile: Boolean,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_style_source_volatile(
          handle,
          view(sourceId),
          isVolatile,
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Sets the style's global transition options.
   *
   * See `mln_map_set_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleTransitionOptions(
    options: StyleTransitionOptions,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_style_transition_options(
          handle,
          writeStyleTransitionOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Queues a style URL command. A committed command means the map accepted the URL, not that the
   * style loaded: style loading events report the network, decode, and parse results that follow.
   *
   * See `mln_map_set_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setStyleUrl(url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_style_url(handle, view(url), completion, diagnostic))
    }

  /**
   * Submits a copied tile-options command.
   *
   * See `mln_map_set_tile_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setTileOptions(options: MapTileOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_tile_options(handle, writeMapTileOptions(options), completion, diagnostic),
      )
    }

  /**
   * Submits a copied viewport-options command.
   *
   * See `mln_map_set_viewport_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setViewportOptions(options: MapViewportOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_viewport_options(
          handle,
          writeMapViewportOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Submits one atomic camera update.
   *
   * See `mln_map_update_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun updateCamera(update: CameraUpdate): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(C.mln_map_update_camera(handle, writeCameraUpdate(update), completion, diagnostic))
    }

  /**
   * This handle's commands without their completions. A discarding call returns once native accepts
   * the command, and native logs a command that fails as a warning.
   */
  public val discarding: MapDiscarding
    get() = MapDiscarding(this)

  override fun toString(): String =
    "MapHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}

/** The commands of a [MapHandle] that drop their completions. */
@JvmInline
public value class MapDiscarding internal constructor(internal val owner: MapHandle) {
  internal val binding
    get() = owner.binding

  internal val bindingCallbacks
    get() = owner.bindingCallbacks

  /** [MapHandle.addCustomGeometrySource] without its completion. */
  public fun addCustomGeometrySource(sourceId: String, options: CustomGeometrySourceOptions): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(
        C.mln_map_add_custom_geometry_source(
          handle,
          view(sourceId),
          writeCustomGeometrySourceOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.addCustomMvtVectorSource] without its completion. */
  public fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
  ): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(
        C.mln_map_add_custom_mvt_vector_source(
          handle,
          view(sourceId),
          writeCustomMvtVectorSourceOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.addGeoJsonSourceData] without its completion. */
  public fun addGeoJsonSourceData(sourceId: String, data: GeoJsonSourceDataHandle): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_add_geo_json_source_data(
          handle,
          view(sourceId),
          argument(data, data.binding),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.addGeoJsonSourceUrl] without its completion. */
  public fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions? = null,
  ): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_add_geo_json_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeGeoJsonSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.addImageSourceImage] without its completion. */
  public fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_add_image_source_image(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          writePremultipliedRgba8Image(image),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.addImageSourceUrl] without its completion. */
  public fun addImageSourceUrl(sourceId: String, coordinates: List<LatLng>, url: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_add_image_source_url(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          view(url),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.addStyleLayerJson] without its completion. */
  public fun addStyleLayerJson(layerJson: String, beforeLayerId: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_add_style_layer_json(
          handle,
          view(layerJson),
          view(beforeLayerId),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.addStyleSourceJson] without its completion. */
  public fun addStyleSourceJson(sourceId: String, sourceJson: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_add_style_source_json(
          handle,
          view(sourceId),
          view(sourceJson),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.applyCameraDelta] without its completion. */
  public fun applyCameraDelta(delta: CameraDelta): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(C.mln_map_apply_camera_delta(handle, writeCameraDelta(delta), completion, diagnostic))
    }

  /** [MapHandle.beginCommandGroup] without its completion. */
  public fun beginCommandGroup(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_begin_command_group(handle, completion, diagnostic))
    }

  /** [MapHandle.cancelCameraTransition] without its completion. */
  public fun cancelCameraTransition(transitionId: ULong): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_cancel_camera_transition(handle, transitionId.toLong(), completion, diagnostic),
      )
    }

  /** [MapHandle.cancelTransitions] without its completion. */
  public fun cancelTransitions(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_cancel_transitions(handle, completion, diagnostic))
    }

  /** [MapHandle.dumpDebugLogs] without its completion. */
  public fun dumpDebugLogs(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_dump_debug_logs(handle, completion, diagnostic))
    }

  /** [MapHandle.endCommandGroup] without its completion. */
  public fun endCommandGroup(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_end_command_group(handle, completion, diagnostic))
    }

  /** [MapHandle.invalidateCustomGeometrySourceRegion] without its completion. */
  public fun invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_invalidate_custom_geometry_source_region(
          handle,
          view(sourceId),
          writeLatLngBounds(bounds),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.invalidateCustomGeometrySourceTile] without its completion. */
  public fun invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileId): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_invalidate_custom_geometry_source_tile(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.invalidateCustomMvtVectorSourceTile] without its completion. */
  public fun invalidateCustomMvtVectorSourceTile(sourceId: String, tileId: CanonicalTileId): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_invalidate_custom_mvt_vector_source_tile(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.moveStyleLayer] without its completion. */
  public fun moveStyleLayer(layerId: String, beforeLayerId: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_move_style_layer(
          handle,
          view(layerId),
          view(beforeLayerId),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.removeFeatureState] without its completion. */
  public fun removeFeatureState(selector: FeatureStateSelector): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_remove_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.removeStyleImage] without its completion. */
  public fun removeStyleImage(imageId: String): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_remove_style_image(handle, view(imageId), completion, diagnostic))
    }

  /** [MapHandle.removeStyleLayer] without its completion. */
  public fun removeStyleLayer(layerId: String): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_remove_style_layer(handle, view(layerId), completion, diagnostic))
    }

  /** [MapHandle.removeStyleSource] without its completion. */
  public fun removeStyleSource(sourceId: String): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_remove_style_source(handle, view(sourceId), completion, diagnostic))
    }

  /** [MapHandle.requestRepaint] without its completion. */
  public fun requestRepaint(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_request_repaint(handle, completion, diagnostic))
    }

  /** [MapHandle.resize] without its completion. */
  public fun resize(extent: LogicalExtent): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_resize(handle, writeLogicalExtent(extent), completion, diagnostic))
    }

  /** [MapHandle.setBounds] without its completion. */
  public fun setBounds(options: BoundOptions): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_set_bounds(handle, writeBoundOptions(options), completion, diagnostic))
    }

  /** [MapHandle.setCustomGeometrySourceTileData] without its completion. */
  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: String,
  ): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_custom_geometry_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(data),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setCustomGeometrySourceTileError] without its completion. */
  public fun setCustomGeometrySourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_custom_geometry_source_tile_error(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(message),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setCustomMvtVectorSourceTileData] without its completion. */
  public fun setCustomMvtVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_custom_mvt_vector_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(data),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setCustomMvtVectorSourceTileError] without its completion. */
  public fun setCustomMvtVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_custom_mvt_vector_source_tile_error(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(message),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setDebugOptions] without its completion. */
  public fun setDebugOptions(options: MapDebugFlag): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_set_debug_options(handle, options.rawValue.toInt(), completion, diagnostic))
    }

  /** [MapHandle.setEventMask] without its completion. */
  public fun setEventMask(mask: RuntimeEventFlag): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_set_event_mask(handle, mask.rawValue.toLong(), completion, diagnostic))
    }

  /** [MapHandle.setFeatureState] without its completion. */
  public fun setFeatureState(selector: FeatureStateSelector, state: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          view(state),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setFreeCameraOptions] without its completion. */
  public fun setFreeCameraOptions(options: FreeCameraOptions): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_free_camera_options(
          handle,
          writeFreeCameraOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setGeoJsonSourceData] without its completion. */
  public fun setGeoJsonSourceData(sourceId: String, data: GeoJsonSourceDataHandle): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_geo_json_source_data(
          handle,
          view(sourceId),
          argument(data, data.binding),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setGeoJsonSourceSynchronousTiling] without its completion. */
  public fun setGeoJsonSourceSynchronousTiling(sourceId: String, enabled: Boolean): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_geo_json_source_synchronous_tiling(
          handle,
          view(sourceId),
          enabled,
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setGeoJsonSourceUrl] without its completion. */
  public fun setGeoJsonSourceUrl(sourceId: String, url: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_geo_json_source_url(
          handle,
          view(sourceId),
          view(url),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setGlobalStateProperty] without its completion. */
  public fun setGlobalStateProperty(propertyName: String, value: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_global_state_property(
          handle,
          view(propertyName),
          view(value),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setImageSourceCoordinates] without its completion. */
  public fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_image_source_coordinates(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setImageSourceImage] without its completion. */
  public fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_image_source_image(
          handle,
          view(sourceId),
          writePremultipliedRgba8Image(image),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setImageSourceUrl] without its completion. */
  public fun setImageSourceUrl(sourceId: String, url: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_image_source_url(handle, view(sourceId), view(url), completion, diagnostic),
      )
    }

  /** [MapHandle.setProjectionMode] without its completion. */
  public fun setProjectionMode(mode: ProjectionMode): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_projection_mode(handle, writeProjectionMode(mode), completion, diagnostic),
      )
    }

  /** [MapHandle.setRenderingStatsViewEnabled] without its completion. */
  public fun setRenderingStatsViewEnabled(enabled: Boolean): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_set_rendering_stats_view_enabled(handle, enabled, completion, diagnostic))
    }

  /** [MapHandle.setStyleImage] without its completion. */
  public fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions? = null,
  ): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_style_image(
          handle,
          view(imageId),
          writePremultipliedRgba8Image(image),
          options?.let { writeStyleImageOptions(it) } ?: 0L,
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setStyleJson] without its completion. */
  public fun setStyleJson(json: String): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_set_style_json(handle, view(json), completion, diagnostic))
    }

  /** [MapHandle.setStyleLayerProperty] without its completion. */
  public fun setStyleLayerProperty(layerId: String, propertyName: String, value: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_style_layer_property(
          handle,
          view(layerId),
          view(propertyName),
          view(value),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setStyleLightJson] without its completion. */
  public fun setStyleLightJson(lightJson: String): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_set_style_light_json(handle, view(lightJson), completion, diagnostic))
    }

  /** [MapHandle.setStyleLightProperty] without its completion. */
  public fun setStyleLightProperty(propertyName: String, value: String): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_style_light_property(
          handle,
          view(propertyName),
          view(value),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setStyleSourceVolatile] without its completion. */
  public fun setStyleSourceVolatile(sourceId: String, isVolatile: Boolean): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_style_source_volatile(
          handle,
          view(sourceId),
          isVolatile,
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setStyleTransitionOptions] without its completion. */
  public fun setStyleTransitionOptions(options: StyleTransitionOptions): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_style_transition_options(
          handle,
          writeStyleTransitionOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setStyleUrl] without its completion. */
  public fun setStyleUrl(url: String): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_set_style_url(handle, view(url), completion, diagnostic))
    }

  /** [MapHandle.setTileOptions] without its completion. */
  public fun setTileOptions(options: MapTileOptions): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_tile_options(handle, writeMapTileOptions(options), completion, diagnostic),
      )
    }

  /** [MapHandle.setViewportOptions] without its completion. */
  public fun setViewportOptions(options: MapViewportOptions): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_viewport_options(
          handle,
          writeMapViewportOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.updateCamera] without its completion. */
  public fun updateCamera(update: CameraUpdate): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(C.mln_map_update_camera(handle, writeCameraUpdate(update), completion, diagnostic))
    }
}
