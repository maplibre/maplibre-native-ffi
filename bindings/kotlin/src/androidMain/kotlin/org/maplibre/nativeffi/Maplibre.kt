package org.maplibre.nativeffi

import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ProjectedMeters
import org.maplibre.nativeffi.internal.callback.LogCallbackState
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogSeverity
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.runtime.NetworkStatus

/** Process-global entry points for the Android JNI bridge. */
public actual object Maplibre {
  /** C ABI contract version expected by this Android binding. */
  public actual const val EXPECTED_C_ABI_VERSION: Long = 0L

  /** Loads the Android JNI bridge library. */
  public actual fun loadNativeLibrary() {
    NativeAccess.ensureLoaded()
  }

  /** Returns the native C ABI contract version. */
  public actual fun cVersion(): Long {
    NativeAccess.ensureLoaded()
    return NativeAccess.cVersion()
  }

  /** Returns the render backends compiled into the loaded native library. */
  public actual fun supportedRenderBackends(): Set<RenderBackend> {
    NativeAccess.ensureLoaded()
    return RenderBackend.fromMask(NativeAccess.supportedRenderBackendMask())
  }

  /** Returns the OpenGL context providers compiled into the loaded native library. */
  public actual fun supportedOpenGLContextProviders(): Set<OpenGLContextProvider> {
    NativeAccess.ensureLoaded()
    return OpenGLContextProvider.fromMask(NativeAccess.supportedOpenGLContextProviderMask())
  }

  /** Reads Maplibre Native's process-global network status. */
  public actual val networkStatus: NetworkStatus
    get() {
      NativeAccess.ensureLoaded()
      return NetworkStatus.fromNative(NativeAccess.networkStatus())
    }

  /** Sets Maplibre Native's process-global network status. */
  public actual fun setNetworkStatus(status: NetworkStatus) {
    NativeAccess.ensureLoaded()
    Status.requireArgument(status.isKnown) {
      "Unknown network status cannot be used as input: ${status.nativeValue}"
    }
    NativeAccess.setNetworkStatus(status.nativeValue)
  }

  /** Installs or replaces the process-global native log callback. */
  public actual fun setLogCallback(callback: LogCallback) {
    LogCallbackState.set(callback)
  }

  /** Clears the process-global native log callback. */
  public actual fun clearLogCallback() {
    LogCallbackState.clear()
  }

  /** Configures severities that native logging may dispatch asynchronously. */
  public actual fun setAsyncLogSeverities(severities: Set<LogSeverity>) {
    NativeAccess.ensureLoaded()
    val mask = severities.fold(0) { acc, severity -> acc or severity.nativeMask }
    Status.check(MaplibreNativeC.mln_log_set_async_severity_mask(mask))
  }

  /** Restores the native default async log severity mask. */
  public actual fun restoreDefaultAsyncLogSeverities() {
    NativeAccess.ensureLoaded()
    Status.check(
      MaplibreNativeC.mln_log_set_async_severity_mask(MaplibreNativeC.MLN_LOG_SEVERITY_MASK_DEFAULT)
    )
  }

  /** Converts a geographic coordinate to spherical Mercator projected meters. */
  public actual fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters {
    NativeAccess.ensureLoaded()
    val nativeCoordinate =
      MaplibreNativeC.mln_lat_lng().latitude(coordinate.latitude).longitude(coordinate.longitude)
    val outMeters = MaplibreNativeC.mln_projected_meters()
    Status.check(MaplibreNativeC.mln_projected_meters_for_lat_lng(nativeCoordinate, outMeters))
    return ProjectedMeters(outMeters.northing(), outMeters.easting())
  }

  /** Converts spherical Mercator projected meters to a geographic coordinate. */
  public actual fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng {
    NativeAccess.ensureLoaded()
    val nativeMeters =
      MaplibreNativeC.mln_projected_meters().northing(meters.northing).easting(meters.easting)
    val outCoordinate = MaplibreNativeC.mln_lat_lng()
    Status.check(MaplibreNativeC.mln_lat_lng_for_projected_meters(nativeMeters, outCoordinate))
    return LatLng(outCoordinate.latitude(), outCoordinate.longitude())
  }
}

internal object NativeAccess {
  private val lock = Any()

  @Volatile private var loaded = false

  fun ensureLoaded() {
    if (loaded) {
      return
    }

    synchronized(lock) {
      if (loaded) {
        return
      }

      Maplibre.checkCompatibleCAbi(cVersion())
      loaded = true
    }
  }

  fun cVersion(): Long = Integer.toUnsignedLong(MaplibreNativeC.mln_c_version())

  fun supportedRenderBackendMask(): Int = MaplibreNativeC.mln_supported_render_backend_mask()

  fun supportedOpenGLContextProviderMask(): Int =
    MaplibreNativeC.mln_opengl_supported_context_provider_mask()

  fun networkStatus(): Int {
    val outStatus = intArrayOf(0)
    Status.check(MaplibreNativeC.mln_network_status_get(outStatus))
    return outStatus[0]
  }

  fun setNetworkStatus(status: Int) {
    Status.check(MaplibreNativeC.mln_network_status_set(status))
  }
}

private fun Maplibre.checkCompatibleCAbi(actualVersion: Long) {
  if (actualVersion != EXPECTED_C_ABI_VERSION) {
    throw org.maplibre.nativeffi.error.AbiVersionMismatchException(
      actualVersion,
      EXPECTED_C_ABI_VERSION,
    )
  }
}
