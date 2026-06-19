package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.glfwPollEvents;
import static org.lwjgl.glfw.GLFW.glfwSetFramebufferSizeCallback;
import static org.lwjgl.glfw.GLFW.glfwSetWindowContentScaleCallback;
import static org.lwjgl.glfw.GLFW.glfwSetWindowSizeCallback;
import static org.lwjgl.glfw.GLFW.glfwWaitEventsTimeout;
import static org.lwjgl.glfw.GLFW.glfwWindowShouldClose;

import java.util.Set;
import org.maplibre.nativeffi.render.RenderBackend;

final class Shell {
  private static final int INITIAL_WIDTH = 960;
  private static final int INITIAL_HEIGHT = 640;
  private static final double IDLE_WAIT_SECONDS = 0.004;

  private Shell() {}

  static void run(RenderTargetMode mode, Set<RenderBackend> backends) throws Exception {
    try (var graphics =
        GraphicsContext.create("MapLibre LWJGL Map", INITIAL_WIDTH, INITIAL_HEIGHT, backends)) {
      var viewport = new ViewportHolder(Viewport.read(graphics.window()));
      viewport.value.log("initial viewport");
      try (var mapState = MapState.create(graphics, viewport.value, mode);
          var input =
              new InputController(graphics.window(), mapState.map(), mapState::requestRender)) {
        System.out.println("render target: " + mode.cliName());
        System.out.println("render target status: " + mode.status());
        InputController.printControls();
        installResizeCallbacks(graphics.window(), viewport);
        // TODO(map-example-spec): Replace poll-and-wait with a display-paced host loop. See Frame
        // loop.
        while (!glfwWindowShouldClose(graphics.window())) {
          glfwPollEvents();
          if (viewport.consumeChanged()) {
            viewport.value.log("resized viewport");
            if (!viewport.value.empty()) {
              graphics.resize(viewport.value);
              mapState.resize(viewport.value);
            }
          }
          if (viewport.value.empty()) {
            glfwWaitEventsTimeout(IDLE_WAIT_SECONDS);
            continue;
          }
          var rendered = mapState.step();
          if (!rendered) {
            glfwWaitEventsTimeout(IDLE_WAIT_SECONDS);
          }
        }
      }
    }
  }

  private static void installResizeCallbacks(long window, ViewportHolder viewport) {
    glfwSetWindowSizeCallback(window, (ignored, width, height) -> viewport.update(window));
    glfwSetFramebufferSizeCallback(window, (ignored, width, height) -> viewport.update(window));
    glfwSetWindowContentScaleCallback(window, (ignored, xScale, yScale) -> viewport.update(window));
  }

  private static final class ViewportHolder {
    private Viewport value;
    private boolean changed;

    ViewportHolder(Viewport value) {
      this.value = value;
    }

    void update(long window) {
      var next = Viewport.read(window);
      if (!next.equals(value)) {
        value = next;
        changed = true;
      }
    }

    boolean consumeChanged() {
      var result = changed;
      changed = false;
      return result;
    }
  }
}
