package org.maplibre.nativeffi.examples.lwjglmap;

import org.lwjgl.vulkan.VK10;
import org.maplibre.nativeffi.camera.CameraOptions;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.map.MapMode;
import org.maplibre.nativeffi.map.MapOptions;
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalContextDescriptor;
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor;
import org.maplibre.nativeffi.render.RenderSessionHandle;
import org.maplibre.nativeffi.render.RenderTargetExtent;
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanContextDescriptor;
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor;
import org.maplibre.nativeffi.runtime.RuntimeEventPayload;
import org.maplibre.nativeffi.runtime.RuntimeEventType;
import org.maplibre.nativeffi.runtime.RuntimeHandle;
import org.maplibre.nativeffi.runtime.RuntimeOptions;

final class MapState implements AutoCloseable {
  private static final String STYLE_URL = "https://tiles.openfreemap.org/styles/bright";

  private final RuntimeHandle runtime;
  private final MapHandle map;
  private final RenderTarget renderTarget;
  private boolean renderPending = true;

  private MapState(RuntimeHandle runtime, MapHandle map, RenderTarget renderTarget) {
    this.runtime = runtime;
    this.map = map;
    this.renderTarget = renderTarget;
  }

  static MapState create(GraphicsContext graphics, Viewport viewport, RenderTargetMode mode) {
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
      map.setStyleUrl(STYLE_URL);
      map.jumpTo(
          new CameraOptions()
              .center(new LatLng(37.7749, -122.4194))
              .zoom(13.0)
              .bearing(12.0)
              .pitch(30.0));
      target = attachRenderTarget(graphics, map, viewport, mode);
      return new MapState(runtime, map, target);
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
      renderTarget.reattach(viewport);
    } else {
      renderTarget.resize(viewport);
    }
    renderPending = true;
  }

  void requestRender() {
    renderPending = true;
  }

  boolean step() {
    runtime.runOnce();
    drainEvents();
    if (!renderPending) {
      return false;
    }
    try {
      if (renderTarget.needsMetalAutoreleasePool()) {
        try (var ignored = MacObjectiveC.autoreleasePool()) {
          renderTarget.renderUpdate();
        }
      } else {
        renderTarget.renderUpdate();
      }
      renderPending = false;
    } catch (org.maplibre.nativeffi.error.InvalidStateException ignored) {
      renderPending = true;
    }
    return true;
  }

  private void drainEvents() {
    while (true) {
      var event = runtime.pollEvent();
      if (event.isEmpty()) {
        return;
      }
      var value = event.get();
      if (RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE.equals(value.type())
          && value.mapSource().filter(source -> source == map).isPresent()) {
        renderPending = true;
      } else if (RuntimeEventType.MAP_RENDER_FRAME_FINISHED.equals(value.type())
          && value.mapSource().filter(source -> source == map).isPresent()
          && value.payload() instanceof RuntimeEventPayload.RenderFrame frame
          && frame.needsRepaint()) {
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

  private static RenderTarget attachRenderTarget(
      GraphicsContext graphics, MapHandle map, Viewport viewport, RenderTargetMode mode) {
    if (graphics instanceof MetalContext metal) {
      return attachMetalRenderTarget(metal, map, viewport, mode);
    }
    if (graphics instanceof VulkanContext vulkan) {
      return attachVulkanRenderTarget(vulkan, map, viewport, mode);
    }
    if (graphics instanceof OpenGLContext opengl) {
      return OpenGLRenderTarget.attach(opengl, map, viewport, mode);
    }
    throw new IllegalStateException("Unsupported graphics context: " + graphics.backend());
  }

  private static RenderTarget attachVulkanRenderTarget(
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
        yield new VulkanSurfaceRenderTarget(
            RenderSessionHandle.attachVulkanSurface(map, descriptor));
      }
      case OWNED_TEXTURE -> attachOwnedTextureRenderTarget(vulkan, map, viewport);
      case BORROWED_TEXTURE -> attachBorrowedTextureRenderTarget(vulkan, map, viewport);
    };
  }

  private static RenderTarget attachMetalRenderTarget(
      MetalContext metal, MapHandle map, Viewport viewport, RenderTargetMode mode) {
    return switch (mode) {
      case NATIVE_SURFACE -> {
        var descriptor =
            new MetalSurfaceDescriptor()
                .extent(
                    new RenderTargetExtent(
                        viewport.width(), viewport.height(), viewport.scaleFactor()))
                .context(metalContextDescriptor(metal))
                .layer(metal.layerPointer());
        yield new MetalSurfaceRenderTarget(RenderSessionHandle.attachMetalSurface(map, descriptor));
      }
      case OWNED_TEXTURE -> attachMetalOwnedTextureRenderTarget(metal, map, viewport);
      case BORROWED_TEXTURE -> attachMetalBorrowedTextureRenderTarget(metal, map, viewport);
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
      return new VulkanOwnedTextureRenderTarget(session, compositor);
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

  private static RenderTarget attachBorrowedTextureRenderTarget(
      VulkanContext vulkan, MapHandle map, Viewport viewport) {
    VulkanBorrowedImage image = null;
    RenderSessionHandle session = null;
    VulkanTextureCompositor compositor = null;
    try {
      image = VulkanBorrowedImage.create(vulkan, viewport);
      var descriptor =
          new VulkanBorrowedTextureDescriptor()
              .extent(
                  new RenderTargetExtent(
                      viewport.width(), viewport.height(), viewport.scaleFactor()))
              .context(vulkanContextDescriptor(vulkan))
              .image(image.imagePointer())
              .imageView(image.viewPointer())
              .format(VK10.VK_FORMAT_R8G8B8A8_UNORM)
              .initialLayout(VK10.VK_IMAGE_LAYOUT_UNDEFINED)
              .finalLayout(VK10.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      session = RenderSessionHandle.attachVulkanBorrowedTexture(map, descriptor);
      compositor = new VulkanTextureCompositor(vulkan, viewport);
      return new VulkanBorrowedTextureRenderTarget(vulkan, map, session, compositor, image);
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
      if (image != null) {
        try {
          image.close();
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

  private static RenderTarget attachMetalOwnedTextureRenderTarget(
      MetalContext metal, MapHandle map, Viewport viewport) {
    var descriptor =
        new MetalOwnedTextureDescriptor()
            .extent(
                new RenderTargetExtent(viewport.width(), viewport.height(), viewport.scaleFactor()))
            .context(metalContextDescriptor(metal));
    RenderSessionHandle session = null;
    MetalTextureCompositor compositor = null;
    try {
      session = RenderSessionHandle.attachMetalOwnedTexture(map, descriptor);
      compositor = new MetalTextureCompositor(metal);
      return new MetalOwnedTextureRenderTarget(session, compositor);
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

  private static RenderTarget attachMetalBorrowedTextureRenderTarget(
      MetalContext metal, MapHandle map, Viewport viewport) {
    MetalBorrowedTexture texture = null;
    RenderSessionHandle session = null;
    MetalTextureCompositor compositor = null;
    try {
      texture = new MetalBorrowedTexture(metal, viewport);
      var descriptor =
          new MetalBorrowedTextureDescriptor()
              .extent(
                  new RenderTargetExtent(
                      viewport.width(), viewport.height(), viewport.scaleFactor()))
              .texture(texture.pointer());
      session = RenderSessionHandle.attachMetalBorrowedTexture(map, descriptor);
      compositor = new MetalTextureCompositor(metal);
      return new MetalBorrowedTextureRenderTarget(metal, map, session, compositor, texture);
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
      if (texture != null) {
        try {
          texture.close();
        } catch (RuntimeException cleanupError) {
          error.addSuppressed(cleanupError);
        }
      }
      throw error;
    }
  }

  private static MetalContextDescriptor metalContextDescriptor(MetalContext metal) {
    return new MetalContextDescriptor(metal.devicePointer());
  }

  private static final class VulkanSurfaceRenderTarget implements RenderTarget {
    private final RenderSessionHandle session;

    VulkanSurfaceRenderTarget(RenderSessionHandle session) {
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

  private static final class MetalSurfaceRenderTarget implements RenderTarget {
    private final RenderSessionHandle session;

    MetalSurfaceRenderTarget(RenderSessionHandle session) {
      this.session = session;
    }

    @Override
    public boolean needsMetalAutoreleasePool() {
      return true;
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

  private static final class VulkanOwnedTextureRenderTarget implements RenderTarget {
    private final RenderSessionHandle session;
    private final VulkanTextureCompositor compositor;

    VulkanOwnedTextureRenderTarget(
        RenderSessionHandle session, VulkanTextureCompositor compositor) {
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

  private static final class MetalOwnedTextureRenderTarget implements RenderTarget {
    private final RenderSessionHandle session;
    private final MetalTextureCompositor compositor;

    MetalOwnedTextureRenderTarget(RenderSessionHandle session, MetalTextureCompositor compositor) {
      this.session = session;
      this.compositor = compositor;
    }

    @Override
    public boolean needsMetalAutoreleasePool() {
      return true;
    }

    @Override
    public void resize(Viewport viewport) {
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor());
    }

    @Override
    public void renderUpdate() {
      session.renderUpdate();
      try (var frameHandle = session.acquireMetalOwnedTextureFrame()) {
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

  private static final class VulkanBorrowedTextureRenderTarget implements RenderTarget {
    private final VulkanContext vulkan;
    private final MapHandle map;
    private RenderSessionHandle session;
    private VulkanTextureCompositor compositor;
    private VulkanBorrowedImage image;

    VulkanBorrowedTextureRenderTarget(
        VulkanContext vulkan,
        MapHandle map,
        RenderSessionHandle session,
        VulkanTextureCompositor compositor,
        VulkanBorrowedImage image) {
      this.vulkan = vulkan;
      this.map = map;
      this.session = session;
      this.compositor = compositor;
      this.image = image;
    }

    @Override
    public boolean needsReattachOnResize() {
      return true;
    }

    @Override
    public void reattach(Viewport viewport) {
      close();
      var replacement = attachBorrowedTextureRenderTarget(vulkan, map, viewport);
      if (replacement instanceof VulkanBorrowedTextureRenderTarget borrowed) {
        session = borrowed.session;
        compositor = borrowed.compositor;
        image = borrowed.image;
        borrowed.session = null;
        borrowed.compositor = null;
        borrowed.image = null;
      } else {
        throw new IllegalStateException("unexpected borrowed texture replacement");
      }
    }

    @Override
    public void resize(Viewport viewport) {
      throw new IllegalStateException(
          "borrowed texture resize requires render target reattachment");
    }

    @Override
    public void renderUpdate() {
      session.renderUpdate();
      compositor.drawImageView(image.view());
    }

    @Override
    public void close() {
      var closingCompositor = compositor;
      var closingSession = session;
      var closingImage = image;
      compositor = null;
      session = null;
      image = null;
      try {
        if (closingCompositor != null) {
          closingCompositor.close();
        }
      } finally {
        try {
          if (closingSession != null) {
            closingSession.close();
          }
        } finally {
          if (closingImage != null) {
            closingImage.close();
          }
        }
      }
    }
  }

  private static final class MetalBorrowedTextureRenderTarget implements RenderTarget {
    private final MetalContext metal;
    private final MapHandle map;
    private RenderSessionHandle session;
    private MetalTextureCompositor compositor;
    private MetalBorrowedTexture texture;

    MetalBorrowedTextureRenderTarget(
        MetalContext metal,
        MapHandle map,
        RenderSessionHandle session,
        MetalTextureCompositor compositor,
        MetalBorrowedTexture texture) {
      this.metal = metal;
      this.map = map;
      this.session = session;
      this.compositor = compositor;
      this.texture = texture;
    }

    @Override
    public boolean needsMetalAutoreleasePool() {
      return true;
    }

    @Override
    public boolean needsReattachOnResize() {
      return true;
    }

    @Override
    public void reattach(Viewport viewport) {
      close();
      var replacement = attachMetalBorrowedTextureRenderTarget(metal, map, viewport);
      if (replacement instanceof MetalBorrowedTextureRenderTarget borrowed) {
        session = borrowed.session;
        compositor = borrowed.compositor;
        texture = borrowed.texture;
        borrowed.session = null;
        borrowed.compositor = null;
        borrowed.texture = null;
      } else {
        throw new IllegalStateException("unexpected borrowed texture replacement");
      }
    }

    @Override
    public void resize(Viewport viewport) {
      throw new IllegalStateException(
          "borrowed texture resize requires render target reattachment");
    }

    @Override
    public void renderUpdate() {
      session.renderUpdate();
      compositor.drawTexture(texture.texture());
    }

    @Override
    public void close() {
      var closingCompositor = compositor;
      var closingSession = session;
      var closingTexture = texture;
      compositor = null;
      session = null;
      texture = null;
      try {
        if (closingCompositor != null) {
          closingCompositor.close();
        }
      } finally {
        try {
          if (closingSession != null) {
            closingSession.close();
          }
        } finally {
          if (closingTexture != null) {
            closingTexture.close();
          }
        }
      }
    }
  }
}
