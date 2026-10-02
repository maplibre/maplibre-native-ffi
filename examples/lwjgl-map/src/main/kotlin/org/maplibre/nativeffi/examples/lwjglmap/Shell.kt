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
 * Input becomes map commands, a map update becomes a frame demand, and the wakes have the thread
 * drain events and frame results.
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
        val events = GlfwWake()
        val style = if (smoke) MapState.SMOKE_STYLE else null
        MapState.create(viewport.value, events.wake, style).use { state ->
          InputController(graphics.window(), state) { viewport.value }
            .use {
              installResizeCallbacks(graphics.window(), viewport)
              renderLoop(graphics, viewport, state, events, mode, smoke)
            }
        }
      }
  }

  private fun renderLoop(
    graphics: GraphicsContext,
    viewport: ViewportHolder,
    state: MapState,
    events: GlfwWake,
    mode: RenderTargetMode,
    smoke: Boolean,
  ) {
    var target = attach(graphics, state, viewport.value, mode)
    // A session fixes its scale factor at attachment, so a scale change reattaches.
    var attachedScale = viewport.value.scaleFactor()
    InputController.printControls()
    val smokeDeadline = System.nanoTime() + SMOKE_TIMEOUT_NANOS
    try {
      // Wakes may have arrived while the attachment waited, so the loop handles pending work
      // before its first wait.
      while (!glfwWindowShouldClose(graphics.window())) {
        if (viewport.consumeChanged()) {
          viewport.value.log("resized viewport")
          val next = viewport.value
          if (!next.empty() && next.scaleFactor() == attachedScale) {
            target.resize(next)
          } else if (!next.empty()) {
            target.close()
            target = attach(graphics, state, next, mode)
            attachedScale = next.scaleFactor()
          }
        }
        if (events.consume() && state.drainRenderUpdates() && !viewport.value.empty()) {
          target.requestFrame()
        }
        if (target.handleWakes() && smoke) {
          println("smoke: rendered a frame")
          return
        }
        check(!smoke || System.nanoTime() < smokeDeadline) {
          "smoke: no frame reached the window within 60 seconds"
        }
        val wakeAt = listOfNotNull(target.retryAtNanos, smokeDeadline.takeIf { smoke }).minOrNull()
        if (wakeAt == null) {
          glfwWaitEvents()
        } else {
          glfwWaitEventsTimeout(maxOf(wakeAt - System.nanoTime(), 0L) / 1e9)
        }
      }
    } finally {
      target.close()
    }
  }

  /** Attaches a session, logs its mode and driver, and demands its first frame. */
  private fun attach(
    graphics: GraphicsContext,
    state: MapState,
    viewport: Viewport,
    mode: RenderTargetMode,
  ): RenderTarget {
    val target = RenderTarget.attach(graphics, state.map, viewport, mode)
    println("render target: ${mode.cliName()}")
    println("render target status: ${mode.status()}")
    println("render driver: ${target.driverLabel}")
    target.requestFrame()
    return target
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
