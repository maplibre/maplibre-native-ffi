package org.maplibre.nativeffi.examples.lwjglmap;

import org.maplibre.nativeffi.camera.CameraOptions;
import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.map.MapMode;
import org.maplibre.nativeffi.map.MapOptions;
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor;
import org.maplibre.nativeffi.render.RenderSessionHandle;
import org.maplibre.nativeffi.render.RenderTargetExtent;
import org.maplibre.nativeffi.render.VulkanContextDescriptor;
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor;
import org.maplibre.nativeffi.runtime.RuntimeEventType;
import org.maplibre.nativeffi.runtime.RuntimeHandle;
import org.maplibre.nativeffi.runtime.RuntimeOptions;

final class MapState implements AutoCloseable {
  private static final String STYLE_URL = "https://tiles.openfreemap.org/styles/bright";

  private final RuntimeHandle runtime;
  private final MapHandle map;
  private final RenderTargetFactory targetFactory;
  private RenderTarget renderTarget;
  private boolean renderPending = true;

  private MapState(
      RuntimeHandle runtime,
      MapHandle map,
      RenderTarget renderTarget,
      RenderTargetFactory targetFactory) {
    this.runtime = runtime;
    this.map = map;
    this.renderTarget = renderTarget;
    this.targetFactory = targetFactory;
  }

  static MapState create(VulkanContext vulkan, Viewport viewport, RenderTargetMode mode) {
    return create(
        viewport, (map, currentViewport) -> attachRenderTarget(vulkan, map, currentViewport, mode));
  }

  static MapState create(OpenGLContext opengl, Viewport viewport, RenderTargetMode mode) {
    return create(
        viewport, (map, currentViewport) -> attachRenderTarget(opengl, map, currentViewport, mode));
  }

  private static MapState create(Viewport viewport, RenderTargetFactory targetFactory) {
    var runtime = RuntimeHandle.create(new RuntimeOptions().cachePath(":memory:"));
    var map =
        MapHandle.create(
            runtime,
            new MapOptions()
                .size(viewport.width(), viewport.height())
                .scaleFactor(viewport.scaleFactor())
                .mapMode(MapMode.CONTINUOUS));
    RenderTarget target = null;
    try {
      target = targetFactory.attach(map, viewport);
      map.setStyleUrl(STYLE_URL);
      map.jumpTo(
          new CameraOptions().center(37.7749, -122.4194).zoom(13.0).bearing(12.0).pitch(30.0));
      return new MapState(runtime, map, target, targetFactory);
    } catch (RuntimeException error) {
      if (target != null) {
        target.close();
      }
      map.close();
      runtime.close();
      throw error;
    }
  }

  MapHandle map() {
    return map;
  }

  void resize(Viewport viewport) {
    if (renderTarget.needsReattachOnResize()) {
      renderTarget.close();
      renderTarget = targetFactory.attach(map, viewport);
    } else {
      renderTarget.resize(viewport);
    }
    renderPending = true;
  }

  boolean step() {
    runtime.runOnce();
    drainEvents();
    if (!renderPending) {
      return false;
    }
    renderPending = false;
    renderTarget.renderUpdate();
    return true;
  }

  private void drainEvents() {
    while (true) {
      var event = runtime.pollEvent();
      if (event.isEmpty()) {
        return;
      }
      var value = event.get();
      if (value.type() == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE
          && value.mapSource().filter(source -> source == map).isPresent()) {
        renderPending = true;
      }
    }
  }

  @Override
  public void close() {
    try {
      renderTarget.close();
    } finally {
      try {
        map.close();
      } finally {
        runtime.close();
      }
    }
  }

  private interface RenderTargetFactory {
    RenderTarget attach(MapHandle map, Viewport viewport);
  }

  private static RenderTarget attachRenderTarget(
      VulkanContext vulkan, MapHandle map, Viewport viewport, RenderTargetMode mode) {
    return switch (mode) {
      case NATIVE_SURFACE -> {
        var descriptor =
            new VulkanSurfaceDescriptor()
                .extent(
                    new RenderTargetExtent(
                        viewport.width(), viewport.height(), viewport.scaleFactor()))
                .context(vulkanContextDescriptor(vulkan))
                .surface(vulkan.surfacePointer());
        yield new SurfaceRenderTarget(RenderSessionHandle.attachVulkanSurface(map, descriptor));
      }
      case OWNED_TEXTURE -> attachOwnedTextureRenderTarget(vulkan, map, viewport);
      case BORROWED_TEXTURE ->
          throw new IllegalArgumentException(
              "the LWJGL borrowed-texture example is implemented for OpenGL");
    };
  }

