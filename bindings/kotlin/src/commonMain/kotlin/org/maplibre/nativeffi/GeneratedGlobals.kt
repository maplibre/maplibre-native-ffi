// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
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
