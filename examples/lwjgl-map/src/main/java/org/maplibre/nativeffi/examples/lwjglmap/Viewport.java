package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.glfwGetFramebufferSize;
import static org.lwjgl.glfw.GLFW.glfwGetWindowContentScale;
import static org.lwjgl.glfw.GLFW.glfwGetWindowSize;

import org.lwjgl.system.MemoryStack;

record Viewport(
    int width,
    int height,
    double scaleFactor,
    int framebufferWidth,
    int framebufferHeight,
    boolean empty) {
  static Viewport read(long window) {
    try (var stack = MemoryStack.stackPush()) {
      var windowWidth = stack.mallocInt(1);
      var windowHeight = stack.mallocInt(1);
      var framebufferWidth = stack.mallocInt(1);
      var framebufferHeight = stack.mallocInt(1);
      var xScale = stack.mallocFloat(1);
      var yScale = stack.mallocFloat(1);
      glfwGetWindowSize(window, windowWidth, windowHeight);
      glfwGetFramebufferSize(window, framebufferWidth, framebufferHeight);
      glfwGetWindowContentScale(window, xScale, yScale);

      var rawLogicalWidth = windowWidth.get(0);
      var rawLogicalHeight = windowHeight.get(0);
      var rawPhysicalWidth = framebufferWidth.get(0);
      var rawPhysicalHeight = framebufferHeight.get(0);
      var empty =
          rawLogicalWidth <= 0
              || rawLogicalHeight <= 0
              || rawPhysicalWidth <= 0
              || rawPhysicalHeight <= 0;

      var logicalWidth = Math.max(1, rawLogicalWidth);
      var logicalHeight = Math.max(1, rawLogicalHeight);
      var physicalWidth = Math.max(1, rawPhysicalWidth);
      var physicalHeight = Math.max(1, rawPhysicalHeight);
      var scale =
          Math.max((double) physicalWidth / logicalWidth, (double) physicalHeight / logicalHeight);
      if (!(scale > 0.0) || !Double.isFinite(scale)) {
        scale = Math.max(xScale.get(0), yScale.get(0));
      }
      if (!(scale > 0.0) || !Double.isFinite(scale)) {
        scale = 1.0;
      }
      return new Viewport(logicalWidth, logicalHeight, scale, physicalWidth, physicalHeight, empty);
    }
  }

  void log(String label) {
    System.out.printf(
        "%s: logical=%dx%d physical=%dx%d scale=%.2f%s%n",
        label,
        width,
        height,
        framebufferWidth,
        framebufferHeight,
        scaleFactor,
        empty ? " empty=true" : "");
  }
}
