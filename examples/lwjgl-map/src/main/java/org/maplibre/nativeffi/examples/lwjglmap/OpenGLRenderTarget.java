package org.maplibre.nativeffi.examples.lwjglmap;

import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor;
import org.maplibre.nativeffi.render.RenderSessionHandle;
import org.maplibre.nativeffi.render.RenderTargetExtent;

final class OpenGLRenderTarget {
  private OpenGLRenderTarget() {}

  static RenderTarget attach(
      OpenGLContext context, MapHandle map, Viewport viewport, RenderTargetMode mode) {
    return switch (mode) {
      case NATIVE_SURFACE -> attachSurface(context, map, viewport);
      case OWNED_TEXTURE -> attachOwnedTexture(context, map, viewport);
      case BORROWED_TEXTURE -> attachBorrowedTexture(context, map, viewport);
    };
  }

  private static RenderTarget attachSurface(
      OpenGLContext context, MapHandle map, Viewport viewport) {
    var descriptor =
        new OpenGLSurfaceDescriptor()
            .extent(extent(viewport))
            .context(context.descriptor())
            .surface(context.surfacePointer());
    return new Surface(RenderSessionHandle.attachOpenGLSurface(map, descriptor));
  }

  private static RenderTarget attachOwnedTexture(
      OpenGLContext context, MapHandle map, Viewport viewport) {
    var descriptor =
        new OpenGLOwnedTextureDescriptor().extent(extent(viewport)).context(context.descriptor());
    RenderSessionHandle session = null;
    OpenGLTextureCompositor compositor = null;
    try {
      session = RenderSessionHandle.attachOpenGLOwnedTexture(map, descriptor);
      compositor = new OpenGLTextureCompositor(context, viewport);
      return new OwnedTexture(session, compositor);
    } catch (RuntimeException error) {
      closeSuppressed(error, compositor);
      closeSuppressed(error, session);
      throw error;
    }
  }

  private static RenderTarget attachBorrowedTexture(
      OpenGLContext context, MapHandle map, Viewport viewport) {
    OpenGLBorrowedTexture texture = null;
    RenderSessionHandle session = null;
    OpenGLTextureCompositor compositor = null;
    try {
      texture = new OpenGLBorrowedTexture(context, viewport);
      var descriptor =
          new OpenGLBorrowedTextureDescriptor()
              .extent(extent(viewport))
              .context(context.descriptor())
              .texture(texture.texture())
              .target(texture.target());
      session = RenderSessionHandle.attachOpenGLBorrowedTexture(map, descriptor);
      compositor = new OpenGLTextureCompositor(context, viewport);
      return new BorrowedTexture(context, map, session, compositor, texture);
    } catch (RuntimeException error) {
      closeSuppressed(error, compositor);
      closeSuppressed(error, session);
      closeSuppressed(error, texture);
      throw error;
    }
  }

  private static RenderTargetExtent extent(Viewport viewport) {
    return new RenderTargetExtent(viewport.width(), viewport.height(), viewport.scaleFactor());
  }

  private static void closeSuppressed(RuntimeException error, AutoCloseable closeable) {
    if (closeable == null) {
      return;
    }
    try {
      closeable.close();
    } catch (Exception cleanupError) {
      error.addSuppressed(cleanupError);
    }
  }

  private static final class Surface implements RenderTarget {
    private final RenderSessionHandle session;

    Surface(RenderSessionHandle session) {
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

  private static final class OwnedTexture implements RenderTarget {
    private final RenderSessionHandle session;
    private final OpenGLTextureCompositor compositor;

    OwnedTexture(RenderSessionHandle session, OpenGLTextureCompositor compositor) {
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

  private static final class BorrowedTexture implements RenderTarget {
    private final OpenGLContext context;
    private final MapHandle map;
    private RenderSessionHandle session;
    private OpenGLTextureCompositor compositor;
    private OpenGLBorrowedTexture texture;

    BorrowedTexture(
        OpenGLContext context,
        MapHandle map,
        RenderSessionHandle session,
        OpenGLTextureCompositor compositor,
        OpenGLBorrowedTexture texture) {
      this.context = context;
      this.map = map;
      this.session = session;
      this.compositor = compositor;
      this.texture = texture;
    }

    @Override
    public boolean needsReattachOnResize() {
      return true;
    }

    @Override
    public void reattach(Viewport viewport) {
      close();
      var replacement = attachBorrowedTexture(context, map, viewport);
      if (replacement instanceof BorrowedTexture borrowed) {
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
