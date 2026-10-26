// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Reports the C ABI contract version. The value is 0 while the ABI is unstable, and will increment
 * on each SemVer major release.
 *
 * See `mln_c_version` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public fun cVersion(): UInt = nativeUncheckedCall {
  val raw = C.mln_c_version()
  raw.toUInt()
}

/**
 * Converts spherical Mercator projected meters to a geographic coordinate.
 *
 * See `mln_lat_lng_for_projected_meters` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
public fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng =
  nativeCall(null, null) {
    val out = allocate(16, 8)
    check(C.mln_lat_lng_for_projected_meters(writeProjectedMeters(meters), out, diagnostic))
    readLatLng(out)
  }

/**
 * Clears the process-global log callback.
 *
 * See `mln_log_clear_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun clearLogCallback(): Unit =
  nativeCall(null, null) { check(C.mln_log_clear_callback(diagnostic)) }

/**
 * Controls which log severities MapLibre Native may dispatch asynchronously.
 *
 * See `mln_log_set_async_severity_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
public fun setLogAsyncSeverityMask(mask: LogSeverityFlag): Unit =
  nativeCall(null, null) {
    check(C.mln_log_set_async_severity_mask(mask.rawValue.toInt(), diagnostic))
  }

/**
 * Computes the physical device-pixel size of a logical extent.
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
 * Reads MapLibre Native's process-global network status.
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
 * Sets MapLibre Native's process-global network status.
 *
 * See `mln_network_set_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
public fun setNetworkStatus(status: NetworkStatus): Unit =
  nativeCall(null, null) { check(C.mln_network_set_status(status.rawValue.toInt(), diagnostic)) }

/**
 * Returns OpenGL context providers supported by this build.
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
 * Returns the process-wide plugin registration entry point; never null.
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
 * Converts a geographic coordinate to spherical Mercator projected meters.
 *
 * See `mln_projected_meters_for_lat_lng` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
public fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters =
  nativeCall(null, null) {
    val out = allocate(16, 8)
    check(C.mln_projected_meters_for_lat_lng(writeLatLng(coordinate), out, diagnostic))
    readProjectedMeters(out)
  }

/**
 * Reports the render backends available in this native library build.
 *
 * See `mln_supported_render_backend_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public fun supportedRenderBackendMask(): RenderBackendFlag =
  nativeCall(null, null) {
    val raw = C.mln_supported_render_backend_mask()
    RenderBackendFlag(raw.toUInt())
  }
