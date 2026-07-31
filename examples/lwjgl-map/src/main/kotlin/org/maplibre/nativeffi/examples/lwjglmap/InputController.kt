package org.maplibre.nativeffi.examples.lwjglmap

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
import org.maplibre.nativeffi.geo.ScreenPoint

/**
 * Decodes GLFW input into camera commands.
 *
 * GLFW delivers these callbacks on the render loop thread, which does not own the map, so this only
 * produces commands; the runtime loop applies them on the map's thread. Anything needing the
 * current viewport is converted here, where the viewport lives.
 */
internal class InputController(
  private val window: Long,
  private val commands: CommandQueue,
  private val renderRequest: RenderRequest,
  private val viewport: () -> Viewport,
) : AutoCloseable {
  private var leftDown = false
  private var rightDown = false
  private var ctrlDown = false
  private var lastX = 0.0
  private var lastY = 0.0
  private var cursorX = 0.0
  private var cursorY = 0.0

  init {
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
    if (dx == 0.0 && dy == 0.0) {
      return
    }
    if (rightDown || (leftDown && ctrlDown)) {
      commands.push(CameraCommand.AdjustBearing(dx * DRAG_ROTATE_FACTOR))
      commands.push(CameraCommand.PitchBy(dy * DRAG_PITCH_FACTOR))
    } else if (leftDown) {
      commands.push(CameraCommand.MoveBy(dx, dy))
    } else {
      return
    }
    renderRequest.set()
  }

  private fun onMouse(button: Int, action: Int, mods: Int) {
    ctrlDown = (mods and GLFW_MOD_CONTROL) != 0
    val wasDragging = dragging
    when (button) {
      GLFW_MOUSE_BUTTON_LEFT ->
        leftDown =
          if (action == GLFW_PRESS) true else if (action == GLFW_RELEASE) false else leftDown

      GLFW_MOUSE_BUTTON_RIGHT ->
        rightDown =
          if (action == GLFW_PRESS) true else if (action == GLFW_RELEASE) false else rightDown
    }
    if (action == GLFW_PRESS) {
      // Queued ahead of the drag's own commands, so the transition stops before the first delta
      // lands.
      commands.push(CameraCommand.CancelTransitions)
    }
    // The deltas in between belong to one live gesture, so the map hears about the gesture rather
    // than a stream of unrelated camera commands.
    if (dragging != wasDragging) {
      commands.push(CameraCommand.SetGestureInProgress(dragging))
    }
  }

  private val dragging: Boolean
    get() = leftDown || rightDown

  private fun onScroll(yOffset: Double) {
    // GLFW reports OS-adjusted scroll deltas; use them directly so trackpads with natural
    // scrolling behave like the host platform expects.
    val scale = 2.0.pow(yOffset * 0.25)
    commands.push(CameraCommand.ScaleBy(scale, ScreenPoint(cursorX, cursorY)))
    renderRequest.set()
  }

  private fun onKey(key: Int, action: Int, mods: Int) {
    ctrlDown = (mods and GLFW_MOD_CONTROL) != 0
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
      return
    }
    val command =
      when (key) {
        GLFW_KEY_LEFT,
        GLFW_KEY_A -> CameraCommand.MoveByAnimated(KEYBOARD_PAN, 0.0, KEYBOARD_ANIMATION_MS)

        GLFW_KEY_RIGHT,
        GLFW_KEY_D -> CameraCommand.MoveByAnimated(-KEYBOARD_PAN, 0.0, KEYBOARD_ANIMATION_MS)

        GLFW_KEY_UP,
        GLFW_KEY_W -> CameraCommand.MoveByAnimated(0.0, KEYBOARD_PAN, KEYBOARD_ANIMATION_MS)

        GLFW_KEY_DOWN,
        GLFW_KEY_S -> CameraCommand.MoveByAnimated(0.0, -KEYBOARD_PAN, KEYBOARD_ANIMATION_MS)

        GLFW_KEY_EQUAL ->
          CameraCommand.ScaleByAnimated(KEYBOARD_ZOOM, viewportCenter(), KEYBOARD_ANIMATION_MS)

        GLFW_KEY_MINUS ->
          CameraCommand.ScaleByAnimated(
            1.0 / KEYBOARD_ZOOM,
            viewportCenter(),
            KEYBOARD_ANIMATION_MS,
          )

        GLFW_KEY_Q -> CameraCommand.AdjustBearingAnimated(-KEYBOARD_BEARING, KEYBOARD_ANIMATION_MS)

        GLFW_KEY_E -> CameraCommand.AdjustBearingAnimated(KEYBOARD_BEARING, KEYBOARD_ANIMATION_MS)
        GLFW_KEY_RIGHT_BRACKET ->
          CameraCommand.AdjustPitchAnimated(KEYBOARD_PITCH, KEYBOARD_ANIMATION_MS)

        GLFW_KEY_LEFT_BRACKET ->
          CameraCommand.AdjustPitchAnimated(-KEYBOARD_PITCH, KEYBOARD_ANIMATION_MS)

        GLFW_KEY_0 -> CameraCommand.ResetOrientation(RESET_ANIMATION_MS)
        else -> null
      }
    if (command != null) {
      commands.push(command)
      renderRequest.set()
    }
  }

  private fun viewportCenter(): ScreenPoint {
    val current = viewport()
    return ScreenPoint(current.width() / 2.0, current.height() / 2.0)
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
    private const val KEYBOARD_ANIMATION_MS = 160.0
    private const val RESET_ANIMATION_MS = 220.0

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
  }
}
