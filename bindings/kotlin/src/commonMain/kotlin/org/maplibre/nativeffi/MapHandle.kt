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
 * A map, with its style and camera.
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
   * Adds a source whose GeoJSON tiles come from callbacks.
   *
   * See `mln_map_add_custom_geometry_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.ALREADY_EXISTS` when the style already has a source named `sourceId`.
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
   * Adds a source whose vector tiles come from callbacks.
   *
   * See `mln_map_add_custom_vector_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.ALREADY_EXISTS` when the style already has a source named `sourceId`.
   */
  public fun addCustomVectorSource(
    sourceId: String,
    options: CustomVectorSourceOptions,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(
        C.mln_map_add_custom_vector_source(
          handle,
          view(sourceId),
          writeCustomVectorSourceOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Adds a GeoJSON source with prepared data.
   *
   * The source uses the options that `data` was prepared with. `data` can be released once this
   * function returns.
   *
   * See `mln_map_add_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.ALREADY_EXISTS` when the style already has a source named `sourceId`.
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
   * Adds an image source that shows `image`.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_add_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.ALREADY_EXISTS` when the style already has a source named `sourceId`.
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
          array(coordinates, 16, w(8, 8, 4)) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          writePremultipliedRgba8Image(image),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Adds a layer to the map's style.
   *
   * `layerJson` is a &#91;layer&#93;(https://maplibre.org/maplibre-style-spec/layers/) object. The
   * layer goes below the layer named `beforeLayerId`, or above every layer when `beforeLayerId` is
   * empty.
   *
   * See `mln_map_add_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no layer named `beforeLayerId`, or no source with the
   *   ID that the layer uses.
   * - `Status.ALREADY_EXISTS` when the style already has a layer with the new layer's ID.
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
   * Adds a source to the map's style.
   *
   * `sourceJson` is a &#91;source&#93;(https://maplibre.org/maplibre-style-spec/sources/) object.
   *
   * See `mln_map_add_style_source_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.ALREADY_EXISTS` when the style already has a source named `sourceId`.
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
   * Moves the camera relative to where it is, as a gesture does.
   *
   * `delta` can pan the map's content by an offset on the screen, scale the zoom, and add to the
   * bearing and pitch. An anchor keeps one screen point in place while the scale, bearing, or pitch
   * changes. An anchor needs one of those changes, and combines with an offset only when the delta
   * has no animation. The function finishes when the map applies the delta, before an animation
   * ends.
   *
   * See `mln_map_apply_camera_delta` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(C.mln_map_apply_camera_delta(handle, writeCameraDelta(delta), completion, diagnostic))
    }

  /**
   * Attaches a render session that renders the map offscreen.
   *
   * Read each image's pixels with `RenderSessionHandle.readTexture()`.
   *
   * The session renders on `device`. When `device` is null, the session creates a device with the
   * default options, and releases it when the session ends.
   *
   * See `mln_map_attach_offscreen` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_STATE` when the map already has a render session.
   * - `Status.NATIVE_ERROR` when `device` is null and the platform can't create a device.
   */
  public fun attachOffscreen(
    descriptor: OffscreenDescriptor,
    options: RenderSessionAttachOptions,
    device: OffscreenDeviceHandle? = null,
  ): NewRenderSession =
    nativeCall(this, binding) {
      attach(
        { out, completion ->
          C.mln_map_attach_offscreen(
            handle,
            writeOffscreenDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            device?.let { argument(it, it.binding) } ?: 0L,
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
   * Creates a session that renders the map into a ring of your OpenGL textures.
   *
   * See `mln_map_attach_open_gl_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.UNSUPPORTED` when `openGlSupportedContextProviderMask()` lacks the context's
   *   platform, or when the target supports none of the `producer_sync_kinds` in `options`.
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
   * Creates a session that renders the map to an OpenGL surface.
   *
   * See `mln_map_attach_open_gl_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.UNSUPPORTED` when `openGlSupportedContextProviderMask()` lacks the context's
   *   platform.
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
   * Holds back the map's changes from rendering until `MapHandle.endCommandGroup()`, so they show
   * in the same frame.
   *
   * Groups nest, and the changes render when the outermost group ends.
   *
   * See `mln_map_begin_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun beginCommandGroup(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_begin_command_group(handle, completion, diagnostic))
    }

  /**
   * Computes a camera that fits a GeoJSON geometry in the map's view.
   *
   * `geometry` is a GeoJSON geometry object with at least one coordinate, not a feature. The camera
   * keeps the map's bearing and pitch unless `fitOptions` sets them, and its zoom stays within the
   * map's zoom limits.
   *
   * See `mln_map_camera_for_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_ARGUMENT` when the padding in `fitOptions` leaves no room for the area: its
   *   left and right add up to at least the map's width, or its top and bottom to at least the
   *   map's height.
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
   * Computes a camera that fits `bounds` in the map's view.
   *
   * The camera keeps the map's bearing and pitch unless `fitOptions` sets them, and its zoom stays
   * within the map's zoom limits.
   *
   * See `mln_map_camera_for_lat_lng_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_ARGUMENT` when the padding in `fitOptions` leaves no room for the area: its
   *   left and right add up to at least the map's width, or its top and bottom to at least the
   *   map's height.
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
   * Computes a camera that fits `coordinates` in the map's view.
   *
   * `coordinates` must not be empty. The camera keeps the map's bearing and pitch unless
   * `fitOptions` sets them, and its zoom stays within the map's zoom limits.
   *
   * See `mln_map_camera_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_ARGUMENT` when the padding in `fitOptions` leaves no room for the area: its
   *   left and right add up to at least the map's width, or its top and bottom to at least the
   *   map's height.
   */
  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(this, binding, { result -> readCameraOptions(CompletionBridge.value(result)) }) {
      check(
        C.mln_map_camera_for_lat_lngs(
          handle,
          array(coordinates, 16, w(8, 8, 4)) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Stops every running camera animation where it is.
   *
   * The end handler of each stopped animation reports `CameraTransitionOutcome.CANCELLED`.
   *
   * See `mln_map_cancel_camera_transitions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cancelCameraTransitions(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_cancel_camera_transitions(handle, completion, diagnostic))
    }

  /**
   * Stops the running camera animations whose animation options set `transitionId`.
   *
   * Other animations keep running. The end handler of each stopped animation reports
   * `CameraTransitionOutcome.CANCELLED`.
   *
   * See `mln_map_cancel_camera_transitions_with_id` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cancelCameraTransitionsWithId(transitionId: ULong): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_cancel_camera_transitions_with_id(
          handle,
          transitionId.toLong(),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Releases the map in any state, and finishes in the background.
   *
   * Use it from destructors and finalizers. Otherwise, use `MapHandle.release()`.
   *
   * See `mln_map_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding) { check(C.mln_map_dispose(handle, diagnostic)) }

  /**
   * Logs debugging information about the map's style and sources.
   *
   * See `mln_map_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun dumpDebugLogs(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_dump_debug_logs(handle, completion, diagnostic))
    }

  /**
   * Ends the group that `MapHandle.beginCommandGroup()` began.
   *
   * See `mln_map_end_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun endCommandGroup(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_end_command_group(handle, completion, diagnostic))
    }

  /**
   * Moves the camera to fit a GeoJSON geometry in the map's view, at once or with an animation.
   *
   * `geometry` is a GeoJSON geometry object with at least one coordinate, not a feature. The map
   * computes the camera when it runs the command, from the camera and size that earlier commands
   * leave, as `MapHandle.cameraForGeometry()` would then. The function finishes when the map
   * applies the change, before an animation ends.
   *
   * See `mln_map_fit_camera_to_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_ARGUMENT` when the padding in `update` leaves no room for the area: its left
   *   and right add up to at least the map's width, or its top and bottom to at least the map's
   *   height. The camera doesn't move.
   */
  public fun fitCameraToGeometry(
    geometry: String,
    update: CameraFitUpdate,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(
        C.mln_map_fit_camera_to_geometry(
          handle,
          view(geometry),
          writeCameraFitUpdate(update),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Moves the camera to fit `bounds` in the map's view, at once or with an animation.
   *
   * The map computes the camera when it runs the command, from the camera and size that earlier
   * commands leave, as `MapHandle.cameraForLatLngBounds()` would then. The function finishes when
   * the map applies the change, before an animation ends.
   *
   * See `mln_map_fit_camera_to_lat_lng_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_ARGUMENT` when the padding in `update` leaves no room for the area: its left
   *   and right add up to at least the map's width, or its top and bottom to at least the map's
   *   height. The camera doesn't move.
   */
  public fun fitCameraToLatLngBounds(
    bounds: LatLngBounds,
    update: CameraFitUpdate,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(
        C.mln_map_fit_camera_to_lat_lng_bounds(
          handle,
          writeLatLngBounds(bounds),
          writeCameraFitUpdate(update),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Moves the camera to fit `coordinates` in the map's view, at once or with an animation.
   *
   * `coordinates` must not be empty. The map computes the camera when it runs the command, from the
   * camera and size that earlier commands leave, as `MapHandle.cameraForLatLngs()` would then. The
   * function finishes when the map applies the change, before an animation ends.
   *
   * See `mln_map_fit_camera_to_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_ARGUMENT` when the padding in `update` leaves no room for the area: its left
   *   and right add up to at least the map's width, or its top and bottom to at least the map's
   *   height. The camera doesn't move.
   */
  public fun fitCameraToLatLngs(
    coordinates: List<LatLng>,
    update: CameraFitUpdate,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, bindingCallbacks) {
      check(
        C.mln_map_fit_camera_to_lat_lngs(
          handle,
          array(coordinates, 16, w(8, 8, 4)) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          writeCameraFitUpdate(update),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Reads the map's camera.
   *
   * The query reads the camera after the map applies the commands sent before it.
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
   * Reads the state of a feature, as a JSON object.
   *
   * The object is empty when the feature has no state.
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
   * Reads the global state of the map's style as a JSON object.
   *
   * See `mln_map_get_global_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getGlobalState(): Deferred<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
      check(C.mln_map_get_global_state(handle, completion, diagnostic))
    }

  /**
   * Reads the corners of an image source's image, clockwise from the top left.
   *
   * See `mln_map_get_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't an image source.
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
   * Reads the JSON of the style that the map loaded.
   *
   * The JSON doesn't include the changes made to the style since it loaded.
   *
   * See `mln_map_get_loaded_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getLoadedStyleJson(): Deferred<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
      check(C.mln_map_get_loaded_style_json(handle, completion, diagnostic))
    }

  /**
   * Copies the map's state.
   *
   * The copy doesn't wait for pending commands. To read the camera after they apply, use
   * `MapHandle.getCamera()`.
   *
   * See `mln_map_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getSnapshot(): MapSnapshot =
    nativeCall(this, binding) {
      val out = sized(w(408, 408, 380), w(8, 8, 4))
      check(C.mln_map_get_snapshot(handle, out, diagnostic))
      readMapSnapshot(out)
    }

  /**
   * Reads an image of the map's style.
   *
   * Completes with no value when the style has no image named `imageId`.
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
   * Reads a layer of the map's style.
   *
   * Completes with no value when the style has no layer named `layerId`.
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
   * Reads a layer of the map's style as a
   * &#91;layer&#93;(https://maplibre.org/maplibre-style-spec/layers/) object.
   *
   * Completes with no value when the style has no layer named `layerId`.
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
   * Reads a property of a layer in the map's style.
   *
   * Completes with no value when the layer doesn't set the property.
   *
   * See `mln_map_get_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no layer named `layerId`.
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
   * Reads a property of the light of the map's style.
   *
   * Completes with no value when the light doesn't set the property.
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
   * Reads a source of the map's style.
   *
   * Completes with no value when the style has no source named `sourceId`.
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
   * Reads the default transition of the map's style.
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
   * Reads the URL of the map's style.
   *
   * The URL is empty when the style came from `MapHandle.setStyleJson()`.
   *
   * See `mln_map_get_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getStyleUrl(): Deferred<String> =
    nativeSubmit(this, binding, { result -> readViewString(CompletionBridge.value(result)) }) {
      check(C.mln_map_get_style_url(handle, completion, diagnostic))
    }

  /**
   * Makes a custom geometry source ask again for the tiles in `bounds` that the map still needs.
   *
   * See `mln_map_invalidate_custom_geometry_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a custom geometry source.
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
   * Makes a custom geometry source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_geometry_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a custom geometry source.
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
   * Makes a custom vector source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_vector_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a custom vector source.
   */
  public fun invalidateCustomVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_invalidate_custom_vector_source_tile(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Computes the area that the map would show with `camera`.
   *
   * Camera fields that `camera` doesn't set come from the map's camera. The longitudes can fall
   * outside -180 to 180, so a view across the antimeridian gives one continuous range.
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
   * Lists the layers of the map's style, from bottom to top.
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
          w(56, 88, 52),
        ) {
          readStyleLayerInfo(it)
        }
      },
    ) {
      check(C.mln_map_list_style_layers(handle, completion, diagnostic))
    }

  /**
   * Lists the sources of the map's style.
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
          w(120, 152, 108),
        ) {
          readStyleSourceInfo(it)
        }
      },
    ) {
      check(C.mln_map_list_style_sources(handle, completion, diagnostic))
    }

  /**
   * Moves a layer of the map's style below the layer named `beforeLayerId`, or above every layer
   * when `beforeLayerId` is empty.
   *
   * See `mln_map_move_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no layer named `layerId` or `beforeLayerId`.
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
   * Releases the map.
   *
   * Detach the map's render session first.
   *
   * See `mln_map_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public override fun release(): Deferred<Unit> =
    nativeRetire(this, binding) { check(C.mln_map_release(handle, completion, diagnostic)) }

  /**
   * Removes the feature state that `selector` selects.
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
   * Removes an image from the map's style.
   *
   * See `mln_map_remove_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no image named `imageId`.
   */
  public fun removeStyleImage(imageId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_remove_style_image(handle, view(imageId), completion, diagnostic))
    }

  /**
   * Removes a layer from the map's style.
   *
   * See `mln_map_remove_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no layer named `layerId`.
   */
  public fun removeStyleLayer(layerId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_remove_style_layer(handle, view(layerId), completion, diagnostic))
    }

  /**
   * Removes a source from the map's style.
   *
   * See `mln_map_remove_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_STATE` when a layer still uses the source.
   */
  public fun removeStyleSource(sourceId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_remove_style_source(handle, view(sourceId), completion, diagnostic))
    }

  /**
   * Renders a new frame of a continuous map, even when nothing changed.
   *
   * See `mln_map_request_repaint` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun requestRepaint(): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_request_repaint(handle, completion, diagnostic))
    }

  /**
   * Renders a still image of a static or tile map.
   *
   * The map's render session draws the image once every resource that the image needs has loaded.
   *
   * See `mln_map_request_still_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.CANCELLED` when the map is released before the image is done.
   * - `Status.NATIVE_ERROR` when the style or a resource that the image needs fails to load.
   */
  public fun requestStillImage(): Deferred<Unit> =
    nativeUnit(this, binding) {
      check(C.mln_map_request_still_image(handle, completion, diagnostic))
    }

  /**
   * Sets the map's size.
   *
   * A map with a render session takes its size from the session, so resize the session with
   * `RenderSessionHandle.resize()` instead.
   *
   * See `mln_map_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_STATE` when the map has a render session.
   * - `Status.INVALID_STATE` when a render session attached to the map before the resize applied.
   */
  public fun resize(logicalSize: LogicalSize): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_resize(handle, writeLogicalSize(logicalSize), completion, diagnostic))
    }

  /**
   * Limits the camera's center to an area, and its zoom and pitch to ranges.
   *
   * Limits that `options` doesn't set keep their current values. When the camera is outside the new
   * limits, it moves inside them.
   *
   * See `mln_map_set_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setBounds(options: BoundOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_bounds(handle, writeBoundOptions(options), completion, diagnostic))
    }

  /**
   * Sets the data of a tile in a custom geometry source.
   *
   * `data` is a GeoJSON feature collection. The source ignores data for a tile that it hasn't asked
   * for, or has since cancelled.
   *
   * See `mln_map_set_custom_geometry_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a custom geometry source.
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
   * Reports that a tile of a custom geometry source failed to load.
   *
   * See `mln_map_set_custom_geometry_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a custom geometry source.
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
   * Sets the data of a tile in a custom vector source.
   *
   * `data` is the tile, encoded as `encoding`. The source ignores data for a tile that it hasn't
   * asked for, or has since cancelled.
   *
   * See `mln_map_set_custom_vector_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a custom vector source.
   */
  public fun setCustomVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    encoding: StyleVectorTileEncoding,
    data: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_custom_vector_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          encoding.rawValue.toInt(),
          view(data),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Reports that a tile of a custom vector source failed to load.
   *
   * See `mln_map_set_custom_vector_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a custom vector source.
   */
  public fun setCustomVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_custom_vector_source_tile_error(
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
   * Draws debugging overlays, such as tile borders, on the map.
   *
   * `flags` replaces the overlays set before.
   *
   * See `mln_map_set_debug_flags` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setDebugFlags(flags: MapDebugFlag): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_debug_flags(handle, flags.rawValue.toInt(), completion, diagnostic))
    }

  /**
   * Selects the events that the map reports.
   *
   * See `mln_map_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setEventMask(mask: RuntimeEventFlag): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_event_mask(handle, mask.rawValue.toLong(), completion, diagnostic))
    }

  /**
   * Merges `state` into the state of a feature.
   *
   * `state` is a JSON object. Its keys replace the same keys in the feature's state, and the other
   * keys stay.
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
   * Places the camera at a position and orientation in space.
   *
   * The map adjusts them to a camera that it can show. It stops running camera animations first,
   * and their end handlers report `CameraTransitionOutcome.CANCELLED`.
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
   * Replaces the data of a GeoJSON source.
   *
   * `data` must have been prepared with the same options as the source. It can be released once
   * this function returns.
   *
   * See `mln_map_set_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a GeoJSON source, or `data`
   *   was prepared with other options.
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
   * Sets whether a GeoJSON source builds its tiles synchronously.
   *
   * This works like the synchronous tiling option of `GeoJsonSourceOptions`, but `enabled` can't
   * turn off synchronous tiling that the source's options turned on.
   *
   * See `mln_map_set_geo_json_source_synchronous_tiling` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a GeoJSON source.
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
   * Makes a GeoJSON source load its data from `url`.
   *
   * See `mln_map_set_geo_json_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't a GeoJSON source.
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
   * Sets a value that the style's `global-state`
   * &#91;expressions&#93;(https://maplibre.org/maplibre-style-spec/expressions/) read.
   *
   * See `mln_map_set_global_state_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_STATE` until the map has parsed its style, which
   *   `RuntimeEventType.MAP_STYLE_LOADED` reports, and again while it loads a new style URL.
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
   * Moves the image of an image source.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_set_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't an image source.
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
          array(coordinates, 16, w(8, 8, 4)) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Replaces the image of an image source.
   *
   * See `mln_map_set_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't an image source.
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
   * Makes an image source load its image from `url`.
   *
   * See `mln_map_set_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
   * - `Status.INVALID_ARGUMENT` when the source named `sourceId` isn't an image source.
   */
  public fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_image_source_url(handle, view(sourceId), view(url), completion, diagnostic),
      )
    }

  /**
   * Sets whether the map draws in perspective or axonometric projection, and how much it skews the
   * view.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_projection_mode` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setProjectionMode(options: ProjectionModeOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(
        C.mln_map_set_projection_mode(
          handle,
          writeProjectionModeOptions(options),
          completion,
          diagnostic,
        ),
      )
    }

  /**
   * Shows or hides an overlay of rendering statistics on the map.
   *
   * See `mln_map_set_rendering_stats_view_enabled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_rendering_stats_view_enabled(handle, enabled, completion, diagnostic))
    }

  /**
   * Adds an image to the map's style, or replaces the image named `imageId`.
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
   * Loads the map's style from `json`.
   *
   * The style's sources, sprites, and glyphs load in the background.
   *
   * See `mln_map_set_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.NATIVE_ERROR` when `json` isn't a valid style.
   */
  public fun setStyleJson(json: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_style_json(handle, view(json), completion, diagnostic))
    }

  /**
   * Sets a property of a layer in the map's style.
   *
   * `propertyName` is one of the layer's layout or paint
   * &#91;properties&#93;(https://maplibre.org/maplibre-style-spec/layers/), or `filter`, `minzoom`,
   * `maxzoom`, `source`, or `source-layer`. A null `value` resets any of these but `source` and
   * `source-layer` to its default.
   *
   * See `mln_map_set_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no layer named `layerId`.
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
   * Replaces the &#91;light&#93;(https://maplibre.org/maplibre-style-spec/light/) of the map's
   * style.
   *
   * See `mln_map_set_style_light_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLightJson(lightJson: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_style_light_json(handle, view(lightJson), completion, diagnostic))
    }

  /**
   * Sets a property of the light of the map's style.
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
   * Sets whether a source is volatile.
   *
   * The map doesn't store the tiles of a volatile source in the persistent cache.
   *
   * See `mln_map_set_style_source_volatile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.NOT_FOUND` when the style has no source named `sourceId`.
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
   * Sets the default transition of the map's style.
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
   * Loads the map's style from `url`.
   *
   * The style loads in the background, and `RuntimeEventType.MAP_STYLE_LOADED` or
   * `RuntimeEventType.MAP_LOADING_FAILED` reports how the load ends. Sources, layers, and images
   * added before the style loads are lost when it loads.
   *
   * See `mln_map_set_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setStyleUrl(url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding) {
      check(C.mln_map_set_style_url(handle, view(url), completion, diagnostic))
    }

  /**
   * Sets how the map chooses which tiles to load, and at what detail.
   *
   * Settings that `options` doesn't set keep their current values.
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
   * Sets how the map orients and constrains its view.
   *
   * Settings that `options` doesn't set keep their current values.
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
   * Moves the camera, at once or with an animation.
   *
   * Camera fields that `update` doesn't set keep their values, and the update stops any running
   * animation of the fields it sets. The function finishes when the map applies the update, before
   * an animation ends. To learn when an animation ends, set an `CameraTransitionHandler` in its
   * animation options.
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

  /** [MapHandle.addCustomVectorSource] without its completion. */
  public fun addCustomVectorSource(sourceId: String, options: CustomVectorSourceOptions): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(
        C.mln_map_add_custom_vector_source(
          handle,
          view(sourceId),
          writeCustomVectorSourceOptions(options),
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
          array(coordinates, 16, w(8, 8, 4)) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          writePremultipliedRgba8Image(image),
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

  /** [MapHandle.cancelCameraTransitions] without its completion. */
  public fun cancelCameraTransitions(): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_cancel_camera_transitions(handle, completion, diagnostic))
    }

  /** [MapHandle.cancelCameraTransitionsWithId] without its completion. */
  public fun cancelCameraTransitionsWithId(transitionId: ULong): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_cancel_camera_transitions_with_id(
          handle,
          transitionId.toLong(),
          completion,
          diagnostic,
        ),
      )
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

  /** [MapHandle.fitCameraToGeometry] without its completion. */
  public fun fitCameraToGeometry(geometry: String, update: CameraFitUpdate): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(
        C.mln_map_fit_camera_to_geometry(
          handle,
          view(geometry),
          writeCameraFitUpdate(update),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.fitCameraToLatLngBounds] without its completion. */
  public fun fitCameraToLatLngBounds(bounds: LatLngBounds, update: CameraFitUpdate): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(
        C.mln_map_fit_camera_to_lat_lng_bounds(
          handle,
          writeLatLngBounds(bounds),
          writeCameraFitUpdate(update),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.fitCameraToLatLngs] without its completion. */
  public fun fitCameraToLatLngs(coordinates: List<LatLng>, update: CameraFitUpdate): Unit =
    nativeDiscard(owner, binding, bindingCallbacks) {
      check(
        C.mln_map_fit_camera_to_lat_lngs(
          handle,
          array(coordinates, 16, w(8, 8, 4)) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          writeCameraFitUpdate(update),
          completion,
          diagnostic,
        ),
      )
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

  /** [MapHandle.invalidateCustomVectorSourceTile] without its completion. */
  public fun invalidateCustomVectorSourceTile(sourceId: String, tileId: CanonicalTileId): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_invalidate_custom_vector_source_tile(
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
  public fun resize(logicalSize: LogicalSize): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_resize(handle, writeLogicalSize(logicalSize), completion, diagnostic))
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

  /** [MapHandle.setCustomVectorSourceTileData] without its completion. */
  public fun setCustomVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    encoding: StyleVectorTileEncoding,
    data: ByteArray,
  ): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_custom_vector_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          encoding.rawValue.toInt(),
          view(data),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setCustomVectorSourceTileError] without its completion. */
  public fun setCustomVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_custom_vector_source_tile_error(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(message),
          completion,
          diagnostic,
        ),
      )
    }

  /** [MapHandle.setDebugFlags] without its completion. */
  public fun setDebugFlags(flags: MapDebugFlag): Unit =
    nativeDiscard(owner, binding) {
      check(C.mln_map_set_debug_flags(handle, flags.rawValue.toInt(), completion, diagnostic))
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
          array(coordinates, 16, w(8, 8, 4)) { at, item -> putLatLng(at, item) },
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
  public fun setProjectionMode(options: ProjectionModeOptions): Unit =
    nativeDiscard(owner, binding) {
      check(
        C.mln_map_set_projection_mode(
          handle,
          writeProjectionModeOptions(options),
          completion,
          diagnostic,
        ),
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
