package org.maplibre.nativeffi.internal.struct;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_surface_descriptor;
import org.maplibre.nativeffi.internal.c.mln_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_render_target_extent;
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_descriptor;
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalContextDescriptor;
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor;
import org.maplibre.nativeffi.render.NativePointer;
import org.maplibre.nativeffi.render.OwnedTextureDescriptor;
import org.maplibre.nativeffi.render.RenderTargetExtent;
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanContextDescriptor;
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor;

final class RenderStructsTest {
  @Test
  void renderTargetDescriptorsMaterializeNestedSizesAndFields() {
    var extent = new RenderTargetExtent(640, 480, 2.0);
    var metalContext = new MetalContextDescriptor(NativePointer.ofAddress(0x1234));
    var vulkanContext =
        new VulkanContextDescriptor(
            NativePointer.ofAddress(0x10),
            NativePointer.ofAddress(0x20),
            NativePointer.ofAddress(0x30),
            NativePointer.ofAddress(0x40),
            7);

    try (var arena = Arena.ofConfined()) {
      var ownedTexture =
          RenderStructs.ownedTextureDescriptor(new OwnedTextureDescriptor().extent(extent), arena);
      assertEquals(
          (int) mln_owned_texture_descriptor.sizeof(),
          mln_owned_texture_descriptor.size(ownedTexture));
      assertExtent(extent, mln_owned_texture_descriptor.extent(ownedTexture));

      var metalOwnedTexture =
          RenderStructs.metalOwnedTextureDescriptor(
              new MetalOwnedTextureDescriptor().extent(extent).context(metalContext), arena);
      assertEquals(
          (int) mln_metal_owned_texture_descriptor.sizeof(),
          mln_metal_owned_texture_descriptor.size(metalOwnedTexture));
      assertExtent(extent, mln_metal_owned_texture_descriptor.extent(metalOwnedTexture));
      assertMetalContext(
          metalContext, mln_metal_owned_texture_descriptor.context(metalOwnedTexture));

      var metalBorrowedTexture =
          RenderStructs.metalBorrowedTextureDescriptor(
              new MetalBorrowedTextureDescriptor()
                  .extent(extent)
                  .texture(NativePointer.ofAddress(0x50)),
              arena);
      assertEquals(
          (int) mln_metal_borrowed_texture_descriptor.sizeof(),
          mln_metal_borrowed_texture_descriptor.size(metalBorrowedTexture));
      assertExtent(extent, mln_metal_borrowed_texture_descriptor.extent(metalBorrowedTexture));
      assertPointer(0x50, mln_metal_borrowed_texture_descriptor.texture(metalBorrowedTexture));

      var metalSurface =
          RenderStructs.metalSurfaceDescriptor(
              new MetalSurfaceDescriptor()
                  .extent(extent)
                  .context(metalContext)
                  .layer(NativePointer.ofAddress(0x60)),
              arena);
      assertEquals(
          (int) mln_metal_surface_descriptor.sizeof(),
          mln_metal_surface_descriptor.size(metalSurface));
      assertExtent(extent, mln_metal_surface_descriptor.extent(metalSurface));
      assertMetalContext(metalContext, mln_metal_surface_descriptor.context(metalSurface));
      assertPointer(0x60, mln_metal_surface_descriptor.layer(metalSurface));

      var vulkanOwnedTexture =
          RenderStructs.vulkanOwnedTextureDescriptor(
              new VulkanOwnedTextureDescriptor().extent(extent).context(vulkanContext), arena);
      assertEquals(
          (int) mln_vulkan_owned_texture_descriptor.sizeof(),
          mln_vulkan_owned_texture_descriptor.size(vulkanOwnedTexture));
      assertExtent(extent, mln_vulkan_owned_texture_descriptor.extent(vulkanOwnedTexture));
      assertVulkanContext(
          vulkanContext, mln_vulkan_owned_texture_descriptor.context(vulkanOwnedTexture));

      var vulkanBorrowedTexture =
          RenderStructs.vulkanBorrowedTextureDescriptor(
              new VulkanBorrowedTextureDescriptor()
                  .extent(extent)
                  .context(vulkanContext)
                  .image(NativePointer.ofAddress(0x70))
                  .imageView(NativePointer.ofAddress(0x80))
                  .format(44)
                  .initialLayout(1)
                  .finalLayout(2),
              arena);
      assertEquals(
          (int) mln_vulkan_borrowed_texture_descriptor.sizeof(),
          mln_vulkan_borrowed_texture_descriptor.size(vulkanBorrowedTexture));
      assertExtent(extent, mln_vulkan_borrowed_texture_descriptor.extent(vulkanBorrowedTexture));
      assertVulkanContext(
          vulkanContext, mln_vulkan_borrowed_texture_descriptor.context(vulkanBorrowedTexture));
      assertPointer(0x70, mln_vulkan_borrowed_texture_descriptor.image(vulkanBorrowedTexture));
      assertPointer(0x80, mln_vulkan_borrowed_texture_descriptor.image_view(vulkanBorrowedTexture));
      assertEquals(44, mln_vulkan_borrowed_texture_descriptor.format(vulkanBorrowedTexture));
      assertEquals(1, mln_vulkan_borrowed_texture_descriptor.initial_layout(vulkanBorrowedTexture));
      assertEquals(2, mln_vulkan_borrowed_texture_descriptor.final_layout(vulkanBorrowedTexture));

      var vulkanSurface =
          RenderStructs.vulkanSurfaceDescriptor(
              new VulkanSurfaceDescriptor()
                  .extent(extent)
                  .context(vulkanContext)
                  .surface(NativePointer.ofAddress(0x90)),
              arena);
      assertEquals(
          (int) mln_vulkan_surface_descriptor.sizeof(),
          mln_vulkan_surface_descriptor.size(vulkanSurface));
      assertExtent(extent, mln_vulkan_surface_descriptor.extent(vulkanSurface));
      assertVulkanContext(vulkanContext, mln_vulkan_surface_descriptor.context(vulkanSurface));
      assertPointer(0x90, mln_vulkan_surface_descriptor.surface(vulkanSurface));
    }
  }

