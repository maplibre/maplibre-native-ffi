package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.GLFW_KEY_0;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_A;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_D;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_DOWN;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_E;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_EQUAL;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_LEFT;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_LEFT_BRACKET;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_MINUS;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_PAGE_DOWN;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_PAGE_UP;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_Q;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_RIGHT;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_RIGHT_BRACKET;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_S;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_UP;
import static org.lwjgl.glfw.GLFW.GLFW_KEY_W;
import static org.lwjgl.glfw.GLFW.GLFW_MOD_CONTROL;
import static org.lwjgl.glfw.GLFW.GLFW_MOUSE_BUTTON_LEFT;
import static org.lwjgl.glfw.GLFW.GLFW_MOUSE_BUTTON_RIGHT;
import static org.lwjgl.glfw.GLFW.GLFW_PRESS;
import static org.lwjgl.glfw.GLFW.GLFW_RELEASE;
import static org.lwjgl.glfw.GLFW.GLFW_REPEAT;
import static org.lwjgl.glfw.GLFW.glfwSetCursorPosCallback;
import static org.lwjgl.glfw.GLFW.glfwSetKeyCallback;
import static org.lwjgl.glfw.GLFW.glfwSetMouseButtonCallback;
import static org.lwjgl.glfw.GLFW.glfwSetScrollCallback;

import org.maplibre.nativeffi.camera.AnimationOptions;
import org.maplibre.nativeffi.camera.CameraOptions;
import org.maplibre.nativeffi.geo.ScreenPoint;
import org.maplibre.nativeffi.map.MapHandle;

final class InputController implements AutoCloseable {
  private static final double DRAG_ROTATE_FACTOR = 0.5;
  private static final double DRAG_PITCH_FACTOR = 0.5;
  private static final double KEYBOARD_PAN = 120.0;
  private static final double KEYBOARD_ZOOM = 1.25;
  private static final double KEYBOARD_BEARING = 10.0;
  private static final double KEYBOARD_PITCH = 5.0;
  private static final AnimationOptions KEYBOARD_ANIMATION = new AnimationOptions().durationMs(160);
  private static final AnimationOptions RESET_ANIMATION = new AnimationOptions().durationMs(220);

  private final long window;
  private final MapHandle map;
  private boolean leftDown;
  private boolean rightDown;
  private boolean ctrlDown;
  private double lastX;
  private double lastY;
  private double cursorX;
  private double cursorY;

  InputController(long window, MapHandle map) {
    this.window = window;
    this.map = map;
    installCallbacks();
  }

  static void printControls() {
    System.out.println("Controls:");
    System.out.println("  left drag: pan");
    System.out.println("  right drag or Ctrl+left drag: rotate with X, pitch with Y");
    System.out.println("  scroll: zoom at cursor");
    System.out.println("  arrows or WASD: pan");
    System.out.println("  + / -: zoom at center");
    System.out.println("  Q / E: rotate");
    System.out.println("  PageUp / PageDown or [ / ]: pitch");
    System.out.println("  0: reset pitch and bearing");
  }

  private void installCallbacks() {
    glfwSetCursorPosCallback(window, (ignored, x, y) -> onCursor(x, y));
    glfwSetMouseButtonCallback(
        window, (ignored, button, action, mods) -> onMouse(button, action, mods));
    glfwSetScrollCallback(window, (ignored, xOffset, yOffset) -> onScroll(yOffset));
    glfwSetKeyCallback(window, (ignored, key, scancode, action, mods) -> onKey(key, action, mods));
  }

  private void onCursor(double x, double y) {
    cursorX = x;
    cursorY = y;
    var dx = x - lastX;
    var dy = y - lastY;
    lastX = x;
    lastY = y;
    if (rightDown || (leftDown && ctrlDown)) {
      setBearing(currentBearing() + dx * DRAG_ROTATE_FACTOR, false);
      setPitch(currentPitch() - dy * DRAG_PITCH_FACTOR, false);
    } else if (leftDown) {
      map.moveBy(dx, dy);
    }
  }

