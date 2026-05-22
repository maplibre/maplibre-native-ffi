package org.maplibre.nativejni.internal.struct;

import java.util.Objects;
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

/** Internal materializers for render target descriptors and copied render values. */
public final class RenderStructs {
  private RenderStructs() {}

  public record ExtentValue(int width, int height, double scaleFactor) {}

  public record MetalContextValue(long device) {}

  public record VulkanContextValue(
      long instance,
      long physicalDevice,
      long device,
      long graphicsQueue,
      int graphicsQueueFamilyIndex) {}

  public record MetalOwnedTextureValue(ExtentValue extent, MetalContextValue context) {}

  public record MetalBorrowedTextureValue(ExtentValue extent, long texture) {}

  public record MetalSurfaceValue(ExtentValue extent, MetalContextValue context, long layer) {}

  public record VulkanOwnedTextureValue(ExtentValue extent, VulkanContextValue context) {}

  public record VulkanBorrowedTextureValue(
      ExtentValue extent,
      VulkanContextValue context,
      long image,
      long imageView,
      int format,
      int initialLayout,
      Integer finalLayout) {}

  public record VulkanSurfaceValue(ExtentValue extent, VulkanContextValue context, long surface) {}

  public static ExtentValue extent(RenderTargetExtent extent) {
    Objects.requireNonNull(extent, "extent");
    return new ExtentValue(extent.width(), extent.height(), extent.scaleFactor());
  }

  public static MetalContextValue metalContext(MetalContextDescriptor context) {
    Objects.requireNonNull(context, "context");
    return new MetalContextValue(address(context.device()));
  }

  public static VulkanContextValue vulkanContext(VulkanContextDescriptor context) {
    Objects.requireNonNull(context, "context");
    return new VulkanContextValue(
        address(context.instance()),
        address(context.physicalDevice()),
        address(context.device()),
        address(context.graphicsQueue()),
        context.graphicsQueueFamilyIndex());
  }

  public static MetalOwnedTextureValue metalOwnedTextureDescriptor(
      MetalOwnedTextureDescriptor descriptor) {
    Objects.requireNonNull(descriptor, "descriptor");
    return new MetalOwnedTextureValue(
        extent(descriptor.extent()), metalContext(descriptor.context()));
  }

  public static MetalBorrowedTextureValue metalBorrowedTextureDescriptor(
      MetalBorrowedTextureDescriptor descriptor) {
    Objects.requireNonNull(descriptor, "descriptor");
    return new MetalBorrowedTextureValue(
        extent(descriptor.extent()), address(descriptor.texture()));
  }

  public static MetalSurfaceValue metalSurfaceDescriptor(MetalSurfaceDescriptor descriptor) {
    Objects.requireNonNull(descriptor, "descriptor");
    return new MetalSurfaceValue(
        extent(descriptor.extent()),
        metalContext(descriptor.context()),
        address(descriptor.layer()));
  }

  public static VulkanOwnedTextureValue vulkanOwnedTextureDescriptor(
      VulkanOwnedTextureDescriptor descriptor) {
    Objects.requireNonNull(descriptor, "descriptor");
    return new VulkanOwnedTextureValue(
        extent(descriptor.extent()), vulkanContext(descriptor.context()));
  }

  public static VulkanBorrowedTextureValue vulkanBorrowedTextureDescriptor(
      VulkanBorrowedTextureDescriptor descriptor) {
    Objects.requireNonNull(descriptor, "descriptor");
    return new VulkanBorrowedTextureValue(
        extent(descriptor.extent()),
        vulkanContext(descriptor.context()),
        address(descriptor.image()),
        address(descriptor.imageView()),
        descriptor.format(),
        descriptor.initialLayout(),
        descriptor.finalLayout());
  }

  public static VulkanSurfaceValue vulkanSurfaceDescriptor(VulkanSurfaceDescriptor descriptor) {
    Objects.requireNonNull(descriptor, "descriptor");
    return new VulkanSurfaceValue(
        extent(descriptor.extent()),
        vulkanContext(descriptor.context()),
        address(descriptor.surface()));
  }

  private static long address(NativePointer pointer) {
    return Objects.requireNonNull(pointer, "pointer").address();
  }
}
