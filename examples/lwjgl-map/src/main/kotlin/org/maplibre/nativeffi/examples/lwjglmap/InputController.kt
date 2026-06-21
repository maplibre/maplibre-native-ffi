package org.maplibre.nativeffi.examples.lwjglmap

import kotlin.math.max
import kotlin.math.min
import kotlin.math.pow
import org.lwjgl.glfw.GLFW.GLFW_KEY_0
import org.lwjgl.glfw.GLFW.GLFW_KEY_A
import org.lwjgl.glfw.GLFW.GLFW_KEY_D
import org.lwjgl.glfw.GLFW.GLFW_KEY_DOWN
import org.lwjgl.glfw.GLFW.GLFW_KEY_E
import org.lwjgl.glfw.GLFW.GLFW_KEY_EQUAL
import org.lwjgl.glfw.GLFW.GLFW_KEY_LEFT
import org.lwjgl.glfw.GLFW.GLFW_KEY_LEFT_BRACKET
import org.lwjgl.glfw.GLFW.GLFW_KEY_MINUS
import org.lwjgl.glfw.GLFW.GLFW_KEY_Q
import org.lwjgl.glfw.GLFW.GLFW_KEY_RIGHT
import org.lwjgl.glfw.GLFW.GLFW_KEY_RIGHT_BRACKET
import org.lwjgl.glfw.GLFW.GLFW_KEY_S
import org.lwjgl.glfw.GLFW.GLFW_KEY_UP
import org.lwjgl.glfw.GLFW.GLFW_KEY_W
import org.lwjgl.glfw.GLFW.GLFW_MOD_CONTROL
import org.lwjgl.glfw.GLFW.GLFW_MOUSE_BUTTON_LEFT
import org.lwjgl.glfw.GLFW.GLFW_MOUSE_BUTTON_RIGHT
import org.lwjgl.glfw.GLFW.GLFW_PRESS
import org.lwjgl.glfw.GLFW.GLFW_RELEASE
import org.lwjgl.glfw.GLFW.GLFW_REPEAT
import org.lwjgl.glfw.GLFW.glfwSetCursorPosCallback
import org.lwjgl.glfw.GLFW.glfwSetKeyCallback
import org.lwjgl.glfw.GLFW.glfwSetMouseButtonCallback
import org.lwjgl.glfw.GLFW.glfwSetScrollCallback
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle

