package org.maplibre.nativeffi.examples.lwjglmap

import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.GpuSyncKind
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.generated.MetalContextDescriptor
import org.maplibre.nativeffi.generated.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.MetalSurfaceDescriptor
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.render.NativePointer

internal object MetalRenderTarget {
  fun attach(
    context: MetalContext,
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
    context: MetalContext,
    map: MapHandle,
    viewport: Viewport,
    wakes: LoopWakes,
  ): RenderTarget {
    val descriptor =
      MetalSurfaceDescriptor(
        RenderTarget.extent(viewport),
        descriptor(context),
        NativePointer.ofAddress(context.layerAddress()),
      )
    return Surface(RenderTarget.attached(map.metalSurfaceAttach(descriptor, wakes.attachOptions())))
  }

  private fun attachOwnedTexture(
    context: MetalContext,
    map: MapHandle,
    viewport: Viewport,
    wakes: LoopWakes,
  ): RenderTarget {
    val descriptor = MetalOwnedTextureDescriptor(RenderTarget.extent(viewport), descriptor(context))
    val compositor = MetalTextureCompositor(context)
    try {
      return OwnedTexture(
        RenderTarget.attached(
          map.metalOwnedTextureAttach(
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
    context: MetalContext,
    map: MapHandle,
    viewport: Viewport,
    wakes: LoopWakes,
  ): RenderTarget {
    val texture = MetalBorrowedTexture(context, viewport)
    var compositor: MetalTextureCompositor? = null
    try {
      compositor = MetalTextureCompositor(context)
      val descriptor = borrowedDescriptor(viewport, texture)
      return BorrowedTexture(
        context,
        map,
        RenderTarget.attached(map.metalBorrowedTextureAttach(descriptor, wakes.attachOptions())),
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

  private class Surface(session: RenderSessionHandle) : RenderTarget(session) {
    override fun needsMetalAutoreleasePool(): Boolean = true
  }

  private class OwnedTexture(
    session: RenderSessionHandle,
    private val compositor: MetalTextureCompositor,
  ) : RenderTarget(session) {
    override fun needsMetalAutoreleasePool(): Boolean = true

    override fun present(): Boolean {
      // An empty ring keeps the previously composited frame on screen.
      val frameHandle =
        try {
          session.acquireFrame()
        } catch (error: MaplibreException) {
          if (error.status == MaplibreStatus.NOT_READY) return true
          throw error
        }
      try {
        return frameHandle.withGetProducerSync { sync ->
          check(sync.kind == GpuSyncKind.CPU_COMPLETE) {
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
    private val context: MetalContext,
    private val map: MapHandle,
    session: RenderSessionHandle,
    private val compositor: MetalTextureCompositor,
    private var texture: MetalBorrowedTexture,
  ) : RenderTarget(session) {
    override fun needsMetalAutoreleasePool(): Boolean = true

    /** Allocates a texture at the new size and hands it to the live session. */
    override fun resize(viewport: Viewport) {
      val replacement = MetalBorrowedTexture(context, viewport)
      try {
        handOver {
          session.metalBorrowedTextureSetTarget(borrowedDescriptor(viewport, replacement))
        }
      } catch (error: RuntimeException) {
        RenderTarget.closeSuppressed(error, replacement)
        throw error
      }
      texture.close()
      texture = replacement
      RenderTarget.resizeMap(map, viewport)
    }

    override fun present(): Boolean = compositor.drawTexture(texture.texture())

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
