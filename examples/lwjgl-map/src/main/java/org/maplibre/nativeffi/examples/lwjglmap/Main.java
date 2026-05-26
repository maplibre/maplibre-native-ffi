package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.glfwPollEvents;
import static org.lwjgl.glfw.GLFW.glfwSetFramebufferSizeCallback;
import static org.lwjgl.glfw.GLFW.glfwSetWindowContentScaleCallback;
import static org.lwjgl.glfw.GLFW.glfwSetWindowSizeCallback;
import static org.lwjgl.glfw.GLFW.glfwWindowShouldClose;

import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.render.OpenGLContextProvider;
import org.maplibre.nativeffi.render.RenderBackend;

public final class Main {
  private Main() {}

  public static void main(String[] args) throws Exception {
    var options = parseArgs(args);
    var backend = options.backend() != null ? options.backend() : chooseDefaultBackend();
    var mode = options.renderTargetMode();
    ensureBackendSupported(backend);
    System.out.println("lwjgl-map backend: " + backend.cliName());
    System.out.println("lwjgl-map render target: " + mode.cliName());
    System.out.println("render target status: " + mode.status());
    var propertyPath = System.getProperty("org.maplibre.nativeffi.library.path");
    if (propertyPath != null) {
      System.out.println("MapLibre native library: " + propertyPath);
    }

    switch (backend) {
      case VULKAN -> {
        try (var vulkan = VulkanContext.create("MapLibre LWJGL Map", 1280, 720)) {
          run(vulkan.window(), MapState.create(vulkan, Viewport.read(vulkan.window()), mode));
        }
      }
      case OPENGL -> {
        try (var opengl = OpenGLContext.create("MapLibre LWJGL Map", 1280, 720)) {
          run(opengl.window(), MapState.create(opengl, Viewport.read(opengl.window()), mode));
        }
      }
    }
  }

  private static void run(long window, MapState mapState) throws Exception {
    var viewport = new ViewportHolder(Viewport.read(window));
    try (mapState;
        var input = new InputController(window, mapState.map())) {
      InputController.printControls();
      installResizeCallbacks(window, viewport);
      while (!glfwWindowShouldClose(window)) {
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

  private static Options parseArgs(String[] args) {
    var mode = RenderTargetMode.OWNED_TEXTURE;
    ExampleBackend backend = null;
    for (var arg : args) {
      if (arg.startsWith("--render-target=")) {
        mode = RenderTargetMode.parse(arg.substring("--render-target=".length()));
      } else if (arg.startsWith("--backend=")) {
        backend = ExampleBackend.parse(arg.substring("--backend=".length()));
      } else if (!arg.startsWith("-")) {
        mode = RenderTargetMode.parse(arg);
      } else {
        throw new IllegalArgumentException("unknown argument: " + arg);
      }
    }
    return new Options(backend, mode);
  }

  private static ExampleBackend chooseDefaultBackend() {
    var backends = Maplibre.supportedRenderBackends();
    if (backends.contains(RenderBackend.VULKAN)) {
      return ExampleBackend.VULKAN;
    }
    if (backends.contains(RenderBackend.OPENGL)) {
      return ExampleBackend.OPENGL;
    }
    throw new IllegalStateException(
        "The loaded MapLibre native library supports neither Vulkan nor OpenGL");
  }

  private static void ensureBackendSupported(ExampleBackend backend) {
    var backends = Maplibre.supportedRenderBackends();
    switch (backend) {
      case VULKAN -> {
        if (!backends.contains(RenderBackend.VULKAN)) {
          throw new IllegalStateException(
              "The loaded MapLibre native library does not support Vulkan");
        }
      }
      case OPENGL -> {
        if (!backends.contains(RenderBackend.OPENGL)) {
          throw new IllegalStateException(
              "The loaded MapLibre native library does not support OpenGL");
        }
        if (!Maplibre.supportedOpenGLContextProviders().contains(OpenGLContextProvider.WGL)) {
          // TODO(linux): Accept EGL here after the LWJGL example has a Linux
          // EGL context/surface path validated on a Linux machine.
          throw new IllegalStateException("The OpenGL LWJGL example currently requires WGL");
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

  private record Options(ExampleBackend backend, RenderTargetMode renderTargetMode) {}
}
