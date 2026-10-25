// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public object GeneratedApi {
  /**
   * Reports the C ABI contract version. The value is 0 while the ABI is unstable, and will
   * increment on each SemVer major release.
   *
   * See `mln_c_version` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
   */
  public fun cVersion(): UInt = nativeUncheckedCall {
    val raw = C.mln_c_version()
    raw.toUInt()
  }

  /**
   * Prepares GeoJSON source data for installation on a map.
   *
   * See `mln_geojson_source_data_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun geojsonSourceDataCreate(
    data: String,
    options: GeojsonSourceOptions? = null,
  ): GeojsonSourceDataHandle =
    nativeCall(null, null) {
      val out = allocate(8, 8)
      check(
        C.mln_geojson_source_data_create(
          view(data),
          options?.let { writeGeojsonSourceOptions(it) } ?: 0L,
          out,
          diagnostic,
        ),
      )
      adopt(out, GeneratedOwnerDisposal::geojsonSourceData) { GeojsonSourceDataHandle(it) }
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
  public fun logClearCallback(): Unit =
    nativeCall(null, null) { check(C.mln_log_clear_callback(diagnostic)) }

  /**
   * Controls which log severities MapLibre Native may dispatch asynchronously.
   *
   * See `mln_log_set_async_severity_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
   */
  public fun logSetAsyncSeverityMask(mask: LogSeverityFlag): Unit =
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
   * Creates a standalone projection of a view, such as the view of a rendered frame or of a map
   * snapshot.
   *
   * See `mln_map_projection_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun mapProjectionCreate(view: MapView): MapProjectionHandle =
    nativeCall(null, null) {
      val out = allocate(8, 8)
      check(C.mln_map_projection_create(writeMapView(view), out, diagnostic))
      adopt(out, GeneratedOwnerDisposal::mapProjection) { MapProjectionHandle(it) }
    }

  /**
   * Reads MapLibre Native's process-global network status.
   *
   * See `mln_network_get_status` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun networkGetStatus(): NetworkStatus =
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
  public fun networkSetStatus(status: NetworkStatus): Unit =
    nativeCall(null, null) { check(C.mln_network_set_status(status.rawValue.toInt(), diagnostic)) }

  /**
   * Returns OpenGL context providers supported by this build.
   *
   * See `mln_opengl_supported_context_provider_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
   */
  public fun openglSupportedContextProviderMask(): OpenglContextProviderFlag =
    nativeCall(null, null) {
      val raw = C.mln_opengl_supported_context_provider_mask()
      OpenglContextProviderFlag(raw.toUInt())
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
   * Creates a runtime.
   *
   * Suspends until the call's thread starts. On the JVM, Android, and Kotlin/Native the call waits
   * on the calling thread, which it blocks until then. On Kotlin/Wasm it runs on a thread of its
   * own, so the page can call it. Cancellation does not interrupt the wait: a caller cancelled
   * meanwhile receives the cancellation, and the binding disposes the result.
   *
   * See `mln_runtime_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public suspend fun runtimeCreate(options: RuntimeOptions): RuntimeHandle =
    nativeThreadStart({ it.dispose() }) {
      val out = allocate(8, 8)
      threadStart { diagnostic ->
        C.mln_runtime_create(writeRuntimeOptions(options), out, diagnostic)
      }
      adopt(out, GeneratedOwnerDisposal::runtime) { RuntimeHandle(it) }
    }

  /**
   * Parses a style expression.
   *
   * See `mln_style_expression_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  public fun styleExpressionCreate(
    expression: String,
    options: StyleExpressionOptions? = null,
  ): StyleExpressionHandle =
    nativeCall(null, null) {
      val out = allocate(8, 8)
      check(
        C.mln_style_expression_create(
          view(expression),
          options?.let { writeStyleExpressionOptions(it) } ?: 0L,
          out,
          diagnostic,
        ),
      )
      adopt(out, GeneratedOwnerDisposal::styleExpression) { StyleExpressionHandle(it) }
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
}
