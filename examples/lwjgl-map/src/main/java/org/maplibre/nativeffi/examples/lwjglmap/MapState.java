package org.maplibre.nativeffi.examples.lwjglmap;

import org.maplibre.nativeffi.camera.CameraOptions;
import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.map.MapMode;
import org.maplibre.nativeffi.map.MapOptions;
import org.maplibre.nativeffi.render.RenderSessionHandle;
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor;
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

  static MapState create(VulkanContext vulkan, Viewport viewport, RenderTargetMode mode) {
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
      target = attachRenderTarget(vulkan, map, viewport, mode);
      map.setStyleUrl(STYLE_URL);
      map.jumpTo(
          new CameraOptions().center(37.7749, -122.4194).zoom(13.0).bearing(12.0).pitch(30.0));
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
    renderTarget.resize(viewport);
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

  private static RenderTarget attachRenderTarget(
      VulkanContext vulkan, MapHandle map, Viewport viewport, RenderTargetMode mode) {
    return switch (mode) {
      case NATIVE_SURFACE -> {
        var descriptor =
            new VulkanSurfaceDescriptor()
                .size(viewport.width(), viewport.height())
                .scaleFactor(viewport.scaleFactor())
                .instance(vulkan.instancePointer())
                .physicalDevice(vulkan.physicalDevicePointer())
                .device(vulkan.devicePointer())
                .graphicsQueue(vulkan.graphicsQueuePointer())
                .graphicsQueueFamilyIndex(vulkan.graphicsQueueFamilyIndex())
                .surface(vulkan.surfacePointer());
        yield new SurfaceRenderTarget(RenderSessionHandle.attachVulkanSurface(map, descriptor));
      }
      case OWNED_TEXTURE -> attachOwnedTextureRenderTarget(vulkan, map, viewport);
    };
  }

  private static RenderTarget attachOwnedTextureRenderTarget(
      VulkanContext vulkan, MapHandle map, Viewport viewport) {
    var descriptor =
        new VulkanOwnedTextureDescriptor()
            .size(viewport.width(), viewport.height())
            .scaleFactor(viewport.scaleFactor())
            .instance(vulkan.instancePointer())
            .physicalDevice(vulkan.physicalDevicePointer())
            .device(vulkan.devicePointer())
            .graphicsQueue(vulkan.graphicsQueuePointer())
            .graphicsQueueFamilyIndex(vulkan.graphicsQueueFamilyIndex());
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

  private interface RenderTarget extends AutoCloseable {
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
