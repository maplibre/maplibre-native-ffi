package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.GpuSyncKind
import org.maplibre.nativeffi.generated.MapHandle
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
        AttachedSession.attach(driver, RenderTarget.OWNED_TEXTURE_RING_DEPTH) { options ->
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
    val texture = MetalBorrowedTexture(context, viewport)
    var compositor: MetalTextureCompositor? = null
    try {
      compositor = MetalTextureCompositor(context)
      val attached =
        AttachedSession.attach(driver) { options ->
          map.metalBorrowedTextureAttach(borrowedDescriptor(viewport, texture), options)
        }
      return BorrowedTexture(attached, map, context, compositor, texture)
    } catch (error: RuntimeException) {
      runCatching { compositor?.close() }.onFailure(error::addSuppressed)
      texture.close()
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

  private class OwnedTexture(
    attached: AttachedSession,
    private val context: MetalContext,
    private val compositor: MetalTextureCompositor,
  ) : OwnedTextureTarget(attached) {
    override fun draw(frame: AcquiredFrameHandle): Boolean = frame.withGetProducerSync { sync ->
      check(sync.kind == GpuSyncKind.CPU_COMPLETE) {
        "Metal compositor requires CPU-complete producer work"
      }
      frame.withGetMetalTexture { view ->
        check(view.width != 0u && view.height != 0u && !view.texture.isNull) {
          "owned Metal frame has an empty extent or null texture"
        }
        compositor.drawTexture(view.texture.address)
      }
    }

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
    texture: MetalBorrowedTexture,
  ) : BorrowedTextureTarget<MetalBorrowedTexture>(attached, map, texture) {
    override fun allocate(viewport: Viewport): MetalBorrowedTexture =
      MetalBorrowedTexture(context, viewport)

    override fun setTarget(viewport: Viewport, replacement: MetalBorrowedTexture): Deferred<Unit> =
      session.metalBorrowedTextureSetTarget(borrowedDescriptor(viewport, replacement))

    override fun draw(texture: MetalBorrowedTexture): Boolean =
      compositor.drawTexture(texture.texture())

    override fun resizeHost(viewport: Viewport) {
      context.resizeDrawable(viewport)
    }

    override fun closeHost() {
      compositor.close()
      texture.close()
    }
  }
}
