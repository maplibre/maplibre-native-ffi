package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.glfw.GLFW.glfwPollEvents
import org.lwjgl.glfw.GLFW.glfwSetFramebufferSizeCallback
import org.lwjgl.glfw.GLFW.glfwSetWindowContentScaleCallback
import org.lwjgl.glfw.GLFW.glfwSetWindowSizeCallback
import org.lwjgl.glfw.GLFW.glfwWaitEventsTimeout
import org.lwjgl.glfw.GLFW.glfwWindowShouldClose
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.RenderBackend

/**
 * The two loops the example runs on two native threads.
 *
 * GLFW requires window creation and event polling on the process main thread, so the main thread is
 * the render loop: it owns the window, input decoding, the graphics context, and the render session
 * it attaches. The spawned thread is the runtime loop: it owns the runtime and the map for their
 * whole lifetime.
 */
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

      val commands = CommandQueue()
      val renderRequest = RenderRequest()
      val channel = MapChannel()
      // The runtime loop sizes the map from this snapshot; every later viewport change reaches
      // native through the render session, which this thread owns.
      val runtimeThread =
        Thread(
          { runtimeLoop(initialViewport, commands, renderRequest, channel) },
          "maplibre-runtime",
        )
      runtimeThread.start()

      try {
        renderLoop(graphics, mode, viewport, commands, renderRequest, channel)
      } finally {
        // The render loop has closed its session by now; a map with an attached session cannot be
        // destroyed, so only then may the runtime loop tear down.
        channel.requestShutdown()
        runtimeThread.join()
      }
      channel.failure()?.let { throw it }
    }
  }

  /**
   * Owns the runtime and the map for their whole lifetime, on a thread that is not the one
   * presenting. It never touches the render session: the render loop attaches its own against the
   * map published here.
   */
  private fun runtimeLoop(
    viewport: Viewport,
    commands: CommandQueue,
    renderRequest: RenderRequest,
    channel: MapChannel,
  ) {
    try {
      MapState.create(viewport).use { state ->
        state.acquireWakeSource().use { wake ->
          channel.publish(state.map, wake)
          commands.onEnqueue = { channel.wakeRuntimeLoop() }
          while (!channel.shutdownRequested()) {
            // After a failure this loop idles instead of tearing down, because the render loop
            // still
            // holds a session attached to the map.
            if (channel.failure() == null) {
              try {
                state.step(commands, renderRequest)
              } catch (error: Throwable) {
                channel.fail(error)
              }
            }
          }
        }
      }
    } catch (error: Throwable) {
      channel.fail(error)
    }
  }

  /** The display-paced render loop. Owns the window, input, and the render session it attaches. */
  private fun renderLoop(
    graphics: GraphicsContext,
    mode: RenderTargetMode,
    viewport: ViewportHolder,
    commands: CommandQueue,
    renderRequest: RenderRequest,
    channel: MapChannel,
  ) {
    // The runtime loop creates the map; this loop attaches its own session against it and owns that
    // session for the rest of the run.
    val map = awaitMap(channel) ?: return
    val target = RenderTarget.attach(graphics, map, viewport.value, mode)
    try {
      InputController(graphics.window(), commands, renderRequest) { viewport.value }
        .use {
          println("render target: ${mode.cliName()}")
          println("render target status: ${mode.status()}")
          InputController.printControls()
          installResizeCallbacks(graphics.window(), viewport)
          // TODO(map-example-spec): Replace poll-and-wait with a display-paced host loop. See Frame
          // loop.
          while (!glfwWindowShouldClose(graphics.window()) && channel.failure() == null) {
            glfwPollEvents()
            if (viewport.consumeChanged()) {
              viewport.value.log("resized viewport")
              if (!viewport.value.empty()) {
                graphics.resize(viewport.value)
                if (target.needsReattachOnResize()) {
                  target.reattach(viewport.value)
                } else {
                  target.resize(viewport.value)
                }
                renderRequest.set()
              }
            }
            if (viewport.value.empty()) {
              glfwWaitEventsTimeout(IDLE_WAIT_SECONDS)
              continue
            }
            // Consume before rendering, so a request the runtime loop publishes during the render
            // call is not discarded. The map applies a new logical size on the runtime loop's next
            // pump, so no update until then is expected rather than a failure.
            var rendered = false
            if (renderRequest.consume()) {
              rendered = render(target)
              if (!rendered) {
                renderRequest.set()
              }
            }
            if (!rendered) {
              glfwWaitEventsTimeout(IDLE_WAIT_SECONDS)
            }
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

  /** Waits for the runtime loop to publish its map, or returns null when it failed first. */
  private fun awaitMap(channel: MapChannel): MapHandle? {
    while (channel.failure() == null) {
      channel.mapHandle()?.let {
        return it
      }
      Thread.sleep(1)
    }
    return null
  }

  private fun installResizeCallbacks(window: Long, viewport: ViewportHolder) {
    glfwSetWindowSizeCallback(window) { _, _, _ -> viewport.update(window) }
    glfwSetFramebufferSizeCallback(window) { _, _, _ -> viewport.update(window) }
    glfwSetWindowContentScaleCallback(window) { _, _, _ -> viewport.update(window) }
  }

  /** Render loop state: GLFW delivers every resize callback on the thread that polls. */
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
