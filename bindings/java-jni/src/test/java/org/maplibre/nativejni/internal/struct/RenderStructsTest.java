package org.maplibre.nativejni.internal.struct;

import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.MetalContextDescriptor;
import org.maplibre.nativejni.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativejni.render.MetalSurfaceDescriptor;
import org.maplibre.nativejni.render.NativePointer;
import org.maplibre.nativejni.render.RenderTargetExtent;
import org.maplibre.nativejni.render.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.VulkanContextDescriptor;
import org.maplibre.nativejni.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativejni.render.VulkanSurfaceDescriptor;

final class RenderStructsTest {
  @Test
  void renderTargetDescriptorsMaterializeNestedFields() {
    var extent = new RenderTargetExtent(640, 480, 2.0);
    var metalContext = new MetalContextDescriptor(NativePointer.ofAddress(0x1234));
    var vulkanContext =
        new VulkanContextDescriptor(
                NativePointer.ofAddress(0x10),
                NativePointer.ofAddress(0x20),
                NativePointer.ofAddress(0x30),
                NativePointer.ofAddress(0x40),
                7)
            .procAddresses(NativePointer.ofAddress(0x41), NativePointer.ofAddress(0x42));

    var metalOwnedTexture =
        RenderStructs.metalOwnedTextureDescriptor(
            new MetalOwnedTextureDescriptor().extent(extent).context(metalContext));
    assertExtent(extent, metalOwnedTexture.extent());
    assertEquals(0x1234, metalOwnedTexture.context().device());

    var metalBorrowedTexture =
        RenderStructs.metalBorrowedTextureDescriptor(
            new MetalBorrowedTextureDescriptor()
                .extent(extent)
                .texture(NativePointer.ofAddress(0x50)));
    assertExtent(extent, metalBorrowedTexture.extent());
    assertEquals(0x50, metalBorrowedTexture.texture());

    var metalSurface =
        RenderStructs.metalSurfaceDescriptor(
            new MetalSurfaceDescriptor()
                .extent(extent)
                .context(metalContext)
                .layer(NativePointer.ofAddress(0x60)));
    assertExtent(extent, metalSurface.extent());
    assertEquals(0x1234, metalSurface.context().device());
    assertEquals(0x60, metalSurface.layer());

    var vulkanOwnedTexture =
        RenderStructs.vulkanOwnedTextureDescriptor(
            new VulkanOwnedTextureDescriptor().extent(extent).context(vulkanContext));
    assertExtent(extent, vulkanOwnedTexture.extent());
    assertVulkanContext(vulkanContext, vulkanOwnedTexture.context());

    var vulkanBorrowedTexture =
        RenderStructs.vulkanBorrowedTextureDescriptor(
            new VulkanBorrowedTextureDescriptor()
                .extent(extent)
                .context(vulkanContext)
                .image(NativePointer.ofAddress(0x70))
                .imageView(NativePointer.ofAddress(0x80))
                .format(44)
                .initialLayout(1)
                .finalLayout(2));
    assertExtent(extent, vulkanBorrowedTexture.extent());
    assertVulkanContext(vulkanContext, vulkanBorrowedTexture.context());
    assertEquals(0x70, vulkanBorrowedTexture.image());
    assertEquals(0x80, vulkanBorrowedTexture.imageView());
    assertEquals(44, vulkanBorrowedTexture.format());
    assertEquals(1, vulkanBorrowedTexture.initialLayout());
    assertEquals(2, vulkanBorrowedTexture.finalLayout());

    var vulkanSurface =
        RenderStructs.vulkanSurfaceDescriptor(
            new VulkanSurfaceDescriptor()
                .extent(extent)
                .context(vulkanContext)
                .surface(NativePointer.ofAddress(0x90)));
    assertExtent(extent, vulkanSurface.extent());
    assertVulkanContext(vulkanContext, vulkanSurface.context());
    assertEquals(0x90, vulkanSurface.surface());
  }

  private static void assertExtent(RenderTargetExtent expected, RenderStructs.ExtentValue value) {
    assertEquals(expected.width(), value.width());
    assertEquals(expected.height(), value.height());
    assertEquals(expected.scaleFactor(), value.scaleFactor(), 0.0);
  }

  private static void assertVulkanContext(
      VulkanContextDescriptor expected, RenderStructs.VulkanContextValue value) {
    assertEquals(expected.instance().address(), value.instance());
    assertEquals(expected.physicalDevice().address(), value.physicalDevice());
    assertEquals(expected.device().address(), value.device());
    assertEquals(expected.graphicsQueue().address(), value.graphicsQueue());
    assertEquals(expected.graphicsQueueFamilyIndex(), value.graphicsQueueFamilyIndex());
    assertEquals(expected.getInstanceProcAddr().address(), value.getInstanceProcAddr());
    assertEquals(expected.getDeviceProcAddr().address(), value.getDeviceProcAddr());
  }
}
