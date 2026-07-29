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
      val descriptor =
        MetalBorrowedTextureDescriptor(
          RenderTarget.extent(viewport),
          viewport.framebufferWidth(),
          viewport.framebufferHeight(),
          NativePointer.ofAddress(texture.texture()),
        )
      session = map.attachMetalBorrowedTexture(descriptor)
      compositor = MetalTextureCompositor(context)
      return BorrowedTexture(context, map, session, compositor, texture)
    } catch (error: RuntimeException) {
      RenderTarget.closeSuppressed(error, compositor)
      RenderTarget.closeSuppressed(error, session)
      RenderTarget.closeSuppressed(error, texture)
      throw error
    }
  }

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
      session.acquireMetalOwnedTextureFrame().use { frameHandle ->
        val frame = frameHandle.frame()
        check(frame.width() != 0 && frame.height() != 0 && !frame.texture().isNull) {
          "owned Metal frame has an empty extent or null texture"
        }
        compositor.drawTexture(frame.texture().address)
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
    private val context: MetalContext,
    private val map: MapHandle,
    private var session: RenderSessionHandle?,
    private var compositor: MetalTextureCompositor?,
    private var texture: MetalBorrowedTexture?,
  ) : RenderTarget {
    override fun needsMetalAutoreleasePool(): Boolean = true

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
      val currentSession = checkNotNull(session) { "Metal borrowed texture session is detached" }
      val currentCompositor =
        checkNotNull(compositor) { "Metal borrowed texture compositor is detached" }
      val currentTexture = checkNotNull(texture) { "Metal borrowed texture is detached" }
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
