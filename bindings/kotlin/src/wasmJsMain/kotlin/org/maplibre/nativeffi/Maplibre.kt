package org.maplibre.nativeffi

import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ProjectedMeters
import org.maplibre.nativeffi.internal.callback.LogCallbackRegistry
import org.maplibre.nativeffi.internal.callback.LogQueueBridge
import org.maplibre.nativeffi.internal.callback.LogQueueDrain
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.BrowserModule
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.NativeCall
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLng
import org.maplibre.nativeffi.internal.wasm.generated.MlnProjectedMeters
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogSeverity
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.runtime.NetworkStatus

/**
 * Process-global entry points for the browser binding.
 *
 * Every call here is process-global rather than owned by a runtime, so none of them is placed on
 * another thread: they run on the caller's, which is what makes them usable before a runtime exists
 * and during teardown after one is gone.
 */
public actual object Maplibre {
  /** C ABI contract version expected by this browser binding. */
  public actual const val EXPECTED_C_ABI_VERSION: Long = 0L

  /** The native default async log severity mask: error and warning. */
  private const val DEFAULT_LOG_SEVERITY_MASK: Int = (1 shl 1) or (1 shl 2)

  private val logCallbacks = LogCallbackRegistry<LogQueueBridge>()

  /**
   * Verifies that the browser module is loaded.
   *
   * A browser cannot load a module synchronously: the factory returns a promise and the pthread
   * pool spawns before it settles. So this checks rather than loads, and [loadNativeLibraryAsync]
   * is what a browser host calls first.
   */
  public actual fun loadNativeLibrary() {
    BrowserModule.require()
    checkCompatibleCAbi()
  }

  /**
   * Refuses a module whose C ABI contract version is not the one this binding was generated for.
   *
   * Split out of [loadNativeLibrary], as on every other platform, so the guard can be exercised
   * against a version no loadable module reports.
   */
  internal fun checkCompatibleCAbi(actualVersion: Long = cVersion()) {
    if (actualVersion == EXPECTED_C_ABI_VERSION) {
      return
    }

    throw AbiVersionMismatchException(actualVersion, EXPECTED_C_ABI_VERSION)
  }

  /**
   * Loads the browser module from [url], which names the ES module beside its wasm and manifest.
   *
   * Callers that race observe one instance. The module's headers digest and call protocol are
   * checked before it becomes reachable.
   */
  public suspend fun loadNativeLibraryAsync(url: String = DEFAULT_MODULE_URL) {
    BrowserModule.load(url)
    loadNativeLibrary()
  }

  /** Returns the native C ABI contract version. */
  public actual fun cVersion(): Long =
    NativeCall.call("mln_c_version", 0, {}, { Heap.loadLong(it) })

  /** Returns the render backends compiled into the loaded browser module. */
  public actual fun supportedRenderBackends(): Set<RenderBackend> =
    RenderBackend.fromMask(
      NativeCall.call("mln_supported_render_backend_mask", 0, {}, { Heap.loadInt(it) })
    )

  /** Returns the OpenGL context providers compiled into the loaded browser module. */
  public actual fun supportedOpenGLContextProviders(): Set<OpenGLContextProvider> =
    OpenGLContextProvider.fromMask(
      NativeCall.call("mln_opengl_supported_context_provider_mask", 0, {}, { Heap.loadInt(it) })
    )

  /** Reads Maplibre Native's process-global network status. */
  public actual val networkStatus: NetworkStatus
    get() =
      Heap.withScratch(4) { out ->
        NativeCall.call(
          "mln_network_status_get",
          1,
          { slots -> slots.setPointer(0, out) },
          { Status.check(Heap.loadInt(it)) },
        )
        NetworkStatus.fromNative(Heap.loadInt(out))
      }

  /** Sets Maplibre Native's process-global network status. */
  public actual fun setNetworkStatus(status: NetworkStatus) {
    Status.requireArgument(status.isKnown) {
      "Unknown network status cannot be used as input: ${status.nativeValue}"
    }
    NativeCall.call(
      "mln_network_status_set",
      1,
      { slots -> slots.setInt(0, status.nativeValue) },
      { Status.check(Heap.loadInt(it)) },
    )
  }

  /**
   * Installs or replaces the process-global native log callback.
   *
   * Records reach [callback] on a browser task of this binding's own, because logging has no
   * runtime to pump. They therefore arrive on a later turn of the page's event loop than the one
   * that produced them, rather than on the thread that produced them.
   *
   * **The callback's result is ignored, and native is told the record was not consumed.** MapLibre
   * needs that answer on the logging thread, which cannot enter the page's WebAssembly instance
   * where a host's callback body lives, so this binding answers in advance and always answers the
   * same way. Every record therefore reaches MapLibre's platform logger as well as [callback], and
   * a host that wants one sink filters at its own or narrows the severity mask with
   * [setAsyncLogSeverities].
   */
  public actual fun setLogCallback(callback: LogCallback) {
    BrowserModule.require()
    // Replacing from inside the callback being replaced cannot wait for that upcall to finish, so
    // it is rejected here rather than returning while the old callback is still running.
    logCallbacks.current()?.checkCanClose()
    val bridge = LogQueueBridge(callback)
    // Every installation begins an era, including a replacement. The registry runs its install
    // lambda only for the first callback, so relying on that would leave a replacement reading its
    // predecessor's mark and inheriting records produced before it existed. The mark is taken here,
    // before the registry publishes the new bridge.
    Status.check(LogQueueDrain.beginEra(logCallbacks::current))
    logCallbacks.set(bridge) { 0 }
  }

  /** Clears the process-global native log callback. */
  public actual fun clearLogCallback() {
    BrowserModule.require()
    logCallbacks.current()?.checkCanClose()
    logCallbacks.clear { LogQueueDrain.clear() }
  }

  /**
   * Drops the process-global log callback as part of a shutdown.
   *
   * The registration is a Kotlin reference rather than anything in the module's heap, so releasing
   * the module does not reclaim it: the bridge, the host's [LogCallback], and everything that
   * callback closes over would stay reachable from this object for the life of the document. It is
   * also the one root a host could not drop afterwards, because [clearLogCallback] refuses once the
   * module has been released.
   *
   * Runs while the module is still there, which is what lets it take the ordinary clear path.
   * Unlike [clearLogCallback] it does not refuse a callback that is mid-delivery: the dispatcher
   * has already been stopped by the time this runs, so a refusal here would leave a host with a
   * half-finished shutdown and nothing useful to do about it.
   */
  internal fun discardLogCallbackAfterShutdown() {
    logCallbacks.clear { LogQueueDrain.clear() }
  }

  /**
   * Whether a host log callback is installed.
   *
   * The seam the shutdown test reads. A page has no other way to see this root: the callback is
   * write-only from outside, and the thing that would prove it had gone -- the host's callback
   * becoming unreachable -- is not observable from Kotlin at all.
   */
  internal fun hasLogCallback(): Boolean = logCallbacks.current() != null

  /** Configures severities that native logging may dispatch asynchronously. */
  public actual fun setAsyncLogSeverities(severities: Set<LogSeverity>) {
    val mask = severities.fold(0) { accumulated, severity -> accumulated or severity.nativeMask }
    setAsyncLogSeverityMask(mask)
  }

  /** Restores the native default async log severity mask. */
  public actual fun restoreDefaultAsyncLogSeverities() {
    setAsyncLogSeverityMask(DEFAULT_LOG_SEVERITY_MASK)
  }

  private fun setAsyncLogSeverityMask(mask: Int) {
    NativeCall.call(
      "mln_log_set_async_severity_mask",
      1,
      { slots -> slots.setInt(0, mask) },
      { Status.check(Heap.loadInt(it)) },
    )
  }

  /** Converts a geographic coordinate to spherical Mercator projected meters. */
  public actual fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters =
    Heap.withScratch(MlnLatLng.SIZEOF + MlnProjectedMeters.SIZEOF) { scratch ->
      val out = scratch + MlnLatLng.SIZEOF
      MlnLatLng.setLatitude(scratch, coordinate.latitude)
      MlnLatLng.setLongitude(scratch, coordinate.longitude)
      NativeCall.call(
        "mln_projected_meters_for_lat_lng",
        2,
        { slots ->
          slots.setPointer(0, scratch)
          slots.setPointer(1, out)
        },
        { Status.check(Heap.loadInt(it)) },
      )
      ProjectedMeters(MlnProjectedMeters.northing(out), MlnProjectedMeters.easting(out))
    }

  /** Converts spherical Mercator projected meters to a geographic coordinate. */
  public actual fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng =
    Heap.withScratch(MlnProjectedMeters.SIZEOF + MlnLatLng.SIZEOF) { scratch ->
      val out = scratch + MlnProjectedMeters.SIZEOF
      MlnProjectedMeters.setNorthing(scratch, meters.northing)
      MlnProjectedMeters.setEasting(scratch, meters.easting)
      NativeCall.call(
        "mln_lat_lng_for_projected_meters",
        2,
        { slots ->
          slots.setPointer(0, scratch)
          slots.setPointer(1, out)
        },
        { Status.check(Heap.loadInt(it)) },
      )
      LatLng(MlnLatLng.latitude(out), MlnLatLng.longitude(out))
    }

  /** Where the module is expected to sit when a host serves it beside its own bundle. */
  public const val DEFAULT_MODULE_URL: String = "./maplibre_native_c.mjs"
}