internal class InputController(
  private val window: Long,
  private val map: MapHandle,
  private val renderRequested: () -> Unit,
) : AutoCloseable {
  private var leftDown = false
  private var rightDown = false
  private var ctrlDown = false
  private var lastX = 0.0
  private var lastY = 0.0
  private var cursorX = 0.0
  private var cursorY = 0.0

  init {
    installCallbacks()
  }

  private fun installCallbacks() {
    glfwSetCursorPosCallback(window) { _, x, y -> onCursor(x, y) }
    glfwSetMouseButtonCallback(window) { _, button, action, mods -> onMouse(button, action, mods) }
    glfwSetScrollCallback(window) { _, _, yOffset -> onScroll(yOffset) }
    glfwSetKeyCallback(window) { _, key, _, action, mods -> onKey(key, action, mods) }
  }

  private fun onCursor(x: Double, y: Double) {
    cursorX = x
    cursorY = y
    val dx = x - lastX
    val dy = y - lastY
    lastX = x
    lastY = y
    if (rightDown || (leftDown && ctrlDown)) {
      setBearing(currentBearing() + dx * DRAG_ROTATE_FACTOR, animated = false)
      setPitch(currentPitch() - dy * DRAG_PITCH_FACTOR, animated = false)
      renderRequested()
    } else if (leftDown) {
      map.moveBy(dx, dy)
      renderRequested()
    }
  }

  private fun onMouse(button: Int, action: Int, mods: Int) {
    ctrlDown = (mods and GLFW_MOD_CONTROL) != 0
    when (button) {
      GLFW_MOUSE_BUTTON_LEFT ->
        leftDown =
          if (action == GLFW_PRESS) true else if (action == GLFW_RELEASE) false else leftDown

      GLFW_MOUSE_BUTTON_RIGHT ->
        rightDown =
          if (action == GLFW_PRESS) true else if (action == GLFW_RELEASE) false else rightDown
    }
    if (action == GLFW_PRESS) {
      map.cancelTransitions()
    }
  }

  private fun onScroll(yOffset: Double) {
    // GLFW reports OS-adjusted scroll deltas; use them directly so trackpads with natural
    // scrolling behave like the host platform expects.
    val scale = 2.0.pow(yOffset * 0.25)
    map.scaleBy(scale, ScreenPoint(cursorX, cursorY))
    renderRequested()
  }

  private fun onKey(key: Int, action: Int, mods: Int) {
    ctrlDown = (mods and GLFW_MOD_CONTROL) != 0
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
      return
    }
    val changed =
      when (key) {
        GLFW_KEY_LEFT,
        GLFW_KEY_A -> {
          map.moveByAnimated(KEYBOARD_PAN, 0.0, KEYBOARD_ANIMATION)
          true
        }

        GLFW_KEY_RIGHT,
        GLFW_KEY_D -> {
          map.moveByAnimated(-KEYBOARD_PAN, 0.0, KEYBOARD_ANIMATION)
          true
        }

        GLFW_KEY_UP,
        GLFW_KEY_W -> {
          map.moveByAnimated(0.0, KEYBOARD_PAN, KEYBOARD_ANIMATION)
          true
        }

        GLFW_KEY_DOWN,
        GLFW_KEY_S -> {
          map.moveByAnimated(0.0, -KEYBOARD_PAN, KEYBOARD_ANIMATION)
          true
        }

        GLFW_KEY_EQUAL -> {
          map.scaleByAnimated(KEYBOARD_ZOOM, viewportCenter(), KEYBOARD_ANIMATION)
          true
        }

        GLFW_KEY_MINUS -> {
          map.scaleByAnimated(1.0 / KEYBOARD_ZOOM, viewportCenter(), KEYBOARD_ANIMATION)
          true
        }

        GLFW_KEY_Q -> {
          setBearing(currentBearing() - KEYBOARD_BEARING, animated = true)
          true
        }

        GLFW_KEY_E -> {
          setBearing(currentBearing() + KEYBOARD_BEARING, animated = true)
          true
        }

        GLFW_KEY_RIGHT_BRACKET -> {
          setPitch(currentPitch() + KEYBOARD_PITCH, animated = true)
          true
        }

        GLFW_KEY_LEFT_BRACKET -> {
          setPitch(currentPitch() - KEYBOARD_PITCH, animated = true)
          true
        }

        GLFW_KEY_0 -> {
          map.easeTo(
            CameraOptions().apply {
              bearing = 0.0
              pitch = 0.0
            },
            RESET_ANIMATION,
          )
          true
        }

        else -> false
      }
    if (changed) {
      renderRequested()
    }
  }

  private fun currentBearing(): Double = map.camera.bearing ?: 0.0

  private fun currentPitch(): Double = map.camera.pitch ?: 0.0

  private fun viewportCenter(): ScreenPoint {
    val viewport = Viewport.read(window)
    return ScreenPoint(viewport.width() / 2.0, viewport.height() / 2.0)
  }

  private fun setBearing(bearing: Double, animated: Boolean) {
    val camera = CameraOptions().apply { this.bearing = bearing }
    if (animated) {
      map.easeTo(camera, KEYBOARD_ANIMATION)
    } else {
      map.jumpTo(camera)
    }
  }

  private fun setPitch(pitch: Double, animated: Boolean) {
    val clamped = max(0.0, min(60.0, pitch))
    val camera = CameraOptions().apply { this.pitch = clamped }
    if (animated) {
      map.easeTo(camera, KEYBOARD_ANIMATION)
    } else {
      map.jumpTo(camera)
    }
  }

  override fun close() {
    glfwSetCursorPosCallback(window, null)
    glfwSetMouseButtonCallback(window, null)
    glfwSetScrollCallback(window, null)
    glfwSetKeyCallback(window, null)
  }

  internal companion object {
    private const val DRAG_ROTATE_FACTOR = 0.5
    private const val DRAG_PITCH_FACTOR = 0.5
    private const val KEYBOARD_PAN = 120.0
    private const val KEYBOARD_ZOOM = 1.25
    private const val KEYBOARD_BEARING = 10.0
    private const val KEYBOARD_PITCH = 5.0
    private val KEYBOARD_ANIMATION = animation(160.0)
    private val RESET_ANIMATION = animation(220.0)

    fun printControls() {
      println("Controls:")
      println("  left drag: pan")
      println("  right drag or Ctrl+left drag: rotate with X, pitch with Y")
      println("  scroll: zoom at cursor")
      println("  arrows or WASD: pan")
      println("  + / -: zoom at center")
      println("  Q / E: rotate")
      println("  ] / [: pitch")
      println("  0: reset pitch and bearing")
    }

    private fun animation(durationMs: Double): AnimationOptions =
      AnimationOptions().apply { this.durationMs = durationMs }
  }
}
