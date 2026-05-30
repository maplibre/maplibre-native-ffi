package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.glfwPollEvents;
import static org.lwjgl.glfw.GLFW.glfwSetFramebufferSizeCallback;
import static org.lwjgl.glfw.GLFW.glfwSetWindowContentScaleCallback;
import static org.lwjgl.glfw.GLFW.glfwSetWindowSizeCallback;
import static org.lwjgl.glfw.GLFW.glfwWindowShouldClose;

import java.util.concurrent.atomic.AtomicBoolean;
import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.render.RenderBackend;

public final class Main {
  private static final int INITIAL_WIDTH = 960;
  private static final int INITIAL_HEIGHT = 640;

  private Main() {}

  public static void main(String[] args) throws Exception {
    var mode = parseArgs(args);
    if (mode == null) {
      return;
    }
    var backends = Maplibre.supportedRenderBackends();
    System.out.println("native render backends: " + backends);
    if (!supportsUsableBackend(backends)) {
      throw new IllegalStateException(
          "The loaded MapLibre native library does not support a backend usable by lwjgl-map on"
              + " this platform");
    }
    var logCallbackInstalled = new AtomicBoolean(true);
    Runtime.getRuntime()
        .addShutdownHook(
            new Thread(
                () -> {
                  if (logCallbackInstalled.getAndSet(false)) {
                    Maplibre.clearLogCallback();
                  }
                }));
    Maplibre.setLogCallback(
        record -> {
          System.err.printf(
              "MapLibre %s %s %d: %s%n",
              record.severity(), record.event(), record.code(), record.message());
          return true;
        });
    var propertyPath = System.getProperty("org.maplibre.nativeffi.library.path");
    if (propertyPath != null) {
      System.out.println("MapLibre native library: " + propertyPath);
    }

    try (var clearLogCallback =
            (AutoCloseable)
                () -> {
                  if (logCallbackInstalled.getAndSet(false)) {
                    Maplibre.clearLogCallback();
                  }
                };
        var graphics =
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
        while (!glfwWindowShouldClose(graphics.window())) {
          glfwPollEvents();
          if (viewport.consumeChanged()) {
            viewport.value.log("resized viewport");
            graphics.resize(viewport.value);
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
    if (args.length == 1 && args[0].equals("--help")) {
      printUsage();
      return null;
    }
    if (args.length != 1 || args[0].startsWith("-")) {
      printUsage();
      System.exit(1);
    }
    try {
      return RenderTargetMode.parse(args[0]);
    } catch (IllegalArgumentException error) {
      System.err.println(error.getMessage());
      printUsage();
      System.exit(1);
      throw error;
    }
  }

  private static void printUsage() {
    System.err.println(
        """
        Usage: lwjgl-map <mode>

        Modes:
          owned-texture     session-owned texture render target
          borrowed-texture  caller-owned texture render target
          native-surface    native surface render target
        """);
  }

  private static boolean supportsUsableBackend(java.util.Set<RenderBackend> backends) {
    // TODO: Add OpenGL/EGL and Windows WGL host backends once they can be tested on suitable
    // Linux/Windows machines.
    return (GraphicsContext.isMac() && backends.contains(RenderBackend.METAL))
        || backends.contains(RenderBackend.VULKAN);
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
