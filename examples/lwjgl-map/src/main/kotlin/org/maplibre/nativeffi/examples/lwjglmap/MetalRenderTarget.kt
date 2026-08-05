package org.maplibre.nativeffi.examples.lwjglmap

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderSessionHandle

internal object MetalRenderTarget {
  fun attach(
    context: MetalContext,
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
    context: MetalContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    val descriptor =
      MetalSurfaceDescriptor(
        RenderTarget.extent(viewport),
        descriptor(context),
        NativePointer.ofAddress(context.layerAddress()),
      )
    return Surface(map.attachMetalSurface(descriptor))
  }

  private fun attachOwnedTexture(
    context: MetalContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    val descriptor = MetalOwnedTextureDescriptor(RenderTarget.extent(viewport), descriptor(context))
    var session: RenderSessionHandle? = null
    var compositor: MetalTextureCompositor? = null
    try {
      session = map.attachMetalOwnedTexture(descriptor)
      compositor = MetalTextureCompositor(context)
      return OwnedTexture(session, compositor)
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      RenderTarget.closeSuppressed(error, session)
      throw error
    }
  }

  private fun attachBorrowedTexture(
    context: MetalContext,
    map: MapHandle,
    viewport: Viewport,
  ): RenderTarget {
    var texture: MetalBorrowedTexture? = null
    var session: RenderSessionHandle? = null
    var compositor: MetalTextureCompositor? = null
    try {
      texture = MetalBorrowedTexture(context, viewport)
      session = map.attachMetalBorrowedTexture(borrowedDescriptor(viewport, texture))
      compositor = MetalTextureCompositor(context)
      return BorrowedTexture(context, session, compositor, texture)
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      RenderTarget.closeSuppressed(error, session)
      RenderTarget.closeSuppressed(error, texture)
      throw error
    }
  }

  private fun borrowedDescriptor(
    viewport: Viewport,
    texture: MetalBorrowedTexture,
  ): MetalBorrowedTextureDescriptor =
    MetalBorrowedTextureDescriptor(
      RenderTarget.extent(viewport),
      viewport.framebufferWidth(),
      viewport.framebufferHeight(),
      NativePointer.ofAddress(texture.texture()),
    )

  private fun descriptor(context: MetalContext): MetalContextDescriptor =
    MetalContextDescriptor(NativePointer.ofAddress(context.deviceAddress()))

  private class Surface(private val session: RenderSessionHandle) : RenderTarget {
    override fun needsMetalAutoreleasePool(): Boolean = true

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
    private val compositor: MetalTextureCompositor,
  ) : RenderTarget {
    override fun needsMetalAutoreleasePool(): Boolean = true

    override fun resize(viewport: Viewport) {
      session.resize(viewport.width(), viewport.height(), viewport.scaleFactor())
    }

    override fun renderUpdate(): Boolean {
      if (!session.renderUpdate()) {
        return false
      }
      return session.acquireMetalOwnedTextureFrame().use { frameHandle ->
        val frame = frameHandle.frame()
        check(frame.width() != 0 && frame.height() != 0 && !frame.texture().isNull) {
          "owned Metal frame has an empty extent or null texture"
        }
        compositor.drawTexture(frame.texture().address)
      }
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
    private val context: MetalContext,
    private val session: RenderSessionHandle,
    private val compositor: MetalTextureCompositor,
    private var texture: MetalBorrowedTexture,
  ) : RenderTarget {
    override fun needsMetalAutoreleasePool(): Boolean = true

    /** Local to the render loop thread: allocate a texture at the new size and hand it over. */
    override fun resize(viewport: Viewport) {
      val replacement = MetalBorrowedTexture(context, viewport)
      try {
        session.setMetalBorrowedTextureTarget(borrowedDescriptor(viewport, replacement))
      } catch (error: RuntimeException) {
        // A failed handover leaves it unknown which texture the session holds, so detach before
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
      if (!session.renderUpdate()) {
        return false
      }
      return compositor.drawTexture(texture.texture())
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