  private void onMouse(int button, int action, int mods) {
    ctrlDown = (mods & GLFW_MOD_CONTROL) != 0;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      leftDown = action == GLFW_PRESS ? true : action == GLFW_RELEASE ? false : leftDown;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      rightDown = action == GLFW_PRESS ? true : action == GLFW_RELEASE ? false : rightDown;
    }
    if (action == GLFW_PRESS) {
      map.cancelTransitions();
    }
  }

  private void onScroll(double yOffset) {
    // GLFW reports OS-adjusted scroll deltas; use them directly so trackpads with natural
    // scrolling behave like the host platform expects.
    var scale = Math.pow(2.0, yOffset * 0.25);
    map.scaleBy(scale, new ScreenPoint(cursorX, cursorY));
  }

  private void onKey(int key, int action, int mods) {
    ctrlDown = (mods & GLFW_MOD_CONTROL) != 0;
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
      return;
    }
    switch (key) {
      case GLFW_KEY_LEFT, GLFW_KEY_A -> map.moveByAnimated(KEYBOARD_PAN, 0.0, KEYBOARD_ANIMATION);
      case GLFW_KEY_RIGHT, GLFW_KEY_D -> map.moveByAnimated(-KEYBOARD_PAN, 0.0, KEYBOARD_ANIMATION);
      case GLFW_KEY_UP, GLFW_KEY_W -> map.moveByAnimated(0.0, KEYBOARD_PAN, KEYBOARD_ANIMATION);
      case GLFW_KEY_DOWN, GLFW_KEY_S -> map.moveByAnimated(0.0, -KEYBOARD_PAN, KEYBOARD_ANIMATION);
      case GLFW_KEY_EQUAL -> map.scaleByAnimated(KEYBOARD_ZOOM, KEYBOARD_ANIMATION);
      case GLFW_KEY_MINUS -> map.scaleByAnimated(1.0 / KEYBOARD_ZOOM, KEYBOARD_ANIMATION);
      case GLFW_KEY_Q -> setBearing(currentBearing() - KEYBOARD_BEARING, true);
      case GLFW_KEY_E -> setBearing(currentBearing() + KEYBOARD_BEARING, true);
      case GLFW_KEY_PAGE_UP, GLFW_KEY_RIGHT_BRACKET ->
          setPitch(currentPitch() + KEYBOARD_PITCH, true);
      case GLFW_KEY_PAGE_DOWN, GLFW_KEY_LEFT_BRACKET ->
          setPitch(currentPitch() - KEYBOARD_PITCH, true);
      case GLFW_KEY_0 -> map.easeTo(new CameraOptions().bearing(0.0).pitch(0.0), RESET_ANIMATION);
      default -> {}
    }
  }

  private double currentBearing() {
    var camera = map.camera();
    return camera.hasBearing() ? camera.bearing() : 0.0;
  }

  private double currentPitch() {
    var camera = map.camera();
    return camera.hasPitch() ? camera.pitch() : 0.0;
  }

  private void setBearing(double bearing, boolean animated) {
    var camera = new CameraOptions().bearing(bearing);
    if (animated) {
      map.easeTo(camera, KEYBOARD_ANIMATION);
    } else {
      map.jumpTo(camera);
    }
  }

  private void setPitch(double pitch, boolean animated) {
    var clamped = Math.max(0.0, Math.min(60.0, pitch));
    var camera = new CameraOptions().pitch(clamped);
    if (animated) {
      map.easeTo(camera, KEYBOARD_ANIMATION);
    } else {
      map.jumpTo(camera);
    }
  }

  @Override
  public void close() {
    glfwSetCursorPosCallback(window, null);
    glfwSetMouseButtonCallback(window, null);
    glfwSetScrollCallback(window, null);
    glfwSetKeyCallback(window, null);
  }
}
