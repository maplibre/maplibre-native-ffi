package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.glfw.GLFW.GLFW_CLIENT_API
import org.lwjgl.glfw.GLFW.GLFW_NO_API
import org.lwjgl.glfw.GLFW.GLFW_RESIZABLE
import org.lwjgl.glfw.GLFW.GLFW_TRUE
import org.lwjgl.glfw.GLFW.glfwCreateWindow
import org.lwjgl.glfw.GLFW.glfwDefaultWindowHints
import org.lwjgl.glfw.GLFW.glfwDestroyWindow
import org.lwjgl.glfw.GLFW.glfwGetVersionString
import org.lwjgl.glfw.GLFW.glfwInit
import org.lwjgl.glfw.GLFW.glfwTerminate
import org.lwjgl.glfw.GLFW.glfwWindowHint
import org.lwjgl.glfw.GLFWNativeCocoa.glfwGetCocoaView
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.system.MemoryUtil.memAddress
import org.lwjgl.system.macosx.CoreFoundation
import org.maplibre.nativeffi.render.RenderBackend

internal class MetalContext
private constructor(
  private val window: Long,
  private var view: Long,
  private var device: Long,
  private var layer: Long,
) : GraphicsContext {
  private var closed = false

  override fun window(): Long = window

  override fun backend(): RenderBackend = RenderBackend.METAL

  fun deviceAddress(): Long = device

  fun layerAddress(): Long = layer

  fun createBorrowedTexture(viewport: Viewport): Long {
    var descriptor = NULL
    try {
      MacObjectiveC.autoreleasePool().use {
        descriptor = MacObjectiveC.allocInit("MTLTextureDescriptor")
        MacObjectiveC.sendVoid(descriptor, "setTextureType:", MTL_TEXTURE_TYPE_2D)
        MacObjectiveC.sendVoid(descriptor, "setPixelFormat:", MTL_PIXEL_FORMAT_RGBA8_UNORM)
        MacObjectiveC.sendVoid(descriptor, "setWidth:", viewport.framebufferWidth().toLong())
        MacObjectiveC.sendVoid(descriptor, "setHeight:", viewport.framebufferHeight().toLong())
        MacObjectiveC.sendVoid(descriptor, "setDepth:", 1L)
        MacObjectiveC.sendVoid(descriptor, "setMipmapLevelCount:", 1L)
        MacObjectiveC.sendVoid(descriptor, "setArrayLength:", 1L)
        MacObjectiveC.sendVoid(descriptor, "setSampleCount:", 1L)
        MacObjectiveC.sendVoid(
          descriptor,
          "setUsage:",
          (MTL_TEXTURE_USAGE_SHADER_READ or MTL_TEXTURE_USAGE_RENDER_TARGET).toLong(),
        )
        val texture = MacObjectiveC.sendPointer(device, "newTextureWithDescriptor:", descriptor)
        check(texture != NULL) { "Metal borrowed texture creation failed" }
        return texture
      }
    } finally {
      MacObjectiveC.release(descriptor)
    }
  }

  fun createCommandQueue(): Long {
    val queue = MacObjectiveC.sendPointer(device, "newCommandQueue")
    check(queue != NULL) { "Metal command queue creation failed" }
    return queue
  }

  fun createRenderPipeline(): Long {
    var source = NULL
    var vertexName = NULL
    var fragmentName = NULL
    var library = NULL
    var vertex = NULL
    var fragment = NULL
    var descriptor = NULL
    try {
      MacObjectiveC.autoreleasePool().use {
        MemoryStack.stackPush().use { stack ->
          source = MacObjectiveC.cfString(MetalShaders.TEXTURE_COMPOSITOR)
          vertexName = MacObjectiveC.cfString("vertex_main")
          fragmentName = MacObjectiveC.cfString("fragment_main")
          val errorOut = stack.callocPointer(1)
          library =
            MacObjectiveC.sendPointer(
              device,
              "newLibraryWithSource:options:error:",
              source,
              NULL,
              memAddress(errorOut),
            )
          if (library == NULL) {
            error(
              "Metal shader library creation failed: ${MacObjectiveC.errorDescription(errorOut[0])}"
            )
          }
          vertex = MacObjectiveC.sendPointer(library, "newFunctionWithName:", vertexName)
          fragment = MacObjectiveC.sendPointer(library, "newFunctionWithName:", fragmentName)
          check(vertex != NULL && fragment != NULL) { "Metal shader function lookup failed" }

          descriptor = MacObjectiveC.allocInit("MTLRenderPipelineDescriptor")
          MacObjectiveC.sendVoid(descriptor, "setVertexFunction:", vertex)
          MacObjectiveC.sendVoid(descriptor, "setFragmentFunction:", fragment)
          val attachment =
            MacObjectiveC.sendPointer(
              MacObjectiveC.sendPointer(descriptor, "colorAttachments"),
              "objectAtIndexedSubscript:",
              0,
            )
          MacObjectiveC.sendVoid(attachment, "setPixelFormat:", MTL_PIXEL_FORMAT_BGRA8_UNORM)

          errorOut.put(0, NULL)
          val pipeline =
            MacObjectiveC.sendPointer(
              device,
              "newRenderPipelineStateWithDescriptor:error:",
              descriptor,
              memAddress(errorOut),
            )
          if (pipeline == NULL) {
            error(
              "Metal render pipeline creation failed: ${MacObjectiveC.errorDescription(errorOut[0])}"
            )
          }
          return pipeline
        }
      }
    } finally {
      MacObjectiveC.release(descriptor)
      MacObjectiveC.release(fragment)
      MacObjectiveC.release(vertex)
      MacObjectiveC.release(library)
      if (fragmentName != NULL) {
        CoreFoundation.CFRelease(fragmentName)
      }
      if (vertexName != NULL) {
        CoreFoundation.CFRelease(vertexName)
      }
      if (source != NULL) {
        CoreFoundation.CFRelease(source)
      }
    }
  }

  fun nextDrawable(): Long = MacObjectiveC.sendPointer(layer, "nextDrawable")

  fun drawableTexture(drawable: Long): Long = MacObjectiveC.sendPointer(drawable, "texture")

  override fun resize(viewport: Viewport) {
    resizeLayer(layer, viewport)
  }

  fun releaseObject(objectAddress: Long) {
    MacObjectiveC.release(objectAddress)
  }

  override fun close() {
    if (closed) {
      return
    }
    closed = true
    if (view != NULL) {
      MacObjectiveC.sendVoid(view, "setLayer:", NULL)
    }
    MacObjectiveC.release(layer)
    MacObjectiveC.release(device)
    MacObjectiveC.release(view)
    layer = NULL
    device = NULL
    view = NULL
    if (window != NULL) {
      glfwDestroyWindow(window)
    }
    glfwTerminate()
  }

  internal companion object {
    private const val MTL_PIXEL_FORMAT_RGBA8_UNORM = 70
    private const val MTL_PIXEL_FORMAT_BGRA8_UNORM = 80
    private const val MTL_TEXTURE_TYPE_2D = 2
    private const val MTL_TEXTURE_USAGE_SHADER_READ = 1
    private const val MTL_TEXTURE_USAGE_RENDER_TARGET = 4

    fun create(title: String, width: Int, height: Int): MetalContext {
      check(glfwInit()) { "GLFW initialization failed" }
      var window = NULL
      var retainedView = NULL
      var device = NULL
      var layer = NULL
      try {
        MacObjectiveC.autoreleasePool().use {
          glfwDefaultWindowHints()
          glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)
          glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE)
          window = glfwCreateWindow(width, height, title, NULL, NULL)
          check(window != NULL) { "GLFW window creation failed" }
          val viewport = Viewport.read(window)
          val nsView = glfwGetCocoaView(window)
          check(nsView != NULL) { "GLFW did not expose a Cocoa NSView" }
          retainedView = MacObjectiveC.retain(nsView)
          device = MacObjectiveC.metalSystemDefaultDevice()
          check(device != NULL) { "MTLCreateSystemDefaultDevice returned nil" }
          layer = MacObjectiveC.allocInit("CAMetalLayer")
          MacObjectiveC.sendVoid(layer, "setDevice:", device)
          MacObjectiveC.sendVoid(layer, "setPixelFormat:", MTL_PIXEL_FORMAT_BGRA8_UNORM)
          resizeLayer(layer, viewport)
          MacObjectiveC.sendVoid(layer, "setOpaque:", true)
          MacObjectiveC.sendVoid(retainedView, "setWantsLayer:", true)
          MacObjectiveC.sendVoid(retainedView, "setLayer:", layer)

          System.out.printf("GLFW %s, Metal, Cocoa%n", glfwGetVersionString())
          return MetalContext(window, retainedView, device, layer)
        }
      } catch (error: RuntimeException) {
        MacObjectiveC.release(layer)
        MacObjectiveC.release(device)
        MacObjectiveC.release(retainedView)
        if (window != NULL) {
          glfwDestroyWindow(window)
        }
        glfwTerminate()
        throw error
      }
    }

    private fun resizeLayer(layer: Long, viewport: Viewport) {
      MacObjectiveC.sendSize(
        layer,
        "setDrawableSize:",
        viewport.framebufferWidth().toDouble(),
        viewport.framebufferHeight().toDouble(),
      )
    }
  }
}
