package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.glfw.GLFW.glfwPollEvents
import org.lwjgl.glfw.GLFW.glfwSetFramebufferSizeCallback
import org.lwjgl.glfw.GLFW.glfwSetWindowContentScaleCallback
import org.lwjgl.glfw.GLFW.glfwSetWindowSizeCallback
import org.lwjgl.glfw.GLFW.glfwWaitEventsTimeout
import org.lwjgl.glfw.GLFW.glfwWindowShouldClose
import org.maplibre.nativeffi.render.RenderBackend

internal object Shell {
  private const val INITIAL_WIDTH = 960
  private const val INITIAL_HEIGHT = 640
  private const val IDLE_WAIT_SECONDS = 0.004

  fun run(mode: RenderTargetMode, backends: Set<RenderBackend>) {
    GraphicsContext.create("MapLibre LWJGL Map", INITIAL_WIDTH, INITIAL_HEIGHT, backends).use {
      graphics ->
      val viewport = ViewportHolder(Viewport.read(graphics.window()))
      viewport.value.log("initial viewport")
      MapState.create(graphics, viewport.value, mode).use { mapState ->
        InputController(graphics.window(), mapState.map, mapState::requestRender).use {
          println("render target: ${mode.cliName()}")
          println("render target status: ${mode.status()}")
          InputController.printControls()
          installResizeCallbacks(graphics.window(), viewport)
          // TODO(map-example-spec): Replace poll-and-wait with a display-paced host loop. See Frame
          // loop.
          while (!glfwWindowShouldClose(graphics.window())) {
            glfwPollEvents()
            if (viewport.consumeChanged()) {
              viewport.value.log("resized viewport")
              if (!viewport.value.empty()) {
                graphics.resize(viewport.value)
                mapState.resize(viewport.value)
              }
            }
            if (viewport.value.empty()) {
              glfwWaitEventsTimeout(IDLE_WAIT_SECONDS)
              continue
            }
            val rendered = mapState.step()
            if (!rendered) {
              glfwWaitEventsTimeout(IDLE_WAIT_SECONDS)
            }
          }
        }
      }
    }
  }

  private fun installResizeCallbacks(window: Long, viewport: ViewportHolder) {
    glfwSetWindowSizeCallback(window) { _, _, _ -> viewport.update(window) }
    glfwSetFramebufferSizeCallback(window) { _, _, _ -> viewport.update(window) }
    glfwSetWindowContentScaleCallback(window) { _, _, _ -> viewport.update(window) }
  }

  private class ViewportHolder(var value: Viewport) {
    private var changed = false

    fun update(window: Long) {
      val next = Viewport.read(window)
      if (next != value) {
        value = next
        changed = true
      }
    }

    fun consumeChanged(): Boolean {
      val result = changed
      changed = false
      return result
    }
  }
}