  private static RenderTarget attachRenderTarget(
      OpenGLContext opengl, MapHandle map, Viewport viewport, RenderTargetMode mode) {
    return switch (mode) {
      case NATIVE_SURFACE -> {
        var descriptor =
            new OpenGLSurfaceDescriptor()
                .extent(
                    new RenderTargetExtent(
                        viewport.width(), viewport.height(), viewport.scaleFactor()))
                .context(opengl.descriptor())
                .surface(opengl.surfacePointer());
        yield new SurfaceRenderTarget(RenderSessionHandle.attachOpenGLSurface(map, descriptor));
      }
      case OWNED_TEXTURE -> attachOpenGLOwnedTextureRenderTarget(opengl, map, viewport);
      case BORROWED_TEXTURE -> attachOpenGLBorrowedTextureRenderTarget(opengl, map, viewport);
    };
  }

  private static RenderTarget attachOwnedTextureRenderTarget(
      VulkanContext vulkan, MapHandle map, Viewport viewport) {
    var descriptor =
        new VulkanOwnedTextureDescriptor()
            .extent(
                new RenderTargetExtent(viewport.width(), viewport.height(), viewport.scaleFactor()))
            .context(vulkanContextDescriptor(vulkan));
    RenderSessionHandle session = null;
    VulkanTextureCompositor compositor = null;
    try {
      session = RenderSessionHandle.attachVulkanOwnedTexture(map, descriptor);
      compositor = new VulkanTextureCompositor(vulkan, viewport);
      return new OwnedTextureRenderTarget(session, compositor);
    } catch (RuntimeException error) {
      if (compositor != null) {
        try {
          compositor.close();
        } catch (RuntimeException cleanupError) {
          error.addSuppressed(cleanupError);
        }
      }
      if (session != null) {
        try {
          session.close();
        } catch (RuntimeException cleanupError) {
          error.addSuppressed(cleanupError);
        }
      }
      throw error;
    }
  }

  private static RenderTarget attachOpenGLOwnedTextureRenderTarget(
      OpenGLContext opengl, MapHandle map, Viewport viewport) {
    var descriptor =
        new OpenGLOwnedTextureDescriptor()
            .extent(
                new RenderTargetExtent(viewport.width(), viewport.height(), viewport.scaleFactor()))
            .context(opengl.descriptor());
    RenderSessionHandle session = null;
    OpenGLTextureCompositor compositor = null;
    try {
      session = RenderSessionHandle.attachOpenGLOwnedTexture(map, descriptor);
      compositor = new OpenGLTextureCompositor(opengl, viewport);
      return new OpenGLOwnedTextureRenderTarget(session, compositor);
    } catch (RuntimeException error) {
      if (compositor != null) {
        try {
          compositor.close();
        } catch (RuntimeException cleanupError) {
          error.addSuppressed(cleanupError);
        }
      }
      if (session != null) {
        try {
          session.close();
        } catch (RuntimeException cleanupError) {
          error.addSuppressed(cleanupError);
        }
      }
      throw error;
    }
  }

  private static RenderTarget attachOpenGLBorrowedTextureRenderTarget(
      OpenGLContext opengl, MapHandle map, Viewport viewport) {
    OpenGLBorrowedTexture borrowedTexture = null;
    RenderSessionHandle session = null;
    OpenGLTextureCompositor compositor = null;
    try {
      borrowedTexture = new OpenGLBorrowedTexture(opengl, viewport);
      var descriptor =
          new OpenGLBorrowedTextureDescriptor()
              .extent(
                  new RenderTargetExtent(
                      viewport.width(), viewport.height(), viewport.scaleFactor()))
              .context(opengl.descriptor())
              .texture(borrowedTexture.texture())
              .target(borrowedTexture.target());
      session = RenderSessionHandle.attachOpenGLBorrowedTexture(map, descriptor);
      compositor = new OpenGLTextureCompositor(opengl, viewport);
      return new OpenGLBorrowedTextureRenderTarget(session, compositor, borrowedTexture);
    } catch (RuntimeException error) {
      if (compositor != null) {
        try {
          compositor.close();
        } catch (RuntimeException cleanupError) {
          error.addSuppressed(cleanupError);
        }
      }
      if (session != null) {
        try {
          session.close();
        } catch (RuntimeException cleanupError) {
          error.addSuppressed(cleanupError);
        }
      }
      if (borrowedTexture != null) {
        try {
          borrowedTexture.close();
        } catch (RuntimeException cleanupError) {
          error.addSuppressed(cleanupError);
        }
      }
      throw error;
    }
  }

