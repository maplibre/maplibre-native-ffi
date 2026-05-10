package org.maplibre.nativeffi.internal;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.maplibre.nativeffi.MetalBorrowedTextureDescriptor;
import org.maplibre.nativeffi.MetalOwnedTextureDescriptor;
import org.maplibre.nativeffi.MetalSurfaceDescriptor;
import org.maplibre.nativeffi.NativePointer;
import org.maplibre.nativeffi.OwnedTextureDescriptor;
import org.maplibre.nativeffi.TextureImageInfo;
import org.maplibre.nativeffi.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativeffi.VulkanOwnedTextureDescriptor;
import org.maplibre.nativeffi.VulkanSurfaceDescriptor;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_frame;
import org.maplibre.nativeffi.internal.c.mln_metal_surface_descriptor;
import org.maplibre.nativeffi.internal.c.mln_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_texture_image_info;
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame;
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_descriptor;

/** Internal materializers and readers for render target descriptors and frames. */
public final class RenderStructs {
  private RenderStructs() {}

  public static MemorySegment ownedTextureDescriptor(
      OwnedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_owned_texture_descriptor_default(arena);
    mln_owned_texture_descriptor.width(segment, descriptor.width());
    mln_owned_texture_descriptor.height(segment, descriptor.height());
    mln_owned_texture_descriptor.scale_factor(segment, descriptor.scaleFactor());
    return segment;
  }

  public static MemorySegment metalOwnedTextureDescriptor(
      MetalOwnedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_metal_owned_texture_descriptor_default(arena);
    mln_metal_owned_texture_descriptor.width(segment, descriptor.width());
    mln_metal_owned_texture_descriptor.height(segment, descriptor.height());
    mln_metal_owned_texture_descriptor.scale_factor(segment, descriptor.scaleFactor());
    mln_metal_owned_texture_descriptor.device(segment, pointer(descriptor.device()));
    return segment;
  }

  public static MemorySegment metalBorrowedTextureDescriptor(
      MetalBorrowedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_metal_borrowed_texture_descriptor_default(arena);
    mln_metal_borrowed_texture_descriptor.width(segment, descriptor.width());
    mln_metal_borrowed_texture_descriptor.height(segment, descriptor.height());
    mln_metal_borrowed_texture_descriptor.scale_factor(segment, descriptor.scaleFactor());
    mln_metal_borrowed_texture_descriptor.texture(segment, pointer(descriptor.texture()));
    return segment;
  }

  public static MemorySegment vulkanOwnedTextureDescriptor(
      VulkanOwnedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_vulkan_owned_texture_descriptor_default(arena);
    mln_vulkan_owned_texture_descriptor.width(segment, descriptor.width());
    mln_vulkan_owned_texture_descriptor.height(segment, descriptor.height());
    mln_vulkan_owned_texture_descriptor.scale_factor(segment, descriptor.scaleFactor());
    mln_vulkan_owned_texture_descriptor.instance(segment, pointer(descriptor.instance()));
    mln_vulkan_owned_texture_descriptor.physical_device(
        segment, pointer(descriptor.physicalDevice()));
    mln_vulkan_owned_texture_descriptor.device(segment, pointer(descriptor.device()));
    mln_vulkan_owned_texture_descriptor.graphics_queue(
        segment, pointer(descriptor.graphicsQueue()));
    mln_vulkan_owned_texture_descriptor.graphics_queue_family_index(
        segment, descriptor.graphicsQueueFamilyIndex());
    return segment;
  }

