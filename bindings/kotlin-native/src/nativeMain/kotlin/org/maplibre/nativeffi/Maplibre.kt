package org.maplibre.nativeffi

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UIntVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ProjectedMeters
import org.maplibre.nativeffi.internal.c.MLN_LOG_SEVERITY_MASK_DEFAULT
import org.maplibre.nativeffi.internal.c.mln_c_version
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_lat_lng_for_projected_meters
import org.maplibre.nativeffi.internal.c.mln_log_set_async_severity_mask
import org.maplibre.nativeffi.internal.c.mln_network_status_get
import org.maplibre.nativeffi.internal.c.mln_network_status_set
import org.maplibre.nativeffi.internal.c.mln_opengl_supported_context_provider_mask
import org.maplibre.nativeffi.internal.c.mln_projected_meters
import org.maplibre.nativeffi.internal.c.mln_projected_meters_for_lat_lng
import org.maplibre.nativeffi.internal.c.mln_supported_render_backend_mask
import org.maplibre.nativeffi.internal.callback.LogCallbackState
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogSeverity
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.runtime.NetworkStatus

/** Process-global entry points for the Kotlin/Native binding. */
@OptIn(ExperimentalForeignApi::class)
public object Maplibre {
  /** Native libraries are linked by the host binary for Kotlin/Native. */
  public fun loadNativeLibrary() {
    // Direct cinterop calls bind against the native library at link/load time.
  }

  /** Returns the native C ABI contract version. */
  public fun cVersion(): Long = mln_c_version().toLong()

  /** Returns the render backends compiled into the loaded native library. */
  public fun supportedRenderBackends(): Set<RenderBackend> =
    RenderBackend.fromMask(mln_supported_render_backend_mask())

  /** Returns the OpenGL context providers compiled into the loaded native library. */
  public fun supportedOpenGLContextProviders(): Set<OpenGLContextProvider> =
    OpenGLContextProvider.fromMask(mln_opengl_supported_context_provider_mask())

  /** Reads or sets Maplibre Native's process-global network status. */
  public var networkStatus: NetworkStatus
    get() = memScoped {
      val outStatus = alloc<UIntVar>()
      Status.check(mln_network_status_get(outStatus.ptr))
      NetworkStatus.fromNative(outStatus.value)
    }
    set(status) {
      Status.check(mln_network_status_set(status.nativeValue.toUInt()))
    }

  /** Installs or replaces the process-global native log callback. */
  public fun setLogCallback(callback: LogCallback) {
    LogCallbackState.set(callback)
  }

  /** Clears the process-global native log callback. */
  public fun clearLogCallback() {
    LogCallbackState.clear()
  }

  /** Configures severities that native logging may dispatch asynchronously. */
  public fun setAsyncLogSeverities(severities: Set<LogSeverity>) {
    val mask = severities.fold(0) { acc, severity -> acc or severity.nativeMask }
    Status.check(mln_log_set_async_severity_mask(mask.toUInt()))
  }

  /** Restores the native default async log severity mask. */
  public fun restoreDefaultAsyncLogSeverities() {
    Status.check(mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_MASK_DEFAULT))
  }

  /** Converts a geographic coordinate to spherical Mercator projected meters. */
  public fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters = memScoped {
    val outMeters = alloc<mln_projected_meters>()
    Status.check(mln_projected_meters_for_lat_lng(CoreStructs.latLng(coordinate), outMeters.ptr))
    CoreStructs.projectedMeters(outMeters)
  }

  /** Converts spherical Mercator projected meters to a geographic coordinate. */
  public fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng = memScoped {
    val outCoordinate = alloc<mln_lat_lng>()
    Status.check(
      mln_lat_lng_for_projected_meters(CoreStructs.projectedMeters(meters), outCoordinate.ptr)
    )
    CoreStructs.latLng(outCoordinate)
  }
}
