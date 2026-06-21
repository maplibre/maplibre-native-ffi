package org.maplibre.nativeffi.examples.lwjglmap

import kotlin.math.ceil
import kotlin.math.max
import org.lwjgl.glfw.GLFW.glfwGetFramebufferSize
import org.lwjgl.glfw.GLFW.glfwGetWindowContentScale
import org.lwjgl.glfw.GLFW.glfwGetWindowSize
import org.lwjgl.system.MemoryStack

internal class Viewport(
  private val width: Int,
  private val height: Int,
  private val scaleFactor: Double,
  private val framebufferWidth: Int,
  private val framebufferHeight: Int,
  private val empty: Boolean,
) {
  fun width(): Int = width

  fun height(): Int = height

  fun scaleFactor(): Double = scaleFactor

  fun framebufferWidth(): Int = framebufferWidth

  fun framebufferHeight(): Int = framebufferHeight

  fun empty(): Boolean = empty

  fun log(label: String) {
    System.out.printf(
      "%s: logical=%dx%d physical=%dx%d scale=%.2f%s%n",
      label,
      width,
      height,
      framebufferWidth,
      framebufferHeight,
      scaleFactor,
      if (empty) " empty=true" else "",
    )
  }

  companion object {
    @JvmStatic
    fun read(window: Long): Viewport {
      MemoryStack.stackPush().use { stack ->
        val windowWidth = stack.mallocInt(1)
        val windowHeight = stack.mallocInt(1)
        val framebufferWidth = stack.mallocInt(1)
        val framebufferHeight = stack.mallocInt(1)
        val xScale = stack.mallocFloat(1)
        val yScale = stack.mallocFloat(1)
        glfwGetWindowSize(window, windowWidth, windowHeight)
        glfwGetFramebufferSize(window, framebufferWidth, framebufferHeight)
        glfwGetWindowContentScale(window, xScale, yScale)

        val rawLogicalWidth = windowWidth[0]
        val rawLogicalHeight = windowHeight[0]
        val rawPhysicalWidth = framebufferWidth[0]
        val rawPhysicalHeight = framebufferHeight[0]
        val empty =
          rawLogicalWidth <= 0 ||
            rawLogicalHeight <= 0 ||
            rawPhysicalWidth <= 0 ||
            rawPhysicalHeight <= 0

        val physicalWidth = max(1, rawPhysicalWidth)
        val physicalHeight = max(1, rawPhysicalHeight)
        var scale = max(xScale[0].toDouble(), yScale[0].toDouble())
        if (!(scale > 0.0) || !scale.isFinite()) {
          scale =
            max(
              physicalWidth.toDouble() / max(1, rawLogicalWidth),
              physicalHeight.toDouble() / max(1, rawLogicalHeight),
            )
        }
        if (!(scale > 0.0) || !scale.isFinite()) {
          scale = 1.0
        }
        val logicalWidth = scaledLogicalSize(physicalWidth, scale)
        val logicalHeight = scaledLogicalSize(physicalHeight, scale)
        return Viewport(logicalWidth, logicalHeight, scale, physicalWidth, physicalHeight, empty)
      }
    }

    private fun scaledLogicalSize(physicalSize: Int, scaleFactor: Double): Int =
      max(1, ceil(physicalSize / scaleFactor).toInt())
  }
}
