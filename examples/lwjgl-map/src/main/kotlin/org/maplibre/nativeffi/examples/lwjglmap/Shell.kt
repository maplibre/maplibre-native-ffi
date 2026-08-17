package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.glfw.GLFW.glfwPollEvents
import org.lwjgl.glfw.GLFW.glfwPostEmptyEvent
import org.lwjgl.glfw.GLFW.glfwSetFramebufferSizeCallback
import org.lwjgl.glfw.GLFW.glfwSetWindowContentScaleCallback
import org.lwjgl.glfw.GLFW.glfwSetWindowSizeCallback
import org.lwjgl.glfw.GLFW.glfwWaitEventsTimeout
import org.lwjgl.glfw.GLFW.glfwWindowShouldClose
import org.maplibre.nativeffi.render.RenderBackend

/** The GLFW-thread shell that owns the window, graphics context, and render session. */
internal object Shell {
  private const val INITIAL_WIDTH = 960
  private const val INITIAL_HEIGHT = 640
  private const val IDLE_WAIT_SECONDS = 0.004

  fun run(mode: RenderTargetMode, backends: Set<RenderBackend>) {
    GraphicsContext.create("MapLibre LWJGL Map", INITIAL_WIDTH, INITIAL_HEIGHT, backends).use {
      graphics ->
      val initialViewport = Viewport.read(graphics.window())
      val viewport = ViewportHolder(initialViewport)
      initialViewport.log("initial viewport")
      val renderRequest = RenderRequest()

      MapState.create(initialViewport, ::glfwPostEmptyEvent).use { state ->
        renderLoop(graphics, mode, viewport, state, renderRequest)
      }
    }
  }

  private fun renderLoop(
    graphics: GraphicsContext,
    mode: RenderTargetMode,
    viewport: ViewportHolder,
    state: MapState,
    renderRequest: RenderRequest,
  ) {
    val target = RenderTarget.attach(graphics, state.map, viewport.value, mode)
    try {
      InputController(graphics.window(), state, renderRequest) { viewport.value }
        .use {
          println("render target: ${mode.cliName()}")
          println("render target status: ${mode.status()}")
          InputController.printControls()
          installResizeCallbacks(graphics.window(), viewport)
          while (!glfwWindowShouldClose(graphics.window())) {
            glfwPollEvents()
            state.drainNotifications(renderRequest)
            if (viewport.consumeChanged()) {
              viewport.value.log("resized viewport")
              if (!viewport.value.empty()) {
                graphics.resize(viewport.value)
                state.resize(viewport.value)
                target.resize(viewport.value)
                renderRequest.set()
              }
            }
            if (viewport.value.empty()) {
              glfwWaitEventsTimeout(IDLE_WAIT_SECONDS)
              continue
            }
            var rendered = false
            if (renderRequest.consume()) {
              rendered = render(target)
              if (!rendered) renderRequest.set()
            }
            if (!rendered) glfwWaitEventsTimeout(IDLE_WAIT_SECONDS)
          }
        }
    } finally {
      target.close()
    }
  }

  private fun render(target: RenderTarget): Boolean =
    if (target.needsMetalAutoreleasePool()) {
      MacObjectiveC.autoreleasePool().use { target.renderUpdate() }
    } else {
      target.renderUpdate()
    }

  private fun installResizeCallbacks(window: Long, viewport: ViewportHolder) {
    glfwSetWindowSizeCallback(window) { _, _, _ -> viewport.update(window) }
    glfwSetFramebufferSizeCallback(window) { _, _, _ -> viewport.update(window) }
    glfwSetWindowContentScaleCallback(window) { _, _, _ -> viewport.update(window) }
  }

  /** GLFW delivers every resize callback on the thread that polls. */
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