  private static void assertExtent(RenderTargetExtent expected, MemorySegment segment) {
    assertEquals((int) mln_render_target_extent.sizeof(), mln_render_target_extent.size(segment));
    assertEquals(expected.width(), mln_render_target_extent.width(segment));
    assertEquals(expected.height(), mln_render_target_extent.height(segment));
    assertEquals(expected.scaleFactor(), mln_render_target_extent.scale_factor(segment), 0.0);
  }

  private static void assertMetalContext(MetalContextDescriptor expected, MemorySegment segment) {
    assertEquals(
        (int) mln_metal_context_descriptor.sizeof(), mln_metal_context_descriptor.size(segment));
    assertPointer(expected.device().address(), mln_metal_context_descriptor.device(segment));
  }

  private static void assertVulkanContext(VulkanContextDescriptor expected, MemorySegment segment) {
    assertEquals(
        (int) mln_vulkan_context_descriptor.sizeof(), mln_vulkan_context_descriptor.size(segment));
    assertPointer(expected.instance().address(), mln_vulkan_context_descriptor.instance(segment));
    assertPointer(
        expected.physicalDevice().address(),
        mln_vulkan_context_descriptor.physical_device(segment));
    assertPointer(expected.device().address(), mln_vulkan_context_descriptor.device(segment));
    assertPointer(
        expected.graphicsQueue().address(), mln_vulkan_context_descriptor.graphics_queue(segment));
    assertEquals(
        expected.graphicsQueueFamilyIndex(),
        mln_vulkan_context_descriptor.graphics_queue_family_index(segment));
  }

  private static void assertPointer(long expectedAddress, MemorySegment segment) {
    assertEquals(expectedAddress, segment.address());
  }
}
