package org.maplibre.nativeffi.examples.lwjglmap

import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.EglContextDescriptor
import org.maplibre.nativeffi.generated.GpuSyncKind
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.OpenglBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglClientApi
import org.maplibre.nativeffi.generated.OpenglContextDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptorData
import org.maplibre.nativeffi.generated.OpenglContextOwnership
import org.maplibre.nativeffi.generated.OpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglSurfaceDescriptor
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.WglContextDescriptor
import org.maplibre.nativeffi.render.NativePointer

internal object OpenGLRenderTarget {
  fun attach(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
    mode: RenderTargetMode,
    wakes: LoopWakes,
  ): RenderTarget =
    when (mode) {
      RenderTargetMode.NATIVE_SURFACE -> attachSurface(context, map, viewport, wakes)
      RenderTargetMode.OWNED_TEXTURE -> attachOwnedTexture(context, map, viewport, wakes)
      RenderTargetMode.BORROWED_TEXTURE -> attachBorrowedTexture(context, map, viewport, wakes)
    }

  private fun attachSurface(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
    wakes: LoopWakes,
  ): RenderTarget {
    val descriptor =
      OpenglSurfaceDescriptor(
        RenderTarget.extent(viewport),
        descriptor(context),
        NativePointer.ofAddress(context.surfaceAddress()),
      )
    return RenderTarget(
      RenderTarget.attached(map.openglSurfaceAttach(descriptor, wakes.attachOptions()))
    )
  }

  private fun attachOwnedTexture(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
    wakes: LoopWakes,
  ): RenderTarget {
    val descriptor =
      OpenglOwnedTextureDescriptor(RenderTarget.extent(viewport), descriptor(context))
    val compositor = OpenGLTextureCompositor(context, viewport)
    try {
      return OwnedTexture(
        RenderTarget.attached(
          map.openglOwnedTextureAttach(
            descriptor,
            wakes.attachOptions(RenderTarget.OWNED_TEXTURE_RING_DEPTH),
          )
        ),
        compositor,
      )
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      throw error
    }
  }

  private fun attachBorrowedTexture(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
    wakes: LoopWakes,
  ): RenderTarget {
    val texture = OpenGLBorrowedTexture(context, viewport)
    var compositor: OpenGLTextureCompositor? = null
    try {
      compositor = OpenGLTextureCompositor(context, viewport)
      val descriptor = borrowedDescriptor(context, viewport, texture)
      return BorrowedTexture(
        context,
        map,
        RenderTarget.attached(map.openglBorrowedTextureAttach(descriptor, wakes.attachOptions())),
        compositor,
        texture,
      )
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
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

  private class OwnedTexture(
    session: RenderSessionHandle,
    private val compositor: OpenGLTextureCompositor,
  ) : RenderTarget(session) {
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      super.resize(viewport)
    }

    override fun present(): Boolean {
      // An empty ring leaves the previously composited frame on screen, and nothing new reaches
      // the window.
      val frameHandle =
        try {
          session.acquireFrame()
        } catch (error: MaplibreException) {
          if (error.status == MaplibreStatus.NOT_READY) return false
          throw error
        }
      try {
        frameHandle.withGetProducerSync { sync ->
          check(sync.kind == GpuSyncKind.CPU_COMPLETE) {
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
      return true
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        super.close()
      }
    }
  }

  private class BorrowedTexture(
    private val context: OpenGLContext,
    private val map: MapHandle,
    session: RenderSessionHandle,
    private val compositor: OpenGLTextureCompositor,
    private var texture: OpenGLBorrowedTexture,
  ) : RenderTarget(session) {
    /** Allocates a texture at the new size and hands it to the live session. */
    override fun resize(viewport: Viewport) {
      compositor.resize(viewport)
      val replacement = OpenGLBorrowedTexture(context, viewport)
      try {
        handOver {
          session.openglBorrowedTextureSetTarget(borrowedDescriptor(context, viewport, replacement))
        }
      } catch (error: RuntimeException) {
        RenderTarget.closeSuppressed(error, replacement)
        throw error
      }
      texture.close()
      texture = replacement
      RenderTarget.resizeMap(map, viewport)
    }

    override fun present(): Boolean {
      compositor.drawTexture(texture.texture())
      return true
    }

    override fun close() {
      try {
        compositor.close()
      } finally {
        try {
          super.close()
        } finally {
          texture.close()
        }
      }
    }
  }
}
