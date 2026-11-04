// A continuous map on the surface of an AndroidExternalSurface. The controller attaches a session
// to the first surface, and gives the session each later one. Everything else matches the React
// Native example in examples/react-native and the browser example in examples/browser.
package org.maplibre.nativeffi.example.compose

import android.app.Application
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.Surface
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import java.util.concurrent.CountDownLatch
import kotlin.coroutines.cancellation.CancellationException
import kotlin.math.roundToInt
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import org.maplibre.nativeffi.CameraOptions
import org.maplibre.nativeffi.CameraUpdate
import org.maplibre.nativeffi.CameraUpdateMode
import org.maplibre.nativeffi.LatLng
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.LogicalSize
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.MapOptions
import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderResult
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.RuntimeEvent
import org.maplibre.nativeffi.RuntimeEventFlag
import org.maplibre.nativeffi.RuntimeEventHandler
import org.maplibre.nativeffi.RuntimeEventType
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.Wake
import org.maplibre.nativeffi.androidFramePacerCreate
import org.maplibre.nativeffi.attachAndroidSurface
import org.maplibre.nativeffi.setAndroidSurfaceTarget

private const val STYLE_URL = "https://tiles.openfreemap.org/styles/liberty"
private const val TAG = "MapController"

data class Status(val message: String, val error: Boolean = false)

// Every method runs on the main thread. The binding calls the wakes and the event handler on its
// own threads, and they post their work here.
class MapController(application: Application) : AndroidViewModel(application) {
  var status by mutableStateOf(Status(""))
    private set

  var map: MapHandle? = null
    private set

  private val main = Handler(Looper.getMainLooper())
  private val density = application.resources.displayMetrics.density
  // Paces frames to the display through the main thread's choreographer. A paced session has no
  // redraw wake: the pacer demands each frame.
  private val pacer = androidFramePacerCreate()
  private var runtime: RuntimeHandle? = null
  private var session: RenderSessionHandle? = null
  private var failed = false

  // The view's latest surface and size, and the surface that the session renders to.
  private var surface: Surface? = null
  private var size: LogicalSize? = null
  private var target: Surface? = null
  // The size that the session last got.
  private var targetSize: LogicalSize? = null
  // Target changes run one at a time, in the order the view reports them.
  private var targets: Job? = null

  init {
    viewModelScope.launch {
      try {
        start()
      } catch (error: Throwable) {
        if (error !is CancellationException) fail(error)
      }
    }
  }

  fun surfaceCreated(surface: Surface, width: Int, height: Int) {
    this.surface = surface
    size = logicalSize(width, height)
    retarget()
  }

  fun surfaceChanged(width: Int, height: Int) {
    size = logicalSize(width, height)
    // While a target change runs, it applies the new size when it finishes.
    if (targets?.isActive != true) resize()
  }

  // The platform is taking the surface away, as when the app goes to the background or the
  // activity restarts. The session must let go of it before this returns, so this blocks until the
  // release completes. That can't deadlock: the session's worker thread completes the release, and
  // it never waits for the main thread. The session keeps its tiles for the next surface.
  fun surfaceDestroyed(surface: Surface) {
    if (this.surface == surface) this.surface = null
    if (target != surface) return
    target = null
    val session = session ?: return
    // A session that is still attaching can't release its target yet. One that failed to attach
    // has no target.
    if (session.ready.waitBlocking() != null) return
    val failure =
      try {
        session.releaseTarget().waitBlocking()
      } catch (error: MaplibreException) {
        error
      }
    // The session failed, and the frame results report it.
    if (failure != null) Log.w(TAG, "Couldn't release the surface", failure)
  }

  override fun onCleared() {
    val session = session
    val map = map
    val runtime = runtime
    this.session = null
    this.map = null
    this.runtime = null
    // The view model's scope has ended, so the release finishes in the background, in order.
    @OptIn(DelicateCoroutinesApi::class)
    GlobalScope.launch(Dispatchers.Default) {
      try {
        session?.release()?.await()
        map?.release()?.await()
        runtime?.release()?.await()
      } catch (error: MaplibreException) {
        Log.w(TAG, "Couldn't release the map", error)
      }
      pacer.release()
    }
  }

