package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.atomic.AtomicBoolean
import kotlin.math.max
import kotlin.math.min
import kotlin.time.Duration.Companion.seconds
import kotlin.time.TimeSource
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.WakeSource

/**
 * Owns the runtime and the map for their whole lifetime, on a dedicated thread that is not the one
 * presenting.
 *
 * Native owner-thread checks are keyed on the OS thread, so this is a plain [Thread] rather than a
 * coroutine dispatcher or a pooled executor. The loop never touches the render session: the render
 * loop attaches its own session against the map published here, and closes it before [close] lets
 * this loop destroy the map.
 */
internal class MapRuntimeLoop(
  private val initialExtent: SurfaceExtent,
  private val commands: CameraCommandQueue,
  private val renderRequest: RenderRequest,
) : AutoCloseable {
  /** The map's scale factor is fixed at creation, so a density change restarts the whole loop. */
  val scaleFactor: Double
    get() = initialExtent.scaleFactor

  private val shutdownRequested = AtomicBoolean(false)

  @Volatile private var publishedMap: MapHandle? = null

  /** The first failure seen on this loop, republished by the render loop. */
  @Volatile
  var failure: Throwable? = null
    private set

  /** The map to attach against, once the loop has created one. */
  val map: MapHandle?
    get() = publishedMap

  @Volatile private var wake: WakeSource? = null

  private val thread = Thread({ run() }, "compose-map-runtime").apply { isDaemon = true }

  init {
    thread.start()
  }

  override fun close() {
    shutdownRequested.set(true)
    commands.wake()
    thread.join()
  }

  private fun run() {
    var runtime: RuntimeHandle? = null
    var map: MapHandle? = null
    try {
      val createdRuntime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
      runtime = createdRuntime
      val extent = initialExtent
      val createdMap =
        MapHandle.create(
          createdRuntime,
          MapOptions().apply {
            width = extent.width
            height = extent.height
            scaleFactor = extent.scaleFactor
            mapMode = MapMode.CONTINUOUS
          },
        )
      map = createdMap
      createdMap.setStyleUrl(STYLE_URL)
      createdMap.jumpTo(
        CameraOptions().apply {
          center = LatLng(37.7749, -122.4194)
          zoom = 13.0
          bearing = 12.0
          pitch = 30.0
        }
      )
      publishedMap = createdMap
      pump(createdRuntime, createdMap)
    } catch (error: Throwable) {
      failure = error
    } finally {
      publishedMap = null
      // The render loop owns the session and only sees `failure` on a later
      // frame. Native refuses to destroy a map that still has one attached, so
      // wait for the render loop to close it and shut this loop down. Bounded,
      // so a render loop that already stopped cannot wedge teardown.
      awaitShutdown()
      // A wake source is its own native handle and outlives the runtime, so
      // closing the runtime does not release it.
      commands.onEnqueue = null
      wake?.close()
      wake = null
      try {
        map?.close()
      } finally {
        runtime?.close()
      }
    }
  }

  /** Blocks until [close] is called, or the bound expires. */
  private fun awaitShutdown() {
    val deadline = TimeSource.Monotonic.markNow() + SHUTDOWN_WAIT
    while (!shutdownRequested.get() && deadline.hasNotPassedNow()) {
      Thread.sleep(SHUTDOWN_POLL_MS)
    }
  }

  private fun pump(runtime: RuntimeHandle, map: MapHandle) {
    val source = runtime.acquireWakeSource()
    wake = source
    commands.onEnqueue = { source.signal() }
    val batch = ArrayList<CameraCommand>()
    while (!shutdownRequested.get()) {
      batch.clear()
      commands.drainInto(batch)
      batch.forEach { command -> apply(map, command) }
      // This thread has no display to pace it, so it takes its cadence from the runtime's own
      // work and parks in between. The render loop signals the wake source, so the bound is a
      // backstop rather than the cadence.
      runtime.pump(PARK_TIMEOUT_MS)
      if (drainEvents(runtime, map)) {
        renderRequest.set()
      }
    }
  }

  /**
   * Applies one decoded camera command on the map's owner thread, which is why the
   * read-modify-write commands read the current camera here rather than on the render loop that
   * produced them.
   */
  private fun apply(map: MapHandle, command: CameraCommand) {
    when (command) {
      CameraCommand.CancelTransitions -> map.cancelTransitions()
      is CameraCommand.SetGestureInProgress -> map.isGestureInProgress = command.inProgress
      is CameraCommand.MoveBy -> map.moveBy(command.deltaX, command.deltaY)
      is CameraCommand.MoveByAnimated ->
        map.moveByAnimated(command.deltaX, command.deltaY, KEYBOARD_ANIMATION)
      is CameraCommand.ScaleBy -> map.scaleBy(command.scale, command.anchor)
      is CameraCommand.ScaleByAnimated ->
        map.scaleByAnimated(command.scale, command.anchor, KEYBOARD_ANIMATION)
      is CameraCommand.AdjustBearingAndPitch -> {
        val camera = map.camera
        map.jumpTo(
          CameraOptions().apply {
            bearing = (camera.bearing ?: 0.0) + command.bearingDegrees
            pitch = ((camera.pitch ?: 0.0) + command.pitchDegrees).clampPitch()
          }
        )
      }
      is CameraCommand.AdjustBearingAnimated ->
        map.easeTo(
          CameraOptions().apply { bearing = (map.camera.bearing ?: 0.0) + command.bearingDegrees },
          KEYBOARD_ANIMATION,
        )
      is CameraCommand.AdjustPitchAnimated ->
        map.easeTo(
          CameraOptions().apply {
            pitch = ((map.camera.pitch ?: 0.0) + command.pitchDegrees).clampPitch()
          },
          KEYBOARD_ANIMATION,
        )
      CameraCommand.ResetOrientation ->
        map.easeTo(
          CameraOptions().apply {
            bearing = 0.0
            pitch = 0.0
          },
          RESET_ANIMATION,
        )
    }
  }

  /** Drains runtime events, reporting whether the map wants another frame. */
  private fun drainEvents(runtime: RuntimeHandle, map: MapHandle): Boolean {
    var renderUpdateAvailable = false
    while (true) {
      val event = runtime.pollEvent() ?: return renderUpdateAvailable
      if (event.mapSource != map) {
        continue
      }
      if (event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE) {
        renderUpdateAvailable = true
      } else if (
        event.type == RuntimeEventType.MAP_RENDER_FRAME_FINISHED &&
          (event.payload as? RuntimeEventPayload.RenderFrame)?.needsRepaint == true
      ) {
        renderUpdateAvailable = true
      }
    }
  }

  private companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"
    /**
     * Backstop for the runtime loop's park. The render loop's wake source is what normally releases
     * it, so this only bounds a pump that nothing signals.
     */
    private const val PARK_TIMEOUT_MS = 100L

    /** Bound on waiting for the render loop to close its session at teardown. */
    private val SHUTDOWN_WAIT = 5.seconds
    private const val SHUTDOWN_POLL_MS = 2L
    private val KEYBOARD_ANIMATION = AnimationOptions().apply { durationMs = 160.0 }
    private val RESET_ANIMATION = AnimationOptions().apply { durationMs = 160.0 }
  }
}

private fun Double.clampPitch(): Double = max(0.0, min(60.0, this))
