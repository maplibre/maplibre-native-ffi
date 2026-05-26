package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor

@OptIn(ExperimentalForeignApi::class)
class RenderStructsTest {
  @Test
  fun renderBackendMasksRoundTrip() {
    assertEquals(setOf(RenderBackend.METAL, RenderBackend.VULKAN), RenderBackend.fromMask(3U))
    assertEquals(emptySet(), RenderBackend.fromMask(0U))
  }

  @Test
  fun metalDescriptorsMaterializeOpaquePointersAndExtents() {
    memScoped {
      val extent = RenderTargetExtent(640, 480, 2.0)
      val owned =
        RenderStructs.metalOwnedTextureDescriptor(
            MetalOwnedTextureDescriptor(
              extent,
              MetalContextDescriptor(NativePointer.ofAddress(0x10L)),
            ),
            this,
          )
          .pointed
      assertEquals(640U, owned.extent.width)
      assertEquals(2.0, owned.extent.scale_factor)
      assertFalse(owned.context.device == null)

      val borrowed =
        RenderStructs.metalBorrowedTextureDescriptor(
            MetalBorrowedTextureDescriptor(extent, NativePointer.ofAddress(0x20L)),
            this,
          )
          .pointed
      assertFalse(borrowed.texture == null)

      val surface =
        RenderStructs.metalSurfaceDescriptor(
            MetalSurfaceDescriptor(
              extent,
              MetalContextDescriptor(),
              NativePointer.ofAddress(0x30L),
            ),
            this,
          )
          .pointed
      assertFalse(surface.layer == null)
    }
  }

  @Test
  fun vulkanDescriptorsMaterializeContextPointersAndOptionalFinalLayout() {
    memScoped {
      val context =
        VulkanContextDescriptor(
          NativePointer.ofAddress(0x10L),
          NativePointer.ofAddress(0x20L),
          NativePointer.ofAddress(0x30L),
          NativePointer.ofAddress(0x40L),
          7,
        )
      val borrowed =
        RenderStructs.vulkanBorrowedTextureDescriptor(
            VulkanBorrowedTextureDescriptor(
                RenderTargetExtent(),
                context,
                NativePointer.ofAddress(0x50L),
              )
              .imageView(NativePointer.ofAddress(0x60L))
              .format(44)
              .initialLayout(1)
              .finalLayout(2),
            this,
          )
          .pointed
      assertFalse(borrowed.context.instance == null)
      assertEquals(7U, borrowed.context.graphics_queue_family_index)
      assertEquals(44U, borrowed.format)
      assertEquals(2U, borrowed.final_layout)

      val surface =
        RenderStructs.vulkanSurfaceDescriptor(
            VulkanSurfaceDescriptor(RenderTargetExtent(), context, NativePointer.ofAddress(0x70L)),
            this,
          )
          .pointed
      assertFalse(surface.surface == null)
    }
  }
}
