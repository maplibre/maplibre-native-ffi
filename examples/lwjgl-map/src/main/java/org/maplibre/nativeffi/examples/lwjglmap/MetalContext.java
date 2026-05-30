package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.glfw.GLFW.GLFW_CLIENT_API;
import static org.lwjgl.glfw.GLFW.GLFW_NO_API;
import static org.lwjgl.glfw.GLFW.GLFW_RESIZABLE;
import static org.lwjgl.glfw.GLFW.GLFW_TRUE;
import static org.lwjgl.glfw.GLFW.glfwCreateWindow;
import static org.lwjgl.glfw.GLFW.glfwDefaultWindowHints;
import static org.lwjgl.glfw.GLFW.glfwDestroyWindow;
import static org.lwjgl.glfw.GLFW.glfwGetVersionString;
import static org.lwjgl.glfw.GLFW.glfwInit;
import static org.lwjgl.glfw.GLFW.glfwTerminate;
import static org.lwjgl.glfw.GLFW.glfwWindowHint;
import static org.lwjgl.glfw.GLFWNativeCocoa.glfwGetCocoaView;
import static org.lwjgl.system.MemoryUtil.NULL;
import static org.lwjgl.system.MemoryUtil.memAddress;

import org.lwjgl.system.MemoryStack;
import org.maplibre.nativeffi.render.NativePointer;
import org.maplibre.nativeffi.render.RenderBackend;

final class MetalContext implements GraphicsContext {
  private static final int MTL_PIXEL_FORMAT_RGBA8_UNORM = 70;
  private static final int MTL_PIXEL_FORMAT_BGRA8_UNORM = 80;
  private static final int MTL_TEXTURE_TYPE_2D = 2;
  private static final int MTL_TEXTURE_USAGE_SHADER_READ = 1;
  private static final int MTL_TEXTURE_USAGE_RENDER_TARGET = 4;

  private final long window;
  private long view;
  private long device;
  private long layer;
  private boolean closed;

  private MetalContext(long window, long view, long device, long layer) {
    this.window = window;
    this.view = view;
    this.device = device;
    this.layer = layer;
  }

  static MetalContext create(String title, int width, int height) {
    if (!glfwInit()) {
      throw new IllegalStateException("GLFW initialization failed");
    }
    long window = NULL;
    long retainedView = NULL;
    long device = NULL;
    long layer = NULL;
    try (var ignored = MacObjectiveC.autoreleasePool()) {
      glfwDefaultWindowHints();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      window = glfwCreateWindow(width, height, title, NULL, NULL);
      if (window == NULL) {
        throw new IllegalStateException("GLFW window creation failed");
      }
      var viewport = Viewport.read(window);
      var nsView = glfwGetCocoaView(window);
      if (nsView == NULL) {
        throw new IllegalStateException("GLFW did not expose a Cocoa NSView");
      }
      retainedView = MacObjectiveC.retain(nsView);
      device = MacObjectiveC.metalSystemDefaultDevice();
      if (device == NULL) {
        throw new IllegalStateException("MTLCreateSystemDefaultDevice returned nil");
      }
      layer = MacObjectiveC.allocInit("CAMetalLayer");
      MacObjectiveC.sendVoid(layer, "setDevice:", device);
      MacObjectiveC.sendVoid(layer, "setPixelFormat:", MTL_PIXEL_FORMAT_BGRA8_UNORM);
      resizeLayer(layer, viewport);
      MacObjectiveC.sendVoid(layer, "setOpaque:", true);
      MacObjectiveC.sendVoid(retainedView, "setWantsLayer:", true);
      MacObjectiveC.sendVoid(retainedView, "setLayer:", layer);

      System.out.printf("GLFW %s, Metal, Cocoa%n", glfwGetVersionString());
      return new MetalContext(window, retainedView, device, layer);
    } catch (RuntimeException error) {
      MacObjectiveC.release(layer);
      MacObjectiveC.release(device);
      MacObjectiveC.release(retainedView);
      if (window != NULL) {
        glfwDestroyWindow(window);
      }
      glfwTerminate();
      throw error;
    }
  }

  @Override
  public long window() {
    return window;
  }

  @Override
  public RenderBackend backend() {
    return RenderBackend.METAL;
  }

  NativePointer devicePointer() {
    return NativePointer.ofAddress(device);
  }

  NativePointer layerPointer() {
    return NativePointer.ofAddress(layer);
  }

