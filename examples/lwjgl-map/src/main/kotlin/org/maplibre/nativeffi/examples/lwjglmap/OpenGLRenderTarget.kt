package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.EglContextDescriptor
import org.maplibre.nativeffi.generated.GpuSyncKind
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.OpenglBorrowedTexture
import org.maplibre.nativeffi.generated.OpenglBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglClientApi
import org.maplibre.nativeffi.generated.OpenglContextDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptorData
import org.maplibre.nativeffi.generated.OpenglContextOwnership
import org.maplibre.nativeffi.generated.OpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglSurfaceDescriptor
import org.maplibre.nativeffi.generated.WglContextDescriptor
import org.maplibre.nativeffi.render.NativePointer

/** OpenGL sessions share the GLFW context, so the caller driver runs them on the GLFW thread. */
internal object OpenGLRenderTarget {
  fun attach(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
    mode: RenderTargetMode,
    driver: SessionDriver,
  ): RenderTarget =
    when (mode) {
      RenderTargetMode.NATIVE_SURFACE ->
        NativeSurfaceTarget(
          AttachedSession.attach(driver) { options ->
            map.openglSurfaceAttach(
              OpenglSurfaceDescriptor(
                RenderTarget.extent(viewport),
                descriptor(context),
                NativePointer.ofAddress(context.surfaceAddress()),
              ),
              options,
            )
          }
        )
      RenderTargetMode.OWNED_TEXTURE -> attachOwnedTexture(context, map, viewport, driver)
      RenderTargetMode.BORROWED_TEXTURE -> attachBorrowedTexture(context, map, viewport, driver)
    }

  private fun attachOwnedTexture(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
    driver: SessionDriver,
  ): RenderTarget {
    val compositor = OpenGLTextureCompositor(context, viewport)
    try {
      val attached =
        AttachedSession.attach(driver, RenderTarget.TEXTURE_RING_DEPTH.toUInt()) { options ->
          map.openglOwnedTextureAttach(
            OpenglOwnedTextureDescriptor(RenderTarget.extent(viewport), descriptor(context)),
            options,
          )
        }
      return OwnedTexture(attached, compositor)
    } catch (error: RuntimeException) {
      compositor.close()
      throw error
    }
  }

  private fun attachBorrowedTexture(
    context: OpenGLContext,
    map: MapHandle,
    viewport: Viewport,
    driver: SessionDriver,
  ): RenderTarget {
    val ring = BorrowedTextureTarget.allocateRing(viewport) { OpenGLBorrowedTexture(context, it) }
    var compositor: OpenGLTextureCompositor? = null
    try {
      compositor = OpenGLTextureCompositor(context, viewport)
      val attached =
        AttachedSession.attach(driver, RenderTarget.TEXTURE_RING_DEPTH.toUInt()) { options ->
          map.openglBorrowedTextureAttach(borrowedDescriptor(context, viewport, ring), options)
        }
      return BorrowedTexture(attached, map, context, compositor, ring)
    } catch (error: RuntimeException) {
      runCatching { compositor?.close() }.onFailure(error::addSuppressed)
      ring.forEach(AutoCloseable::close)
      throw error
    }
  }

  private fun borrowedDescriptor(
    context: OpenGLContext,
    viewport: Viewport,
    ring: List<OpenGLBorrowedTexture>,
  ): OpenglBorrowedTextureDescriptor =
    OpenglBorrowedTextureDescriptor(
      RenderTarget.extent(viewport),
      viewport.framebufferWidth().toUInt(),
      viewport.framebufferHeight().toUInt(),
      descriptor(context),
      ring.map { OpenglBorrowedTexture(it.texture().toUInt()) },
      OpenGLTextureCompositor.TEXTURE_TARGET.toUInt(),
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

  /** Composes a frame of either texture mode, whose producer work is complete. */
  private fun draw(compositor: OpenGLTextureCompositor, frame: AcquiredFrameHandle): Boolean {
    frame.withProducerSync { sync ->
      check(sync.kind == GpuSyncKind.CPU_COMPLETE) {
        "OpenGL compositor requires CPU-complete producer work"
      }
      frame.withOpenglTexture { view ->
        check(view.width > 0u && view.height > 0u) {
          "MapLibre returned an empty OpenGL texture frame"
        }
        check(view.target == OpenGLTextureCompositor.TEXTURE_TARGET.toUInt()) {
          "MapLibre texture target is ${view.target}, expected GL_TEXTURE_2D"
        }
        compositor.drawTexture(view.texture.toInt())
      }
    }
    return true
  }

  private class OwnedTexture(
    attached: AttachedSession,
    private val compositor: OpenGLTextureCompositor,
  ) : OwnedTextureTarget(attached) {
    override fun draw(frame: AcquiredFrameHandle): Boolean = draw(compositor, frame)

    override fun resizeHost(viewport: Viewport) {
      compositor.resize(viewport)
    }

    override fun closeHost() {
      compositor.close()
    }
  }

  private class BorrowedTexture(
    attached: AttachedSession,
    map: MapHandle,
    private val context: OpenGLContext,
    private val compositor: OpenGLTextureCompositor,
    ring: List<OpenGLBorrowedTexture>,
  ) : BorrowedTextureTarget<OpenGLBorrowedTexture>(attached, map, ring) {
    override fun allocate(viewport: Viewport): OpenGLBorrowedTexture =
      OpenGLBorrowedTexture(context, viewport)

    override fun setTarget(
      viewport: Viewport,
      replacement: List<OpenGLBorrowedTexture>,
    ): Deferred<Unit> =
      session.openglBorrowedTextureSetTarget(borrowedDescriptor(context, viewport, replacement))

    override fun draw(frame: AcquiredFrameHandle): Boolean = draw(compositor, frame)

    override fun resizeHost(viewport: Viewport) {
      compositor.resize(viewport)
    }

    override fun closeHost() {
      compositor.close()
      super.closeHost()
    }
  }
}
