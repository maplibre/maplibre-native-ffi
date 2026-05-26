package org.maplibre.nativejni.internal.struct;

import java.util.Objects;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.MetalContextDescriptor;
import org.maplibre.nativejni.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativejni.render.MetalSurfaceDescriptor;
import org.maplibre.nativejni.render.NativePointer;
import org.maplibre.nativejni.render.RenderTargetExtent;
import org.maplibre.nativejni.render.TextureImageInfo;
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
      int graphicsQueueFamilyIndex,
      long getInstanceProcAddr,
      long getDeviceProcAddr) {}

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
        context.graphicsQueueFamilyIndex(),
        address(context.getInstanceProcAddr()),
        address(context.getDeviceProcAddr()));
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

  public static MaplibreNativeC.mln_metal_owned_texture_descriptor
      nativeMetalOwnedTextureDescriptor(MetalOwnedTextureDescriptor descriptor) {
    var value = metalOwnedTextureDescriptor(descriptor);
    var out = MaplibreNativeC.mln_metal_owned_texture_descriptor_default();
    setExtent(out.extent(), value.extent());
    out.context().device(JavaCppSupport.pointerOrNull(value.context().device()));
    return out;
  }

  public static MaplibreNativeC.mln_metal_borrowed_texture_descriptor
      nativeMetalBorrowedTextureDescriptor(MetalBorrowedTextureDescriptor descriptor) {
    var value = metalBorrowedTextureDescriptor(descriptor);
    var out = MaplibreNativeC.mln_metal_borrowed_texture_descriptor_default();
    setExtent(out.extent(), value.extent());
    out.texture(JavaCppSupport.pointerOrNull(value.texture()));
    return out;
  }

  public static MaplibreNativeC.mln_metal_surface_descriptor nativeMetalSurfaceDescriptor(
      MetalSurfaceDescriptor descriptor) {
    var value = metalSurfaceDescriptor(descriptor);
    var out = MaplibreNativeC.mln_metal_surface_descriptor_default();
    setExtent(out.extent(), value.extent());
    out.context().device(JavaCppSupport.pointerOrNull(value.context().device()));
    out.layer(JavaCppSupport.pointerOrNull(value.layer()));
    return out;
  }

  public static MaplibreNativeC.mln_vulkan_owned_texture_descriptor
      nativeVulkanOwnedTextureDescriptor(VulkanOwnedTextureDescriptor descriptor) {
    var value = vulkanOwnedTextureDescriptor(descriptor);
    var out = MaplibreNativeC.mln_vulkan_owned_texture_descriptor_default();
    setExtent(out.extent(), value.extent());
    setVulkanContext(out.context(), value.context());
    return out;
  }

  public static MaplibreNativeC.mln_vulkan_borrowed_texture_descriptor
      nativeVulkanBorrowedTextureDescriptor(VulkanBorrowedTextureDescriptor descriptor) {
    var value = vulkanBorrowedTextureDescriptor(descriptor);
    var out = MaplibreNativeC.mln_vulkan_borrowed_texture_descriptor_default();
    setExtent(out.extent(), value.extent());
    setVulkanContext(out.context(), value.context());
    out.image(JavaCppSupport.pointerOrNull(value.image()));
    out.image_view(JavaCppSupport.pointerOrNull(value.imageView()));
    out.format(value.format());
    out.initial_layout(value.initialLayout());
    if (value.finalLayout() != null) {
      out.final_layout(value.finalLayout());
    }
    return out;
  }

  public static MaplibreNativeC.mln_vulkan_surface_descriptor nativeVulkanSurfaceDescriptor(
      VulkanSurfaceDescriptor descriptor) {
    var value = vulkanSurfaceDescriptor(descriptor);
    var out = MaplibreNativeC.mln_vulkan_surface_descriptor_default();
    setExtent(out.extent(), value.extent());
    setVulkanContext(out.context(), value.context());
    out.surface(JavaCppSupport.pointerOrNull(value.surface()));
    return out;
  }

  public static TextureImageInfo textureImageInfo(MaplibreNativeC.mln_texture_image_info info) {
    return new TextureImageInfo(info.width(), info.height(), info.stride(), info.byte_length());
  }

  private static void setExtent(MaplibreNativeC.mln_render_target_extent out, ExtentValue extent) {
    out.width(extent.width());
    out.height(extent.height());
    out.scale_factor(extent.scaleFactor());
  }

  private static void setVulkanContext(
      MaplibreNativeC.mln_vulkan_context_descriptor out, VulkanContextValue context) {
    out.instance(JavaCppSupport.pointerOrNull(context.instance()));
    out.physical_device(JavaCppSupport.pointerOrNull(context.physicalDevice()));
    out.device(JavaCppSupport.pointerOrNull(context.device()));
    out.graphics_queue(JavaCppSupport.pointerOrNull(context.graphicsQueue()));
    out.graphics_queue_family_index(context.graphicsQueueFamilyIndex());
    out.get_instance_proc_addr(JavaCppSupport.pointerOrNull(context.getInstanceProcAddr()));
    out.get_device_proc_addr(JavaCppSupport.pointerOrNull(context.getDeviceProcAddr()));
  }

  private static long address(NativePointer pointer) {
    return Objects.requireNonNull(pointer, "pointer").address();
  }
}
