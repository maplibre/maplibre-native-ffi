package org.maplibre.nativeffi.examples.lwjglmap

import java.util.concurrent.atomic.AtomicBoolean
import kotlin.math.PI
import kotlin.math.atan
import kotlin.math.ln
import kotlin.math.max
import kotlin.math.min
import kotlin.math.pow
import kotlin.math.sin
import kotlin.math.sinh
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.CameraUpdateMode
import org.maplibre.nativeffi.camera.GesturePhase
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.MapSize
import org.maplibre.nativeffi.runtime.CommandDisposition
import org.maplibre.nativeffi.runtime.ReadyEndpoint
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/** Runtime and map state driven by the core-owned runtime worker. */
internal class MapState
private constructor(
  private val runtime: RuntimeHandle,
  val map: MapHandle,
  initialCamera: CameraOptions,
  private val scheduleNotificationDrain: () -> Unit,
) : AutoCloseable {
  private val notificationPending = AtomicBoolean(false)
  private val cameraLock = Any()
  private var desiredCamera = initialCamera.copy()
  private var gestureId = 0L

  init {
    runtime.setNotificationCallback {
      if (notificationPending.compareAndSet(false, true)) {
        scheduleNotificationDrain()
      }
    }
  }

  /** Submits a camera command directly from the GLFW receiver thread. */
  fun submit(command: CameraCommand) {
    synchronized(cameraLock) {
      when (command) {
        CameraCommand.CancelTransitions -> submit(CameraUpdate())
        is CameraCommand.SetGestureInProgress -> {
          if (command.inProgress) gestureId += 1
          submit(
            CameraUpdate(
              gesturePhase = if (command.inProgress) GesturePhase.BEGIN else GesturePhase.END,
              gestureId = gestureId,
            )
          )
        }
        is CameraCommand.MoveBy -> moveBy(command.dx, command.dy, null)
        is CameraCommand.MoveByAnimated -> moveBy(command.dx, command.dy, command.durationMs)
        is CameraCommand.ScaleBy -> scaleBy(command.scale, command.anchor, null)
        is CameraCommand.ScaleByAnimated ->
          scaleBy(command.scale, command.anchor, command.durationMs)
        is CameraCommand.PitchBy -> update(CameraOptions().apply { pitch = pitch(command.delta) })
        is CameraCommand.AdjustBearing ->
          update(CameraOptions().apply { bearing = bearing(command.delta) })
        is CameraCommand.AdjustBearingAnimated ->
          update(CameraOptions().apply { bearing = bearing(command.delta) }, command.durationMs)
        is CameraCommand.AdjustPitchAnimated ->
          update(CameraOptions().apply { pitch = pitch(command.delta) }, command.durationMs)
        is CameraCommand.ResetOrientation ->
          update(
            CameraOptions().apply {
              bearing = 0.0
              pitch = 0.0
            },
            command.durationMs,
          )
      }
    }
  }

  fun resize(viewport: Viewport) {
    map.resize(MapSize(viewport.width(), viewport.height(), viewport.scaleFactor()))
  }

  fun requestRepaint() {
    map.requestRepaint()
  }

  /** Drains owned readiness and event batches on the GLFW receiver thread. */
  fun drainNotifications(renderRequest: RenderRequest) {
    if (!notificationPending.getAndSet(false)) return
    do {
      val ready = runtime.drainReady()
      if (ready.any { it.kind == ReadyEndpoint.Kind.DRIVER_WORK }) {
        renderRequest.set()
      }
      if (ready.any { it.kind == ReadyEndpoint.Kind.RUNTIME_EVENTS } && drainEvents()) {
        renderRequest.set()
      }
    } while (notificationPending.getAndSet(false))
  }

  private fun moveBy(dx: Double, dy: Double, durationMs: Double?) {
    val current = desiredCamera
    val center = current.center ?: return
    val zoom = current.zoom ?: 0.0
    val bearingRadians = (current.bearing ?: 0.0) * PI / 180.0
    val worldDx = -(dx * kotlin.math.cos(bearingRadians) - dy * sin(bearingRadians))
    val worldDy = -(dx * sin(bearingRadians) + dy * kotlin.math.cos(bearingRadians))
    val worldSize = TILE_SIZE * 2.0.pow(zoom)
    val x = (center.longitude + 180.0) / 360.0 * worldSize + worldDx
    val latitudeRadians =
      center.latitude.coerceIn(-MAX_MERCATOR_LATITUDE, MAX_MERCATOR_LATITUDE) * PI / 180.0
    val y =
      (0.5 - ln(kotlin.math.tan(PI / 4.0 + latitudeRadians / 2.0)) / (2.0 * PI)) * worldSize +
        worldDy
    val next =
      LatLng(
        atan(sinh(PI * (1.0 - 2.0 * y / worldSize))) * 180.0 / PI,
        x / worldSize * 360.0 - 180.0,
      )
    update(CameraOptions().apply { this.center = next }, durationMs)
  }

  private fun scaleBy(
    scale: Double,
    anchor: org.maplibre.nativeffi.geo.ScreenPoint,
    durationMs: Double?,
  ) {
    val zoom = (desiredCamera.zoom ?: 0.0) + ln(scale) / ln(2.0)
    update(
      CameraOptions().apply {
        this.zoom = zoom
        this.anchor = anchor
      },
      durationMs,
    )
  }

  private fun bearing(delta: Double): Double = (desiredCamera.bearing ?: 0.0) + delta

  private fun pitch(delta: Double): Double =
    max(0.0, min(60.0, (desiredCamera.pitch ?: 0.0) + delta))

  private fun update(camera: CameraOptions, durationMs: Double? = null) {
    desiredCamera = desiredCamera.copy {
      camera.center?.let { center = it }
      camera.zoom?.let { zoom = it }
      camera.bearing?.let { bearing = it }
      camera.pitch?.let { pitch = it }
    }
    submit(
      CameraUpdate(
        mode = if (durationMs == null) CameraUpdateMode.JUMP else CameraUpdateMode.EASE,
        camera = camera,
        animation = AnimationOptions().apply { durationMs?.let { this.durationMs = it } },
      )
    )
  }

  private fun submit(update: CameraUpdate) {
    map.updateCamera(update)
  }

  private fun drainEvents(): Boolean {
    var renderUpdateAvailable = false
    for (event in runtime.drainEvents().events) {
      if (event.mapSource != map) continue
      if (event.type == RuntimeEventType.COMMAND_FINISHED) {
        val terminal = event.payload as? RuntimeEventPayload.CommandFinished
        if (terminal?.disposition == CommandDisposition.FAILED) {
          System.err.println(
            "command ${terminal.commandId} failed with status ${event.code}: ${event.message}"
          )
        }
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
    return renderUpdateAvailable
  }

  override fun close() {
    runtime.clearNotificationCallback()
    try {
      runSuspend { map.close() }
    } finally {
      runSuspend { runtime.close() }
    }
  }

  companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"
    private const val TILE_SIZE = 512.0
    private const val MAX_MERCATOR_LATITUDE = 85.0511287798066

    fun create(viewport: Viewport, scheduleNotificationDrain: () -> Unit): MapState {
      val runtime = runSuspend {
        RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
      }
      val initialCamera =
        CameraOptions().apply {
          center = LatLng(37.7749, -122.4194)
          zoom = 13.0
          bearing = 12.0
          pitch = 30.0
        }
      val map =
        try {
          runSuspend {
            MapHandle.create(
              runtime,
              MapOptions().apply {
                width = viewport.width()
                height = viewport.height()
                scaleFactor = viewport.scaleFactor()
                mapMode = MapMode.CONTINUOUS
                eventMask =
                  RuntimeEventMask.MAP_RENDER_UPDATE_AVAILABLE +
                    RuntimeEventMask.MAP_RENDER_FRAME_FINISHED
              },
            )
          }
        } catch (error: Throwable) {
          runSuspend { runtime.close() }
          throw error
        }
      try {
        val state = MapState(runtime, map, initialCamera, scheduleNotificationDrain)
        map.setStyleUrl(STYLE_URL)
        state.submit(CameraUpdate(camera = initialCamera))
        runSuspend { runtime.barrier() }
        state.desiredCamera = map.cameraSnapshot().camera.copy()
        return state
      } catch (error: Throwable) {
        runSuspend { map.close() }
        runSuspend { runtime.close() }
        throw error
      }
    }
  }
}
