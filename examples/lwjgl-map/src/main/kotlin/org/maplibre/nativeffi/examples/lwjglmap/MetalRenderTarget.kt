package org.maplibre.nativeffi.examples.lwjglmap

import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.MetalContextDescriptor
import org.maplibre.nativeffi.generated.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.MetalSurfaceDescriptor
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.render.NativePointer

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
    return Surface(
      map.metalSurfaceAttach(descriptor, RenderTarget.callerDriverOptions).let { attachment ->
        RenderTarget.finishAttachment(attachment.session, attachment.ready)
      }
    )
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
      session =
        map.metalOwnedTextureAttach(descriptor, RenderTarget.ownedTextureOptions).let { attachment
          ->
          RenderTarget.finishAttachment(attachment.session, attachment.ready)
        }
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
      session =
        map
          .metalBorrowedTextureAttach(
            borrowedDescriptor(viewport, texture),
            RenderTarget.callerDriverOptions,
          )
          .let { attachment -> RenderTarget.finishAttachment(attachment.session, attachment.ready) }
      compositor = MetalTextureCompositor(context)
      return BorrowedTexture(context, map, session, compositor, texture)
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
      viewport.framebufferWidth().toUInt(),
      viewport.framebufferHeight().toUInt(),
      NativePointer.ofAddress(texture.texture()),
    )

  private fun descriptor(context: MetalContext): MetalContextDescriptor =
    MetalContextDescriptor(NativePointer.ofAddress(context.deviceAddress()))

  private class Surface(private val session: RenderSessionHandle) : RenderTarget {
    override fun needsMetalAutoreleasePool(): Boolean = true

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
    private val compositor: MetalTextureCompositor,
  ) : RenderTarget {
    override fun needsMetalAutoreleasePool(): Boolean = true

    override fun resize(viewport: Viewport) {
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
      val presented =
        try {
          frameHandle.withGetProducerSync { sync ->
            check(sync.kind == org.maplibre.nativeffi.generated.GpuSyncKind.CPU_COMPLETE) {
              "Metal compositor requires CPU-complete producer work"
            }
            frameHandle.withGetMetalTexture { frame ->
              check(frame.width != 0u && frame.height != 0u && !frame.texture.isNull) {
                "owned Metal frame has an empty extent or null texture"
              }
              compositor.drawTexture(frame.texture.address)
            }
          }
        } finally {
          frameHandle.release()
        }
      return presented && !result.needsRepaint
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
    private val context: MetalContext,
    private val map: MapHandle,
    private val session: RenderSessionHandle,
    private val compositor: MetalTextureCompositor,
    private var texture: MetalBorrowedTexture,
  ) : RenderTarget {
    private var sessionReleased = false

    override fun needsMetalAutoreleasePool(): Boolean = true

    /** Local to the render loop thread: allocate a texture at the new size and hand it over. */
    override fun resize(viewport: Viewport) {
      val replacement = MetalBorrowedTexture(context, viewport)
      try {
        RenderTarget.completeDriverOperation(
          session,
          session.metalBorrowedTextureSetTarget(borrowedDescriptor(viewport, replacement)),
        )
      } catch (error: RuntimeException) {
        // A failed handover leaves it unknown which texture the session holds, so detach before
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
      RenderTarget.resizeMap(map, viewport)
    }

    override fun renderUpdate(): Boolean {
      val result = RenderTarget.renderFrame(session) ?: return false
      if (result.disposition != RenderResult.RENDERED) return false
      return compositor.drawTexture(texture.texture()) && !result.needsRepaint
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