  public static MemorySegment vulkanBorrowedTextureDescriptor(
      VulkanBorrowedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_vulkan_borrowed_texture_descriptor_default(arena);
    mln_vulkan_borrowed_texture_descriptor.width(segment, descriptor.width());
    mln_vulkan_borrowed_texture_descriptor.height(segment, descriptor.height());
    mln_vulkan_borrowed_texture_descriptor.scale_factor(segment, descriptor.scaleFactor());
    mln_vulkan_borrowed_texture_descriptor.instance(segment, pointer(descriptor.instance()));
    mln_vulkan_borrowed_texture_descriptor.physical_device(
        segment, pointer(descriptor.physicalDevice()));
    mln_vulkan_borrowed_texture_descriptor.device(segment, pointer(descriptor.device()));
    mln_vulkan_borrowed_texture_descriptor.graphics_queue(
        segment, pointer(descriptor.graphicsQueue()));
    mln_vulkan_borrowed_texture_descriptor.graphics_queue_family_index(
        segment, descriptor.graphicsQueueFamilyIndex());
    mln_vulkan_borrowed_texture_descriptor.image(segment, pointer(descriptor.image()));
    mln_vulkan_borrowed_texture_descriptor.image_view(segment, pointer(descriptor.imageView()));
    mln_vulkan_borrowed_texture_descriptor.format(segment, descriptor.format());
    mln_vulkan_borrowed_texture_descriptor.initial_layout(segment, descriptor.initialLayout());
    if (descriptor.hasFinalLayout()) {
      mln_vulkan_borrowed_texture_descriptor.final_layout(segment, descriptor.finalLayout());
    }
    return segment;
  }

  public static MemorySegment metalSurfaceDescriptor(
      MetalSurfaceDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_metal_surface_descriptor_default(arena);
    mln_metal_surface_descriptor.width(segment, descriptor.width());
    mln_metal_surface_descriptor.height(segment, descriptor.height());
    mln_metal_surface_descriptor.scale_factor(segment, descriptor.scaleFactor());
    mln_metal_surface_descriptor.layer(segment, pointer(descriptor.layer()));
    mln_metal_surface_descriptor.device(segment, pointer(descriptor.device()));
    return segment;
  }

  public static MemorySegment vulkanSurfaceDescriptor(
      VulkanSurfaceDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_vulkan_surface_descriptor_default(arena);
    mln_vulkan_surface_descriptor.width(segment, descriptor.width());
    mln_vulkan_surface_descriptor.height(segment, descriptor.height());
    mln_vulkan_surface_descriptor.scale_factor(segment, descriptor.scaleFactor());
    mln_vulkan_surface_descriptor.instance(segment, pointer(descriptor.instance()));
    mln_vulkan_surface_descriptor.physical_device(segment, pointer(descriptor.physicalDevice()));
    mln_vulkan_surface_descriptor.device(segment, pointer(descriptor.device()));
    mln_vulkan_surface_descriptor.graphics_queue(segment, pointer(descriptor.graphicsQueue()));
    mln_vulkan_surface_descriptor.graphics_queue_family_index(
        segment, descriptor.graphicsQueueFamilyIndex());
    mln_vulkan_surface_descriptor.surface(segment, pointer(descriptor.surface()));
    return segment;
  }

  public static TextureImageInfo textureImageInfo(MemorySegment segment) {
    return new TextureImageInfo(
        mln_texture_image_info.width(segment),
        mln_texture_image_info.height(segment),
        mln_texture_image_info.stride(segment),
        mln_texture_image_info.byte_length(segment));
  }

  public static MemorySegment metalOwnedTextureFrame(Arena arena) {
    var segment = mln_metal_owned_texture_frame.allocate(arena);
    mln_metal_owned_texture_frame.size(segment, (int) mln_metal_owned_texture_frame.sizeof());
    return segment;
  }

  public static MemorySegment vulkanOwnedTextureFrame(Arena arena) {
    var segment = mln_vulkan_owned_texture_frame.allocate(arena);
    mln_vulkan_owned_texture_frame.size(segment, (int) mln_vulkan_owned_texture_frame.sizeof());
    return segment;
  }

  private static MemorySegment pointer(NativePointer pointer) {
    return pointer.isNull() ? MemorySegment.NULL : MemorySegment.ofAddress(pointer.address());
  }
}
