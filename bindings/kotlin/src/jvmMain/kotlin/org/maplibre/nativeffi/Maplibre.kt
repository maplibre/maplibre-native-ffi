package org.maplibre.nativeffi

import java.nio.file.Path
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ProjectedMeters
import org.maplibre.nativeffi.internal.callback.LogCallbackState
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogSeverity
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.runtime.NetworkStatus

/** Process-global entry points for the Kotlin/JVM FFM bridge. */
public actual object Maplibre {
  /** C ABI contract version expected by this Kotlin/JVM binding. */
  public actual const val EXPECTED_C_ABI_VERSION: Long = NativeAccess.EXPECTED_C_ABI_VERSION

  /** Loads the native library using the binding's standard lookup order. */
  public actual fun loadNativeLibrary() {
    NativeAccess.ensureLoaded()
  }

  /** Loads the native library from an exact file path. */
  public fun loadNativeLibrary(libraryPath: Path) {
    NativeAccess.load(libraryPath)
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
    NativeAccess.setAsyncLogSeverityMask(mask)
  }

  /** Restores the native default async log severity mask. */
  public actual fun restoreDefaultAsyncLogSeverities() {
    NativeAccess.ensureLoaded()
    NativeAccess.setAsyncLogSeverityMask(NativeAccess.DEFAULT_LOG_SEVERITY_MASK)
  }

  /** Converts a geographic coordinate to spherical Mercator projected meters. */
  public actual fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters {
    NativeAccess.ensureLoaded()
    return NativeAccess.projectedMetersForLatLng(coordinate)
  }

  /** Converts spherical Mercator projected meters to a geographic coordinate. */
  public actual fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng {
    NativeAccess.ensureLoaded()
    return NativeAccess.latLngForProjectedMeters(meters)
  }
}
