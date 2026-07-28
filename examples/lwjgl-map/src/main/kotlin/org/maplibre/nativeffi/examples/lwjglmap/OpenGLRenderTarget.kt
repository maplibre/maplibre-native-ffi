package org.maplibre.nativeffi.examples.lwjglmap

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLContextDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
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
      val descriptor =
        OpenGLBorrowedTextureDescriptor(
          RenderTarget.extent(viewport),
          viewport.framebufferWidth(),
          viewport.framebufferHeight(),
          descriptor(context),
          texture.texture(),
          texture.target(),
        )
      session = map.attachOpenGLBorrowedTexture(descriptor)
      compositor = OpenGLTextureCompositor(context, viewport)
      return BorrowedTexture(context, map, session, compositor, texture)
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      RenderTarget.closeSuppressed(error, session)
      RenderTarget.closeSuppressed(error, texture)
      throw error
    }
  }

  private fun descriptor(context: OpenGLContext): OpenGLContextDescriptor =
    if (context.isGles) {
      EglContextDescriptor(
        NativePointer.ofAddress(context.eglDisplayAddress()),
        NativePointer.ofAddress(context.eglConfigAddress()),
        NativePointer.ofAddress(context.eglContextAddress()),
        NativePointer.NULL,
      )
    } else {
      WglContextDescriptor(
        NativePointer.ofAddress(context.hdcAddress()),
        NativePointer.ofAddress(context.wglContextAddress()),
        NativePointer.NULL,
      )
    }

  private class Surface(private val session: RenderSessionHandle) : RenderTarget {
    override fun resize(viewport: Viewport) {
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate(): Boolean = session.renderUpdate()

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
      if (!session.renderUpdate()) {
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
    private val map: MapHandle,
    private var session: RenderSessionHandle?,
    private var compositor: OpenGLTextureCompositor?,
    private var texture: OpenGLBorrowedTexture?,
  ) : RenderTarget {
    override fun needsReattachOnResize(): Boolean = true

    /** Local to the render loop thread: close the session, rebuild the texture, attach again. */
    override fun reattach(viewport: Viewport) {
      close()
      val replacement = attachBorrowedTexture(context, map, viewport)
      check(replacement is BorrowedTexture) { "unexpected borrowed texture replacement" }
      session = replacement.session
      compositor = replacement.compositor
      texture = replacement.texture
      replacement.session = null
      replacement.compositor = null
      replacement.texture = null
    }

    override fun resize(viewport: Viewport) {
      error("borrowed texture resize requires render target reattachment")
    }

    override fun renderUpdate(): Boolean {
      val currentSession = checkNotNull(session) { "OpenGL borrowed texture session is detached" }
      val currentCompositor =
        checkNotNull(compositor) { "OpenGL borrowed texture compositor is detached" }
      val currentTexture = checkNotNull(texture) { "OpenGL borrowed texture is detached" }
      if (!currentSession.renderUpdate()) {
        return false
      }
      currentCompositor.drawTexture(currentTexture.texture())
      return true
    }

    override fun close() {
      val closingCompositor = compositor
      val closingSession = session
      val closingTexture = texture
      compositor = null
      session = null
      texture = null
      try {
        closingCompositor?.close()
      } finally {
        try {
          closingSession?.close()
        } finally {
          closingTexture?.close()
        }
      }
    }
  }
}
