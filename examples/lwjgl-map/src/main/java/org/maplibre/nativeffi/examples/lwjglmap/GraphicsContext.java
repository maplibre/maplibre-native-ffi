package org.maplibre.nativeffi.examples.lwjglmap;

import java.util.Set;
import org.maplibre.nativeffi.render.RenderBackend;

interface GraphicsContext extends AutoCloseable {
  static GraphicsContext create(String title, int width, int height, Set<RenderBackend> backends) {
    if (isMac() && backends.contains(RenderBackend.METAL)) {
      return MetalContext.create(title, width, height);
    }
    if (!isMac() && backends.contains(RenderBackend.OPENGL)) {
      return OpenGLContext.create(title, width, height);
    }
    if (backends.contains(RenderBackend.VULKAN)) {
      return VulkanContext.create(title, width, height);
    }
    throw new IllegalStateException(
        "The loaded MapLibre native library does not support a backend usable by lwjgl-map on this"
            + " platform");
  }

  long window();

  RenderBackend backend();

  default void resize(Viewport viewport) {}

  static boolean isMac() {
    return System.getProperty("os.name").toLowerCase(java.util.Locale.ROOT).contains("mac");
  }

  @Override
  void close();
}