  private suspend fun start() {
    showStatus("Loading the library…")
    val runtime = RuntimeHandle.create()
    this.runtime = runtime
    val size = size ?: LogicalSize(1u, 1u)
    val map =
      runtime.createMap(
        MapOptions(
          initialExtent = LogicalExtent(size.width, size.height, density.toDouble()),
          mapMode = MapMode.CONTINUOUS,
          eventMask =
            RuntimeEventFlag.MAP_STYLE_LOADED or
              RuntimeEventFlag.MAP_LOADING_FAILED or
              RuntimeEventFlag.MAP_RENDER_ERROR,
        ),
      )
    this.map = map
    runtime.discarding.setEventHandler(
      RuntimeEventHandler { events -> main.post { onEvents(map, events) } },
    )
    map.ready.await()
    map.discarding.updateCamera(
      CameraUpdate(
        mode = CameraUpdateMode.JUMP,
        camera =
          CameraOptions(
            center = LatLng(latitude = 48.8584, longitude = 2.2945),
            zoom = 15.5,
            bearing = -30.0,
            pitch = 55.0,
          ),
      ),
    )
    map.discarding.setStyleUrl(STYLE_URL)
    showStatus("Loading the style…")
    retarget()
  }

  private fun onEvents(map: MapHandle, events: List<RuntimeEvent>) {
    for (event in events) {
      if (event.source != map.id) continue
      when (event.type) {
        RuntimeEventType.MAP_STYLE_LOADED -> showStatus("")
        RuntimeEventType.MAP_LOADING_FAILED,
        RuntimeEventType.MAP_RENDER_ERROR -> fail(MapError(event.message))
      }
    }
  }

  // Gives the session the view's latest surface, attaching the session first if there is none.
  private fun retarget() {
    val previous = targets
    targets = viewModelScope.launch {
      previous?.join()
      try {
        applyTarget()
        resize()
      } catch (error: Throwable) {
        if (error !is CancellationException) fail(error)
      }
    }
  }

  private suspend fun applyTarget() {
    // The view can report its surface before the map is ready to take a session.
    val map = map ?: return
    val surface = surface ?: return
    val size = size ?: return
    if (surface == target) return
    target = surface
    val session = session
    if (session == null) {
      val attached =
        map.attachAndroidSurface(
          surface,
          size,
          options =
            RenderSessionAttachOptions(
              driver = RenderDriverKind.CORE_WORKER,
              frameWake = Wake { main.post(::drainFrameResults) },
            ),
        )
      this.session = attached
      attached.setFramePacer(pacer)
      attached.ready.await()
    } else {
      session.setAndroidSurfaceTarget(surface, size).await()
    }
    targetSize = size
  }

  // Gives the session the view's latest size.
  private fun resize() {
    val session = session ?: return
    val size = size ?: return
    if (target == null || size == targetSize) return
    targetSize = size
    try {
      session.discarding.resize(size)
    } catch (error: MaplibreException) {
      fail(error)
    }
  }

  private fun drainFrameResults() {
    val session = session ?: return
    val results = session.drainFrameResults()
    if (results.any { it.disposition == RenderResult.FAILED }) {
      fail(MapError("The map failed to render a frame: ${session.getFailure()}"))
    }
  }

  private fun logicalSize(width: Int, height: Int) =
    LogicalSize(
      (width / density).roundToInt().coerceAtLeast(1).toUInt(),
      (height / density).roundToInt().coerceAtLeast(1).toUInt(),
    )

  // Once the controller shows an error, later progress doesn't replace it.
  private fun showStatus(message: String) {
    if (!failed) status = Status(message)
  }

  private fun fail(error: Throwable) {
    Log.e(TAG, "The map failed", error)
    if (failed) return
    failed = true
    status = Status(error.message ?: error.toString(), error = true)
  }
}

private class MapError(message: String) : Exception(message)

// Blocks the calling thread until the deferred completes, and returns its failure. It waits
// without a dispatcher, so it doesn't depend on the main thread's event loop.
private fun Deferred<*>.waitBlocking(): Throwable? {
  val done = CountDownLatch(1)
  var failure: Throwable? = null
  invokeOnCompletion { cause ->
    failure = cause
    done.countDown()
  }
  done.await()
  return failure
}