  long createBorrowedTexture(Viewport viewport) {
    long descriptor = NULL;
    try (var ignored = MacObjectiveC.autoreleasePool()) {
      descriptor = MacObjectiveC.allocInit("MTLTextureDescriptor");
      MacObjectiveC.sendVoid(descriptor, "setTextureType:", MTL_TEXTURE_TYPE_2D);
      MacObjectiveC.sendVoid(descriptor, "setPixelFormat:", MTL_PIXEL_FORMAT_RGBA8_UNORM);
      MacObjectiveC.sendVoid(descriptor, "setWidth:", (long) viewport.framebufferWidth());
      MacObjectiveC.sendVoid(descriptor, "setHeight:", (long) viewport.framebufferHeight());
      MacObjectiveC.sendVoid(descriptor, "setDepth:", 1L);
      MacObjectiveC.sendVoid(descriptor, "setMipmapLevelCount:", 1L);
      MacObjectiveC.sendVoid(descriptor, "setArrayLength:", 1L);
      MacObjectiveC.sendVoid(descriptor, "setSampleCount:", 1L);
      MacObjectiveC.sendVoid(
          descriptor,
          "setUsage:",
          (long) (MTL_TEXTURE_USAGE_SHADER_READ | MTL_TEXTURE_USAGE_RENDER_TARGET));
      var texture = MacObjectiveC.sendPointer(device, "newTextureWithDescriptor:", descriptor);
      if (texture == NULL) {
        throw new IllegalStateException("Metal borrowed texture creation failed");
      }
      return texture;
    } finally {
      MacObjectiveC.release(descriptor);
    }
  }

  long createCommandQueue() {
    var queue = MacObjectiveC.sendPointer(device, "newCommandQueue");
    if (queue == NULL) {
      throw new IllegalStateException("Metal command queue creation failed");
    }
    return queue;
  }

  long createRenderPipeline() {
    long source = NULL;
    long vertexName = NULL;
    long fragmentName = NULL;
    long library = NULL;
    long vertex = NULL;
    long fragment = NULL;
    long descriptor = NULL;
    try (var ignored = MacObjectiveC.autoreleasePool();
        var stack = MemoryStack.stackPush()) {
      source = MacObjectiveC.cfString(MetalShaders.TEXTURE_COMPOSITOR);
      vertexName = MacObjectiveC.cfString("vertex_main");
      fragmentName = MacObjectiveC.cfString("fragment_main");
      var errorOut = stack.callocPointer(1);
      library =
          MacObjectiveC.sendPointer(
              device, "newLibraryWithSource:options:error:", source, NULL, memAddress(errorOut));
      if (library == NULL) {
        throw new IllegalStateException(
            "Metal shader library creation failed: "
                + MacObjectiveC.errorDescription(errorOut.get(0)));
      }
      vertex = MacObjectiveC.sendPointer(library, "newFunctionWithName:", vertexName);
      fragment = MacObjectiveC.sendPointer(library, "newFunctionWithName:", fragmentName);
      if (vertex == NULL || fragment == NULL) {
        throw new IllegalStateException("Metal shader function lookup failed");
      }

      descriptor = MacObjectiveC.allocInit("MTLRenderPipelineDescriptor");
      MacObjectiveC.sendVoid(descriptor, "setVertexFunction:", vertex);
      MacObjectiveC.sendVoid(descriptor, "setFragmentFunction:", fragment);
      var attachment =
          MacObjectiveC.sendPointer(
              MacObjectiveC.sendPointer(descriptor, "colorAttachments"),
              "objectAtIndexedSubscript:",
              0);
      MacObjectiveC.sendVoid(attachment, "setPixelFormat:", MTL_PIXEL_FORMAT_BGRA8_UNORM);

      errorOut.put(0, NULL);
      var pipeline =
          MacObjectiveC.sendPointer(
              device,
              "newRenderPipelineStateWithDescriptor:error:",
              descriptor,
              memAddress(errorOut));
      if (pipeline == NULL) {
        throw new IllegalStateException(
            "Metal render pipeline creation failed: "
                + MacObjectiveC.errorDescription(errorOut.get(0)));
      }
      return pipeline;
    } finally {
      MacObjectiveC.release(descriptor);
      MacObjectiveC.release(fragment);
      MacObjectiveC.release(vertex);
      MacObjectiveC.release(library);
      if (fragmentName != NULL) {
        org.lwjgl.system.macosx.CoreFoundation.CFRelease(fragmentName);
      }
      if (vertexName != NULL) {
        org.lwjgl.system.macosx.CoreFoundation.CFRelease(vertexName);
      }
      if (source != NULL) {
        org.lwjgl.system.macosx.CoreFoundation.CFRelease(source);
      }
    }
  }

  long nextDrawable() {
    return MacObjectiveC.sendPointer(layer, "nextDrawable");
  }

  long drawableTexture(long drawable) {
    return MacObjectiveC.sendPointer(drawable, "texture");
  }

  @Override
  public void resize(Viewport viewport) {
    resizeLayer(layer, viewport);
  }

  void releaseObject(long object) {
    MacObjectiveC.release(object);
  }

  @Override
  public void close() {
    if (closed) {
      return;
    }
    closed = true;
    if (view != NULL) {
      MacObjectiveC.sendVoid(view, "setLayer:", NULL);
    }
    MacObjectiveC.release(layer);
    MacObjectiveC.release(device);
    MacObjectiveC.release(view);
    layer = NULL;
    device = NULL;
    view = NULL;
    if (window != NULL) {
      glfwDestroyWindow(window);
    }
    glfwTerminate();
  }

  private static void resizeLayer(long layer, Viewport viewport) {
    MacObjectiveC.sendSize(
        layer, "setDrawableSize:", viewport.framebufferWidth(), viewport.framebufferHeight());
  }
}