  private static VulkanContextDescriptor vulkanContextDescriptor(VulkanContext vulkan) {
    return new VulkanContextDescriptor(
            vulkan.instancePointer(),
            vulkan.physicalDevicePointer(),
            vulkan.devicePointer(),
            vulkan.graphicsQueuePointer(),
            vulkan.graphicsQueueFamilyIndex())
        .procAddresses(vulkan.getInstanceProcAddrPointer(), vulkan.getDeviceProcAddrPointer());
  }

  private interface RenderTarget extends AutoCloseable {
    default boolean needsReattachOnResize() {
      return false;
    }

    void resize(Viewport viewport);

    void renderUpdate();

    @Override
    void close();
  }

  private static final class SurfaceRenderTarget implements RenderTarget {
    private final RenderSessionHandle session;

    SurfaceRenderTarget(RenderSessionHandle session) {
      this.session = session;
    }

    @Override
    public void resize(Viewport viewport) {
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor());
    }

    @Override
    public void renderUpdate() {
      session.renderUpdate();
    }

    @Override
    public void close() {
      session.close();
    }
  }

  private static final class OpenGLOwnedTextureRenderTarget implements RenderTarget {
    private final RenderSessionHandle session;
    private final OpenGLTextureCompositor compositor;

    OpenGLOwnedTextureRenderTarget(
        RenderSessionHandle session, OpenGLTextureCompositor compositor) {
      this.session = session;
      this.compositor = compositor;
    }

    @Override
    public void resize(Viewport viewport) {
      compositor.resize(viewport);
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor());
    }

    @Override
    public void renderUpdate() {
      session.renderUpdate();
      try (var frameHandle = session.acquireOpenGLOwnedTextureFrame()) {
        compositor.draw(frameHandle);
      }
    }

    @Override
    public void close() {
      try {
        compositor.close();
      } finally {
        session.close();
      }
    }
  }

  private static final class OpenGLBorrowedTextureRenderTarget implements RenderTarget {
    private final RenderSessionHandle session;
    private final OpenGLTextureCompositor compositor;
    private final OpenGLBorrowedTexture borrowedTexture;

    OpenGLBorrowedTextureRenderTarget(
        RenderSessionHandle session,
        OpenGLTextureCompositor compositor,
        OpenGLBorrowedTexture borrowedTexture) {
      this.session = session;
      this.compositor = compositor;
      this.borrowedTexture = borrowedTexture;
    }

    @Override
    public boolean needsReattachOnResize() {
      return true;
    }

    @Override
    public void resize(Viewport viewport) {
      throw new IllegalStateException("borrowed texture render targets must be reattached");
    }

    @Override
    public void renderUpdate() {
      session.renderUpdate();
      compositor.draw(borrowedTexture.target(), borrowedTexture.texture());
    }

    @Override
    public void close() {
      try {
        session.close();
      } finally {
        try {
          borrowedTexture.close();
        } finally {
          compositor.close();
        }
      }
    }
  }

  private static final class OwnedTextureRenderTarget implements RenderTarget {
    private final RenderSessionHandle session;
    private final VulkanTextureCompositor compositor;

    OwnedTextureRenderTarget(RenderSessionHandle session, VulkanTextureCompositor compositor) {
      this.session = session;
      this.compositor = compositor;
    }

    @Override
    public void resize(Viewport viewport) {
      compositor.resize(viewport);
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor());
    }

    @Override
    public void renderUpdate() {
      session.renderUpdate();
      try (var frameHandle = session.acquireVulkanOwnedTextureFrame()) {
        compositor.draw(frameHandle);
      }
    }

    @Override
    public void close() {
      try {
        compositor.close();
      } finally {
        session.close();
      }
    }
  }
}
