package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.glfw.GLFW.glfwPostEmptyEvent
import org.lwjgl.glfw.GLFW.glfwSetFramebufferSizeCallback
import org.lwjgl.glfw.GLFW.glfwSetWindowContentScaleCallback
import org.lwjgl.glfw.GLFW.glfwSetWindowSizeCallback
import org.lwjgl.glfw.GLFW.glfwWaitEvents
import org.lwjgl.glfw.GLFW.glfwWaitEventsTimeout
import org.lwjgl.glfw.GLFW.glfwWindowShouldClose
import org.maplibre.nativeffi.generated.RenderBackendFlag

/**
 * The GLFW-thread shell that owns the window, graphics context, and render session.
 *
 * The thread sleeps in `glfwWaitEvents` until input arrives or a native wake posts an empty event.
 * Input becomes map commands, a map update becomes a frame demand, and each wake has the thread
 * drain events, service driver work, or drain frame results.
 */
internal object Shell {
  private const val INITIAL_WIDTH = 960
  private const val INITIAL_HEIGHT = 640

  /** How long the smoke check waits for its first frame before it fails. */
  private const val SMOKE_TIMEOUT_NANOS = 60_000_000_000L

  /**
   * Runs the example until its window closes. A smoke run instead renders an inline style in a
   * hidden window, so it needs neither the network nor a user, and returns after the first frame
   * that reaches the window.
   */
  fun run(mode: RenderTargetMode, backends: RenderBackendFlag, smoke: Boolean = false) {
    GraphicsContext.create(
        "MapLibre LWJGL Map",
        INITIAL_WIDTH,
        INITIAL_HEIGHT,
        backends,
        visible = !smoke,
      )
      .use { graphics ->
        val viewport = ViewportHolder(Viewport.read(graphics.window()))
        viewport.value.log("initial viewport")
        val wakes = LoopWakes()
        val style = if (smoke) MapState.SMOKE_STYLE else null
        MapState.create(viewport.value, wakes.events.wake, style).use { state ->
          RenderTarget.attach(graphics, state.map, viewport.value, mode, wakes).use { target ->
            InputController(graphics.window(), state) { viewport.value }
              .use {
                println("render target: ${mode.cliName()}")
                println("render target status: ${mode.status()}")
                InputController.printControls()
                installResizeCallbacks(graphics.window(), viewport)
                renderLoop(graphics, viewport, state, target, wakes, smoke)
              }
          }
        }
      }
  }

  private fun renderLoop(
    graphics: GraphicsContext,
    viewport: ViewportHolder,
    state: MapState,
    target: RenderTarget,
    wakes: LoopWakes,
    smoke: Boolean,
  ) {
    val smokeDeadline = System.nanoTime() + SMOKE_TIMEOUT_NANOS
    // The session attached after the map took its style and camera, so it starts with one frame.
    target.requestFrame()
    while (!glfwWindowShouldClose(graphics.window())) {
      if (smoke) {
        val remaining = smokeDeadline - System.nanoTime()
        check(remaining > 0) { "smoke: no frame reached the window" }
        glfwWaitEventsTimeout(remaining / 1e9)
      } else {
        glfwWaitEvents()
      }
      val presented =
        withAutoreleasePool(target) {
          if (viewport.consumeChanged()) {
            viewport.value.log("resized viewport")
            if (!viewport.value.empty()) {
              graphics.resize(viewport.value)
              target.resize(viewport.value)
            }
          }
          if (wakes.events.consume() && state.drainRenderUpdates() && !viewport.value.empty()) {
            target.requestFrame()
          }
          if (wakes.driverWork.consume()) target.serviceDriverWork()
          wakes.frames.consume() && target.drainFrameResults()
        }
      if (smoke && presented) {
        println("smoke: rendered a frame")
        return
      }
    }
  }

  private fun <T> withAutoreleasePool(target: RenderTarget, action: () -> T): T =
    if (target.needsMetalAutoreleasePool()) {
      MacObjectiveC.autoreleasePool().use { action() }
    } else {
      action()
    }

  private fun installResizeCallbacks(window: Long, viewport: ViewportHolder) {
    glfwSetWindowSizeCallback(window) { _, _, _ -> viewport.update(window) }
    glfwSetFramebufferSizeCallback(window) { _, _, _ -> viewport.update(window) }
    glfwSetWindowContentScaleCallback(window) { _, _, _ -> viewport.update(window) }
  }

  /**
   * GLFW delivers every resize callback on the thread that waits for events. A Cocoa resize arrives
   * as a notification rather than an event, so the callback posts one to end the wait.
   */
  private class ViewportHolder(var value: Viewport) {
    private var changed = false

    fun update(window: Long) {
      val next = Viewport.read(window)
      if (next != value) {
        value = next
        changed = true
        glfwPostEmptyEvent()
      }
    }

    fun consumeChanged(): Boolean {
      val result = changed
      changed = false
      return result
    }
  }
}
