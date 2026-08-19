package org.maplibre.nativeffi.examples.lwjglmap

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLContextDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderResult
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.WglContextDescriptor

internal object OpenGLRenderTarget {
  fun attach(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
    mode: RenderTargetMode,
  ): RenderTarget =
    when (mode) {
      RenderTargetMode.NATIVE_SURFACE -> attachSurface(context, map, viewport)
      RenderTargetMode.OWNED_TEXTURE -> attachOwnedTexture(context, map, viewport)
      RenderTargetMode.BORROWED_TEXTURE -> attachBorrowedTexture(context, map, viewport)
    }

  private fun attachSurface(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    val descriptor =
      OpenGLSurfaceDescriptor(
        RenderTarget.extent(viewport),
        descriptor(context),
        NativePointer.ofAddress(context.surfaceAddress()),
      )
    return Surface(map.attachOpenGLSurface(descriptor))
  }

  private fun attachOwnedTexture(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    val descriptor =
      OpenGLOwnedTextureDescriptor(RenderTarget.extent(viewport), descriptor(context))
    var session: RenderSessionHandle? = null
    var compositor: OpenGLTextureCompositor? = null
    try {
      session = map.attachOpenGLOwnedTexture(descriptor)
      compositor = OpenGLTextureCompositor(context, viewport)
      return OwnedTexture(session, compositor)
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      RenderTarget.closeSuppressed(error, session)
      throw error
    }
  }

  private fun attachBorrowedTexture(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    var texture: OpenGLBorrowedTexture? = null
    var session: RenderSessionHandle? = null
    var compositor: OpenGLTextureCompositor? = null
    try {
      texture = OpenGLBorrowedTexture(context, viewport)
      session = map.attachOpenGLBorrowedTexture(borrowedDescriptor(context, viewport, texture))
      compositor = OpenGLTextureCompositor(context, viewport)
      return BorrowedTexture(context, session, compositor, texture)
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      RenderTarget.closeSuppressed(error, session)
      RenderTarget.closeSuppressed(error, texture)
      throw error
    }
  }

  private fun borrowedDescriptor(
    context: OpenGLContext,
    viewport: Viewport,
    texture: OpenGLBorrowedTexture,
  ): OpenGLBorrowedTextureDescriptor =
    OpenGLBorrowedTextureDescriptor(
      RenderTarget.extent(viewport),
      viewport.framebufferWidth(),
      viewport.framebufferHeight(),
      descriptor(context),
      texture.texture(),
      texture.target(),
    )

  private fun descriptor(context: OpenGLContext): OpenGLContextDescriptor =
    if (context.isGles) {
      EglContextDescriptor(
        NativePointer.ofAddress(context.eglDisplayAddress()),
        NativePointer.ofAddress(context.eglConfigAddress()),
        NativePointer.ofAddress(context.eglContextAddress()),
        NativePointer.NULL_POINTER,
      )
    } else {
      WglContextDescriptor(
        NativePointer.ofAddress(context.hdcAddress()),
        NativePointer.ofAddress(context.wglContextAddress()),
        NativePointer.NULL_POINTER,
      )
    }

  private class Surface(private val session: RenderSessionHandle) : RenderTarget {
    override fun resize(viewport: Viewport) {
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate(): Boolean = session.renderUpdate().result == RenderResult.RENDERED

    override fun close() {
      session.close()
    }
  }

  private class OwnedTexture(
    private val session: RenderSessionHandle,
    private val compositor: OpenGLTextureCompositor,
  ) : RenderTarget {
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate(): Boolean {
      if (session.renderUpdate().result != RenderResult.RENDERED) {
        return false
      }
      session.acquireOpenGLOwnedTextureFrame().use { frameHandle ->
        val frame = frameHandle.frame()
        check(frame.width() > 0 && frame.height() > 0) {
          "MapLibre returned an empty OpenGL owned texture frame"
        }
        check(frame.target() == OpenGLTextureCompositor.TEXTURE_TARGET) {
          "MapLibre owned texture target is ${frame.target()}, expected GL_TEXTURE_2D"
        }
        compositor.drawTexture(frame.texture())
      }
      return true
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        session.close()
      }
    }
  }

  private class BorrowedTexture(
    private val context: OpenGLContext,
    private val session: RenderSessionHandle,
    private val compositor: OpenGLTextureCompositor,
    private var texture: OpenGLBorrowedTexture,
  ) : RenderTarget {
    /** Local to the render loop thread: allocate a texture at the new size and hand it over. */
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      val replacement = OpenGLBorrowedTexture(context, viewport)
      try {
        session.setOpenGLBorrowedTextureTarget(borrowedDescriptor(context, viewport, replacement))
      } catch (error: RuntimeException) {
        // A failed handover leaves it unknown which target the session holds, so detach before
        // either is released.
        RenderTarget.detachSuppressed(error, session)
        RenderTarget.closeSuppressed(error, replacement)
        throw error
      }
      // Released only once the session has taken the replacement.
      texture.close()
      texture = replacement
    }

    override fun renderUpdate(): Boolean {
      if (session.renderUpdate().result != RenderResult.RENDERED) {
        return false
      }
      compositor.drawTexture(texture.texture())
      return true
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        try {
          session.close()
        } finally {
          texture.close()
        }
      }
    }
  }
}
