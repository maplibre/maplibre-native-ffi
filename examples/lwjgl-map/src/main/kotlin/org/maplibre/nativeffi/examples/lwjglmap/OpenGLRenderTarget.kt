package org.maplibre.nativeffi.examples.lwjglmap

import org.maplibre.nativeffi.generated.EglContextDescriptor
import org.maplibre.nativeffi.generated.OpenglBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglClientApi
import org.maplibre.nativeffi.generated.OpenglContextDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptorData
import org.maplibre.nativeffi.generated.OpenglContextOwnership
import org.maplibre.nativeffi.generated.OpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglSurfaceDescriptor
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.WglContextDescriptor
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderSessionHandle

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
      OpenglSurfaceDescriptor(
        RenderTarget.extent(viewport),
        descriptor(context),
        NativePointer.ofAddress(context.surfaceAddress()),
      )
    return Surface(
      map.openglSurfaceAttach(descriptor, RenderTarget.callerDriverOptions).let { attachment ->
        RenderTarget.finishAttachment(attachment.session, attachment.ready)
      }
    )
  }

  private fun attachOwnedTexture(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    val descriptor =
      OpenglOwnedTextureDescriptor(RenderTarget.extent(viewport), descriptor(context))
    var session: RenderSessionHandle? = null
    var compositor: OpenGLTextureCompositor? = null
    try {
      session =
        map.openglOwnedTextureAttach(descriptor, RenderTarget.ownedTextureOptions).let { attachment
          ->
          RenderTarget.finishAttachment(attachment.session, attachment.ready)
        }
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
      session =
        map
          .openglBorrowedTextureAttach(
            borrowedDescriptor(context, viewport, texture),
            RenderTarget.callerDriverOptions,
          )
          .let { attachment -> RenderTarget.finishAttachment(attachment.session, attachment.ready) }
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
  ): OpenglBorrowedTextureDescriptor =
    OpenglBorrowedTextureDescriptor(
      RenderTarget.extent(viewport),
      viewport.framebufferWidth().toUInt(),
      viewport.framebufferHeight().toUInt(),
      descriptor(context),
      texture.texture().toUInt(),
      texture.target().toUInt(),
    )

  private fun descriptor(context: OpenGLContext): OpenglContextDescriptor =
    OpenglContextDescriptor(
      ownership = OpenglContextOwnership.SHARED,
      data =
        if (context.isGles) {
          OpenglContextDescriptorData.Egl(
            EglContextDescriptor(
              display = NativePointer.ofAddress(context.eglDisplayAddress()),
              config = NativePointer.ofAddress(context.eglConfigAddress()),
              shareContext = NativePointer.ofAddress(context.eglContextAddress()),
              clientApi = OpenglClientApi.GLES,
              getProcAddress = NativePointer.NULL_POINTER,
            )
          )
        } else {
          OpenglContextDescriptorData.Wgl(
            WglContextDescriptor(
              deviceContext = NativePointer.ofAddress(context.hdcAddress()),
              shareContext = NativePointer.ofAddress(context.wglContextAddress()),
              getProcAddress = NativePointer.NULL_POINTER,
            )
          )
        },
    )

  private class Surface(private val session: RenderSessionHandle) : RenderTarget {
    override fun resize(viewport: Viewport) {
      RenderTarget.completeDriverOperation(session, session.resize(RenderTarget.extent(viewport)))
    }

    override fun renderUpdate(): Boolean {
      val result = RenderTarget.renderFrame(session) ?: return false
      return result.disposition == RenderResult.RENDERED && !result.needsRepaint
    }

    override fun close() {
      RenderTarget.closeSession(session)
    }
  }

  private class OwnedTexture(
    private val session: RenderSessionHandle,
    private val compositor: OpenGLTextureCompositor,
  ) : RenderTarget {
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      RenderTarget.completeDriverOperation(session, session.resize(RenderTarget.extent(viewport)))
    }

    override fun renderUpdate(): Boolean {
      val result = RenderTarget.renderFrame(session) ?: return false
      if (result.disposition != RenderResult.RENDERED) return false
      // An empty ring keeps the previously composited frame on screen.
      val frameHandle =
        try {
          session.acquireFrame()
        } catch (error: org.maplibre.nativeffi.error.MaplibreException) {
          if (error.status == org.maplibre.nativeffi.error.MaplibreStatus.NOT_READY) return false
          throw error
        }
      try {
        frameHandle.withGetProducerSync { sync ->
          check(sync.kind == org.maplibre.nativeffi.generated.GpuSyncKind.CPU_COMPLETE) {
            "OpenGL compositor requires CPU-complete producer work"
          }
          frameHandle.withGetOpenglTexture { frame ->
            check(frame.width > 0u && frame.height > 0u) {
              "MapLibre returned an empty OpenGL owned texture frame"
            }
            check(frame.target == OpenGLTextureCompositor.TEXTURE_TARGET.toUInt()) {
              "MapLibre owned texture target is ${frame.target}, expected GL_TEXTURE_2D"
            }
            compositor.drawTexture(frame.texture.toInt())
          }
        }
      } finally {
        frameHandle.release()
      }
      return !result.needsRepaint
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        RenderTarget.closeSession(session)
      }
    }
  }

  private class BorrowedTexture(
    private val context: OpenGLContext,
    private val session: RenderSessionHandle,
    private val compositor: OpenGLTextureCompositor,
    private var texture: OpenGLBorrowedTexture,
  ) : RenderTarget {
    private var sessionReleased = false

    /** Local to the render loop thread: allocate a texture at the new size and hand it over. */
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      val replacement = OpenGLBorrowedTexture(context, viewport)
      try {
        RenderTarget.completeDriverOperation(
          session,
          session.openglBorrowedTextureSetTarget(borrowedDescriptor(context, viewport, replacement)),
        )
      } catch (error: RuntimeException) {
        // A failed handover leaves it unknown which target the session holds, so detach before
        // either is released.
        RenderTarget.detachSuppressed(error, session)
        sessionReleased = true
        RenderTarget.closeSuppressed(error, replacement)
        throw error
      }
      // Released only once the session has taken the replacement.
      texture.close()
      texture = replacement
      // A handover replaces only the graphics resource, so the map still needs the new
      // extent.
      RenderTarget.resizeMap(session.map(), viewport)
    }

    override fun renderUpdate(): Boolean {
      val result = RenderTarget.renderFrame(session) ?: return false
      if (result.disposition != RenderResult.RENDERED) return false
      compositor.drawTexture(texture.texture())
      return !result.needsRepaint
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        try {
          RenderTarget.closeSession(session, sessionReleased)
        } finally {
          texture.close()
        }
      }
    }
  }
}
