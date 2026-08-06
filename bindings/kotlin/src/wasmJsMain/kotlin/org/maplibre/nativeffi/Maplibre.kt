package org.maplibre.nativeffi

import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ProjectedMeters
import org.maplibre.nativeffi.internal.callback.LogCallbackState
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.BrowserModule
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLng
import org.maplibre.nativeffi.internal.wasm.generated.MlnProjectedMeters
import org.maplibre.nativeffi.internal.wasm.generated.mln_c_version
import org.maplibre.nativeffi.internal.wasm.generated.mln_lat_lng_for_projected_meters
import org.maplibre.nativeffi.internal.wasm.generated.mln_log_set_async_severity_mask
import org.maplibre.nativeffi.internal.wasm.generated.mln_network_status_get
import org.maplibre.nativeffi.internal.wasm.generated.mln_network_status_set
import org.maplibre.nativeffi.internal.wasm.generated.mln_opengl_supported_context_provider_mask
import org.maplibre.nativeffi.internal.wasm.generated.mln_projected_meters_for_lat_lng
import org.maplibre.nativeffi.internal.wasm.generated.mln_supported_render_backend_mask
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogSeverity
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.runtime.NetworkStatus

/** Process-global entry points for the Kotlin/Wasm browser binding. */
public actual object Maplibre {
  /** C ABI contract version expected by this browser binding. */
  public actual const val EXPECTED_C_ABI_VERSION: Long = 0L

  /** The native default async log severity mask: error and warning. */
  private const val DEFAULT_LOG_SEVERITY_MASK: Int = (1 shl 1) or (1 shl 2)

  /**
   * Names the Emscripten module that this binding runs inside of, and checks its C ABI version.
   *
   * The module is instantiated before Kotlin exists, because it is what imported this binding, so
   * there is nothing here to load.
   */
  public actual fun loadNativeLibrary() {
    BrowserModule.attach()
    checkCompatibleCAbi()
  }

  internal fun checkCompatibleCAbi(actualVersion: Long = cVersion()) {
    if (actualVersion == EXPECTED_C_ABI_VERSION) {
      return
    }

    throw AbiVersionMismatchException(actualVersion, EXPECTED_C_ABI_VERSION)
  }

  // Every entry point below names the module before it reaches native, because a host application's
  // own main() runs while this distribution is being imported, which is before the module calls
  // mlnKotlinMain().

  /** Returns the native C ABI contract version. */
  public actual fun cVersion(): Long {
    BrowserModule.attach()
    return mln_c_version().toUInt().toLong()
  }

  /** Returns the render backends compiled into the loaded browser module. */
  public actual fun supportedRenderBackends(): Set<RenderBackend> {
    BrowserModule.attach()
    return RenderBackend.fromMask(mln_supported_render_backend_mask())
  }

  /** Returns the OpenGL context providers compiled into the loaded browser module. */
  public actual fun supportedOpenGLContextProviders(): Set<OpenGLContextProvider> {
    BrowserModule.attach()
    return OpenGLContextProvider.fromMask(mln_opengl_supported_context_provider_mask())
  }

  /** Reads Maplibre Native's process-global network status. */
  public actual val networkStatus: NetworkStatus
    get() {
      BrowserModule.attach()
      return Heap.withScratch(4) { out ->
        Status.check(mln_network_status_get(out.address))
        NetworkStatus.fromNative(Heap.loadInt(out))
      }
    }

  /** Sets Maplibre Native's process-global network status. */
  public actual fun setNetworkStatus(status: NetworkStatus) {
    Status.requireArgument(status.isKnown) {
      "Unknown network status cannot be used as input: ${status.nativeValue}"
    }
    BrowserModule.attach()
    Status.check(mln_network_status_set(status.nativeValue))
  }

  /** Installs or replaces the process-global native log callback. */
  public actual fun setLogCallback(callback: LogCallback, consume: Boolean) {
    LogCallbackState.set(callback, consume)
  }

  /** Clears the process-global native log callback. */
  public actual fun clearLogCallback() {
    LogCallbackState.clear()
  }

  /** Configures severities that native logging may dispatch asynchronously. */
  public actual fun setAsyncLogSeverities(severities: Set<LogSeverity>) {
    val mask = severities.fold(0) { accumulated, severity -> accumulated or severity.nativeMask }
    BrowserModule.attach()
    Status.check(mln_log_set_async_severity_mask(mask))
  }

  /** Restores the native default async log severity mask. */
  public actual fun restoreDefaultAsyncLogSeverities() {
    BrowserModule.attach()
    Status.check(mln_log_set_async_severity_mask(DEFAULT_LOG_SEVERITY_MASK))
  }

  /** Converts a geographic coordinate to spherical Mercator projected meters. */
  public actual fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters {
    BrowserModule.attach()
    return Heap.withScratch(MlnLatLng.SIZEOF + MlnProjectedMeters.SIZEOF) { scratch ->
      val out = scratch + MlnLatLng.SIZEOF
      MlnLatLng.setLatitude(scratch, coordinate.latitude)
      MlnLatLng.setLongitude(scratch, coordinate.longitude)
      Status.check(mln_projected_meters_for_lat_lng(scratch.address, out.address))
      ProjectedMeters(MlnProjectedMeters.northing(out), MlnProjectedMeters.easting(out))
    }
  }

  /** Converts spherical Mercator projected meters to a geographic coordinate. */
  public actual fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng {
    BrowserModule.attach()
    return Heap.withScratch(MlnProjectedMeters.SIZEOF + MlnLatLng.SIZEOF) { scratch ->
      val out = scratch + MlnProjectedMeters.SIZEOF
      MlnProjectedMeters.setNorthing(scratch, meters.northing)
      MlnProjectedMeters.setEasting(scratch, meters.easting)
      Status.check(mln_lat_lng_for_projected_meters(scratch.address, out.address))
      LatLng(MlnLatLng.latitude(out), MlnLatLng.longitude(out))
    }
  }
}
