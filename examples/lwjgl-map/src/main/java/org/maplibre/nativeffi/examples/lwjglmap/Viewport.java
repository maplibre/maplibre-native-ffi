package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.glfwGetFramebufferSize;
import static org.lwjgl.glfw.GLFW.glfwGetWindowContentScale;
import static org.lwjgl.glfw.GLFW.glfwGetWindowSize;

import org.lwjgl.system.MemoryStack;

record Viewport(
    int width, int height, double scaleFactor, int framebufferWidth, int framebufferHeight) {
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

      var logicalWidth = Math.max(1, windowWidth.get(0));
      var logicalHeight = Math.max(1, windowHeight.get(0));
      var physicalWidth = Math.max(1, framebufferWidth.get(0));
      var physicalHeight = Math.max(1, framebufferHeight.get(0));
      double scale = Math.max(xScale.get(0), yScale.get(0));
      if (!(scale > 0.0)) {
        scale =
            Math.max(
                (double) physicalWidth / logicalWidth, (double) physicalHeight / logicalHeight);
      }
      return new Viewport(logicalWidth, logicalHeight, scale, physicalWidth, physicalHeight);
    }
  }
}
