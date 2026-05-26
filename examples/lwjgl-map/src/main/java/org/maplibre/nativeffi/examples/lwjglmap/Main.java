package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.glfwPollEvents;
import static org.lwjgl.glfw.GLFW.glfwSetFramebufferSizeCallback;
import static org.lwjgl.glfw.GLFW.glfwSetWindowContentScaleCallback;
import static org.lwjgl.glfw.GLFW.glfwSetWindowSizeCallback;
import static org.lwjgl.glfw.GLFW.glfwWindowShouldClose;

import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.render.RenderBackend;

public final class Main {
  private Main() {}

  public static void main(String[] args) throws Exception {
    var mode = parseArgs(args);
    if (!Maplibre.supportedRenderBackends().contains(RenderBackend.VULKAN)) {
      throw new IllegalStateException("The loaded MapLibre native library does not support Vulkan");
    }
    System.out.println("lwjgl-map render target: " + mode.cliName());
    System.out.println("render target status: " + mode.status());
    var propertyPath = System.getProperty("org.maplibre.nativeffi.library.path");
    if (propertyPath != null) {
      System.out.println("MapLibre native library: " + propertyPath);
    }

    try (var vulkan = VulkanContext.create("MapLibre LWJGL Map", 1280, 720)) {
      var viewport = new ViewportHolder(Viewport.read(vulkan.window()));
      try (var mapState = MapState.create(vulkan, viewport.value, mode);
          var input = new InputController(vulkan.window(), mapState.map())) {
        InputController.printControls();
        installResizeCallbacks(vulkan.window(), viewport);
        while (!glfwWindowShouldClose(vulkan.window())) {
          glfwPollEvents();
          if (viewport.consumeChanged()) {
            mapState.resize(viewport.value);
          }
          var rendered = mapState.step();
          if (!rendered) {
            Thread.sleep(4);
          }
        }
      }
    }
  }

  private static RenderTargetMode parseArgs(String[] args) {
    var mode = RenderTargetMode.OWNED_TEXTURE;
    for (var arg : args) {
      if (arg.startsWith("--render-target=")) {
        mode = RenderTargetMode.parse(arg.substring("--render-target=".length()));
      } else if (!arg.startsWith("-")) {
        mode = RenderTargetMode.parse(arg);
      } else {
        throw new IllegalArgumentException("unknown argument: " + arg);
      }
    }
    return mode;
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
