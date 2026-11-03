// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Returns the API epoch of the library.
 *
 * Headers and a library with different epochs aren't compatible.
 *
 * See `mln_c_version` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public fun cVersion(): UInt = nativeUncheckedCall {
  val raw = C.mln_c_version()
  raw.toUInt()
}

/**
 * Computes the camera that shows exactly one tile on a square map.
 *
 * Jump a map in `MapMode.TILE` to the camera to render `tile`, where `logicalSize` is the map's
 * size. The camera centers the tile and zooms until the tile fills the map: to zoom z for a
 * 512-pixel map, or z - 1 for a 256-pixel one, plus 0.0001 to keep the neighboring tiles out. It
 * looks straight down with north up and no padding, so the map's earlier camera doesn't change the
 * view. The map keeps the camera within its zoom limits, so for a deep tile, raise the maximum zoom
 * past the camera's with `MapHandle.setBounds()`.
 *
 * See `mln_camera_for_tile` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Errors:
 *
 * - `Status.INVALID_ARGUMENT` when `logicalSize` isn't square, or is wider than 512 pixels, the
 *   width of a vector tile at its zoom. For denser tiles, raise the map's scale factor instead.
 * - `Status.INVALID_ARGUMENT` when `tile.x` or `tile.y` isn't less than 2 to the power `tile.z`, or
 *   when the camera's zoom would fall outside 0 to 25.5, as for tile 0/0/0 on a 256-pixel map.
 */
public fun cameraForTile(tile: CanonicalTileId, logicalSize: LogicalSize): CameraOptions =
  nativeCall(null, null) {
    val out = sized(120, w(8, 8, 4))
    check(
      C.mln_camera_for_tile(
        writeCanonicalTileId(tile),
        writeLogicalSize(logicalSize),
        out,
        diagnostic,
      ),
    )
    readCameraOptions(out)
  }

/**
 * Converts meters in the Web Mercator projection to a coordinate.
 *
 * See `mln_lat_lng_for_projected_meters` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
public fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng =
  nativeCall(null, null) {
    val out = allocate(16, w(8, 8, 4))
    check(C.mln_lat_lng_for_projected_meters(writeProjectedMeters(meters), out, diagnostic))
    readLatLng(out)
  }

/**
 * Sends log messages to the platform's log again, instead of a callback.
 *
 * See `mln_log_clear_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun clearLogCallback(): Unit =
  nativeCall(null, null) { check(C.mln_log_clear_callback(diagnostic)) }

/**
 * Selects the severities that the library logs on a background thread.
 *
 * The library logs other severities on the thread that logged them. `mask` applies to every runtime
 * in the process.
 *
 * See `mln_log_set_async_severity_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun setLogAsyncSeverityMask(mask: LogSeverityFlag): Unit =
  nativeCall(null, null) {
    check(C.mln_log_set_async_severity_mask(mask.rawValue.toInt(), diagnostic))
  }

/**
 * Sends the log messages of every runtime in the process to a callback.
 *
 * The callback replaces the one set before.
 *
 * Runs on the thread that logged the message, or on a background thread for the severities that
 * `setLogAsyncSeverityMask()` selects. The callback can't call the library.
 *
 * See `mln_log_set_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun setLogCallback(handler: LogHandler): Unit =
  nativeCall(null, null) {
    check(C.mln_log_set_callback(writeLogHandler(handler), diagnostic))
    accept(CallbackOwner.global)
  }

/**
 * Computes the size in pixels of a target with a logical size and scale factor.
 *
 * See `mln_logical_extent_physical_size` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
public fun logicalExtentPhysicalSize(extent: LogicalExtent): LogicalExtentPhysicalSizeResult =
  nativeCall(null, null) {
    val out0 = allocate(4, 4)
    val out1 = allocate(4, 4)
    check(C.mln_logical_extent_physical_size(writeLogicalExtent(extent), out0, out1, diagnostic))
    LogicalExtentPhysicalSizeResult(width = readU32(out0), height = readU32(out1))
  }

/**
 * Reads whether the library may use the network.
 *
 * See `mln_network_get_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun getNetworkStatus(): NetworkStatus =
  nativeCall(null, null) {
    val out = allocate(4, 4)
    check(C.mln_network_get_status(out, diagnostic))
    NetworkStatus(readU32(out))
  }

/**
 * Tells the library whether it may use the network.
 *
 * While the status is `NetworkStatus.OFFLINE`, the library starts no network requests. The requests
 * that wait start when the status returns to `NetworkStatus.ONLINE`. The status starts as online
 * and applies to every runtime in the process.
 *
 * See `mln_network_set_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun setNetworkStatus(status: NetworkStatus): Unit =
  nativeCall(null, null) { check(C.mln_network_set_status(status.rawValue.toInt(), diagnostic)) }

/**
 * Reports the OpenGL context platforms that this build of the library has.
 *
 * See `mln_open_gl_supported_context_provider_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
public fun openGlSupportedContextProviderMask(): OpenGlContextProviderFlag =
  nativeCall(null, null) {
    val raw = C.mln_open_gl_supported_context_provider_mask()
    OpenGlContextProviderFlag(raw.toUInt())
  }

/**
 * Returns the function that registers a plugin's style layer types.
 *
 * Pass the function to the plugin's registration entry point before loading a style that uses the
 * plugin's layer types. Plugin callbacks run on library threads and can keep running while the
 * process exits, so they must not use state that the process destroys at exit.
 *
 * See `mln_plugin_get_register_function_v1` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
 */
public fun getPluginRegisterFunctionV1(): NativePointer =
  nativeCall(null, null) {
    val raw = C.mln_plugin_get_register_function_v1()
    NativePointer.ofAddress(raw)
  }

/**
 * Converts a coordinate to meters in the Web Mercator projection.
 *
 * See `mln_projected_meters_for_lat_lng` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
public fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters =
  nativeCall(null, null) {
    val out = allocate(16, w(8, 8, 4))
    check(C.mln_projected_meters_for_lat_lng(writeLatLng(coordinate), out, diagnostic))
    readProjectedMeters(out)
  }

/**
 * Returns the render backends that this build of the library draws with.
 *
 * See `mln_supported_render_backend_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public fun supportedRenderBackendMask(): RenderBackendFlag =
  nativeCall(null, null) {
    val raw = C.mln_supported_render_backend_mask()
    RenderBackendFlag(raw.toUInt())
  }
