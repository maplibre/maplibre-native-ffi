package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.GpuSyncKind
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MetalBorrowedTexture as MetalRingTexture
import org.maplibre.nativeffi.generated.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.MetalContextDescriptor
import org.maplibre.nativeffi.generated.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.NativePointer

internal object MetalRenderTarget {
  fun attach(
    context: MetalContext,
    map: MapHandle,
    viewport: Viewport,
    mode: RenderTargetMode,
    driver: SessionDriver,
  ): RenderTarget =
    when (mode) {
      RenderTargetMode.NATIVE_SURFACE ->
        NativeSurfaceTarget(
          AttachedSession.attach(driver) { options ->
            map.metalSurfaceAttach(
              MetalSurfaceDescriptor(
                RenderTarget.extent(viewport),
                descriptor(context),
                NativePointer.ofAddress(context.layerAddress()),
              ),
              options,
            )
          }
        )
      RenderTargetMode.OWNED_TEXTURE -> attachOwnedTexture(context, map, viewport, driver)
      RenderTargetMode.BORROWED_TEXTURE -> attachBorrowedTexture(context, map, viewport, driver)
    }

  private fun attachOwnedTexture(
    context: MetalContext,
    map: MapHandle,
    viewport: Viewport,
    driver: SessionDriver,
  ): RenderTarget {
    context.resizeDrawable(viewport)
    val compositor = MetalTextureCompositor(context)
    try {
      val attached =
        AttachedSession.attach(driver, RenderTarget.TEXTURE_RING_DEPTH.toUInt()) { options ->
          map.metalOwnedTextureAttach(
            MetalOwnedTextureDescriptor(RenderTarget.extent(viewport), descriptor(context)),
            options,
          )
        }
      return OwnedTexture(attached, context, compositor)
    } catch (error: RuntimeException) {
      compositor.close()
      throw error
    }
  }

  private fun attachBorrowedTexture(
    context: MetalContext,
    map: MapHandle,
    viewport: Viewport,
    driver: SessionDriver,
  ): RenderTarget {
    context.resizeDrawable(viewport)
    val ring = BorrowedTextureTarget.allocateRing(viewport) { MetalBorrowedTexture(context, it) }
    var compositor: MetalTextureCompositor? = null
    try {
      compositor = MetalTextureCompositor(context)
      val attached =
        AttachedSession.attach(driver, RenderTarget.TEXTURE_RING_DEPTH.toUInt()) { options ->
          map.metalBorrowedTextureAttach(borrowedDescriptor(viewport, ring), options)
        }
      return BorrowedTexture(attached, map, context, compositor, ring)
    } catch (error: RuntimeException) {
      runCatching { compositor?.close() }.onFailure(error::addSuppressed)
      ring.forEach(AutoCloseable::close)
      throw error
    }
  }

  private fun borrowedDescriptor(
    viewport: Viewport,
    ring: List<MetalBorrowedTexture>,
  ): MetalBorrowedTextureDescriptor =
    MetalBorrowedTextureDescriptor(
      RenderTarget.extent(viewport),
      viewport.framebufferWidth().toUInt(),
      viewport.framebufferHeight().toUInt(),
      ring.map { MetalRingTexture(NativePointer.ofAddress(it.texture())) },
    )

  /** Composes a frame of either texture mode, whose producer work is complete. */
  private fun draw(compositor: MetalTextureCompositor, frame: AcquiredFrameHandle): Boolean =
    frame.withProducerSync { sync ->
      check(sync.kind == GpuSyncKind.CPU_COMPLETE) {
        "Metal compositor requires CPU-complete producer work"
      }
      frame.withMetalTexture { view ->
        check(view.width != 0u && view.height != 0u && !view.texture.isNull) {
          "Metal frame has an empty extent or null texture"
        }
        compositor.drawTexture(view.texture.address)
      }
    }

  private fun descriptor(context: MetalContext): MetalContextDescriptor =
    MetalContextDescriptor(NativePointer.ofAddress(context.deviceAddress()))

  private class OwnedTexture(
    attached: AttachedSession,
    private val context: MetalContext,
    private val compositor: MetalTextureCompositor,
  ) : OwnedTextureTarget(attached) {
    override fun draw(frame: AcquiredFrameHandle): Boolean = draw(compositor, frame)

    override fun resizeHost(viewport: Viewport) {
      context.resizeDrawable(viewport)
    }

    override fun closeHost() {
      compositor.close()
    }
  }

  private class BorrowedTexture(
    attached: AttachedSession,
    map: MapHandle,
    private val context: MetalContext,
    private val compositor: MetalTextureCompositor,
    ring: List<MetalBorrowedTexture>,
  ) : BorrowedTextureTarget<MetalBorrowedTexture>(attached, map, ring) {
    override fun allocate(viewport: Viewport): MetalBorrowedTexture =
      MetalBorrowedTexture(context, viewport)

    override fun setTarget(
      viewport: Viewport,
      replacement: List<MetalBorrowedTexture>,
    ): Deferred<Unit> =
      session.metalBorrowedTextureSetTarget(borrowedDescriptor(viewport, replacement))

    override fun draw(frame: AcquiredFrameHandle): Boolean = draw(compositor, frame)

    override fun resizeHost(viewport: Viewport) {
      context.resizeDrawable(viewport)
    }

    override fun closeHost() {
      compositor.close()
      super.closeHost()
    }
  }
}
