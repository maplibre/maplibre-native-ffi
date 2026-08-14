package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.atomic.AtomicBoolean
import kotlin.math.PI
import kotlin.math.atan
import kotlin.math.cos
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
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
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
internal class MapState(
  initialExtent: SurfaceExtent,
  private val scheduleNotificationDrain: () -> Unit,
  private val requestRender: () -> Unit,
) : AutoCloseable {
  private val notificationPending = AtomicBoolean(false)
  private val cameraLock = Any()
  private var closed = false
  private var gestureId = 0L
  private var currentSize =
    MapSize(initialExtent.width, initialExtent.height, initialExtent.scaleFactor)
  private var desiredCamera =
    CameraOptions().apply {
      center = LatLng(37.7749, -122.4194)
      zoom = 13.0
      bearing = 12.0
      pitch = 30.0
    }

  private val runtime = runSuspend {
    RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
  }
  private lateinit var ownedMap: MapHandle
  val map: MapHandle
    get() = ownedMap

  init {
    runtime.setNotificationCallback {
      if (notificationPending.compareAndSet(false, true)) {
        scheduleNotificationDrain()
      }
    }
    try {
      ownedMap = runSuspend {
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = initialExtent.width
            height = initialExtent.height
            scaleFactor = initialExtent.scaleFactor
            mapMode = MapMode.CONTINUOUS
            eventMask =
              RuntimeEventMask.MAP_RENDER_UPDATE_AVAILABLE +
                RuntimeEventMask.MAP_RENDER_FRAME_FINISHED
          },
        )
      }
      map.setStyleUrl(STYLE_URL)
      map.updateCamera(CameraUpdate(camera = desiredCamera.copy()))
      runSuspend { runtime.barrier() }
      desiredCamera = map.cameraSnapshot().camera.copy()
    } catch (error: Throwable) {
      runtime.clearNotificationCallback()
      if (::ownedMap.isInitialized) runSuspend { ownedMap.close() }
      runSuspend { runtime.close() }
      throw error
    }
  }

  fun submit(command: CameraCommand) {
    synchronized(cameraLock) {
      when (command) {
        CameraCommand.CancelTransitions -> map.updateCamera(CameraUpdate())
        is CameraCommand.SetGestureInProgress -> {
          if (command.inProgress) gestureId += 1
          map.updateCamera(
            CameraUpdate(
              gesturePhase = if (command.inProgress) GesturePhase.BEGIN else GesturePhase.END,
              gestureId = gestureId,
            )
          )
        }
        is CameraCommand.MoveBy -> moveBy(command.deltaX, command.deltaY, null)
        is CameraCommand.MoveByAnimated ->
          moveBy(command.deltaX, command.deltaY, KEYBOARD_ANIMATION_MS)
        is CameraCommand.ScaleBy -> scaleBy(command.scale, command.anchor, null)
        is CameraCommand.ScaleByAnimated ->
          scaleBy(command.scale, command.anchor, KEYBOARD_ANIMATION_MS)
        is CameraCommand.AdjustBearingAndPitch ->
          update(
            CameraOptions().apply {
              bearing = (desiredCamera.bearing ?: 0.0) + command.bearingDegrees
              pitch = ((desiredCamera.pitch ?: 0.0) + command.pitchDegrees).clampPitch()
            }
          )
        is CameraCommand.AdjustBearingAnimated ->
          update(
            CameraOptions().apply {
              bearing = (desiredCamera.bearing ?: 0.0) + command.bearingDegrees
            },
            KEYBOARD_ANIMATION_MS,
          )
        is CameraCommand.AdjustPitchAnimated ->
          update(
            CameraOptions().apply {
              pitch = ((desiredCamera.pitch ?: 0.0) + command.pitchDegrees).clampPitch()
            },
            KEYBOARD_ANIMATION_MS,
          )
        CameraCommand.ResetOrientation ->
          update(
            CameraOptions().apply {
              bearing = 0.0
              pitch = 0.0
            },
            RESET_ANIMATION_MS,
          )
      }
    }
  }

  fun resize(extent: SurfaceExtent) {
    val size = MapSize(extent.width, extent.height, extent.scaleFactor)
    if (size != currentSize) {
      currentSize = size
      map.resize(size)
    }
  }

  /** Drains owned readiness and event batches on the native-surface producer thread. */
  fun drainNotifications() {
    if (!notificationPending.getAndSet(false)) return
    do {
      val ready = runtime.drainReady()
      if (ready.any { it.kind == ReadyEndpoint.Kind.DRIVER_WORK }) requestRender()
      if (ready.any { it.kind == ReadyEndpoint.Kind.RUNTIME_EVENTS } && drainEvents()) {
        requestRender()
      }
    } while (notificationPending.getAndSet(false))
  }

  private fun moveBy(dx: Double, dy: Double, durationMs: Double?) {
    val center = desiredCamera.center ?: return
    val zoom = desiredCamera.zoom ?: 0.0
    val bearingRadians = (desiredCamera.bearing ?: 0.0) * PI / 180.0
    val worldDx = -(dx * cos(bearingRadians) - dy * sin(bearingRadians))
    val worldDy = -(dx * sin(bearingRadians) + dy * cos(bearingRadians))
    val worldSize = TILE_SIZE * 2.0.pow(zoom)
    val x = (center.longitude + 180.0) / 360.0 * worldSize + worldDx
    val latitudeRadians =
      center.latitude.coerceIn(-MAX_MERCATOR_LATITUDE, MAX_MERCATOR_LATITUDE) * PI / 180.0
    val y =
      (0.5 - ln(kotlin.math.tan(PI / 4.0 + latitudeRadians / 2.0)) / (2.0 * PI)) * worldSize +
        worldDy
    update(
      CameraOptions().apply {
        this.center =
          LatLng(
            atan(sinh(PI * (1.0 - 2.0 * y / worldSize))) * 180.0 / PI,
            x / worldSize * 360.0 - 180.0,
          )
      },
      durationMs,
    )
  }

  private fun scaleBy(
    scale: Double,
    anchor: org.maplibre.nativeffi.geo.ScreenPoint,
    durationMs: Double?,
  ) {
    update(
      CameraOptions().apply {
        zoom = (desiredCamera.zoom ?: 0.0) + ln(scale) / ln(2.0)
        this.anchor = anchor
      },
      durationMs,
    )
  }

  private fun update(camera: CameraOptions, durationMs: Double? = null) {
    desiredCamera = desiredCamera.copy {
      camera.center?.let { center = it }
      camera.zoom?.let { zoom = it }
      camera.bearing?.let { bearing = it }
      camera.pitch?.let { pitch = it }
    }
    map.updateCamera(
      CameraUpdate(
        mode = if (durationMs == null) CameraUpdateMode.JUMP else CameraUpdateMode.EASE,
        camera = camera,
        animation = AnimationOptions().apply { durationMs?.let { this.durationMs = it } },
      )
    )
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
    if (closed) return
    closed = true
    runtime.clearNotificationCallback()
    try {
      runSuspend { map.close() }
    } finally {
      runSuspend { runtime.close() }
    }
  }

  private companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"
    private const val TILE_SIZE = 512.0
    private const val MAX_MERCATOR_LATITUDE = 85.0511287798066
    private const val KEYBOARD_ANIMATION_MS = 160.0
    private const val RESET_ANIMATION_MS = 160.0
  }
}

private fun Double.clampPitch(): Double = max(0.0, min(60.0, this))
