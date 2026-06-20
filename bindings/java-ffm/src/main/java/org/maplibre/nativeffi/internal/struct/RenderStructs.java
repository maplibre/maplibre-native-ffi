package org.maplibre.nativeffi.internal.struct;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_egl_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_frame;
import org.maplibre.nativeffi.internal.c.mln_metal_surface_descriptor;
import org.maplibre.nativeffi.internal.c.mln_opengl_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_opengl_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_frame;
import org.maplibre.nativeffi.internal.c.mln_opengl_surface_descriptor;
import org.maplibre.nativeffi.internal.c.mln_render_target_extent;
import org.maplibre.nativeffi.internal.c.mln_texture_image_info;
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame;
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_descriptor;
import org.maplibre.nativeffi.internal.c.mln_wgl_context_descriptor;
import org.maplibre.nativeffi.render.EglContextDescriptor;
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalContextDescriptor;
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor;
import org.maplibre.nativeffi.render.NativePointer;
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLContextDescriptor;
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor;
import org.maplibre.nativeffi.render.RenderTargetExtent;
import org.maplibre.nativeffi.render.TextureImageInfo;
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanContextDescriptor;
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor;
import org.maplibre.nativeffi.render.WglContextDescriptor;

/** Internal materializers and readers for render target descriptors and frames. */
public final class RenderStructs {
  private RenderStructs() {}

  public static MemorySegment metalOwnedTextureDescriptor(
      MetalOwnedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_metal_owned_texture_descriptor_default(arena);
    fillExtent(mln_metal_owned_texture_descriptor.extent(segment), descriptor.extent());
    fillMetalContext(mln_metal_owned_texture_descriptor.context(segment), descriptor.context());
    return segment;
  }

  public static MemorySegment metalBorrowedTextureDescriptor(
      MetalBorrowedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_metal_borrowed_texture_descriptor_default(arena);
    fillExtent(mln_metal_borrowed_texture_descriptor.extent(segment), descriptor.extent());
    mln_metal_borrowed_texture_descriptor.texture(segment, pointer(descriptor.texture()));
    return segment;
  }

  public static MemorySegment vulkanOwnedTextureDescriptor(
      VulkanOwnedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_vulkan_owned_texture_descriptor_default(arena);
    fillExtent(mln_vulkan_owned_texture_descriptor.extent(segment), descriptor.extent());
    fillVulkanContext(mln_vulkan_owned_texture_descriptor.context(segment), descriptor.context());
    return segment;
  }

  public static MemorySegment vulkanBorrowedTextureDescriptor(
      VulkanBorrowedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_vulkan_borrowed_texture_descriptor_default(arena);
    fillExtent(mln_vulkan_borrowed_texture_descriptor.extent(segment), descriptor.extent());
    fillVulkanContext(
        mln_vulkan_borrowed_texture_descriptor.context(segment), descriptor.context());
    mln_vulkan_borrowed_texture_descriptor.image(segment, pointer(descriptor.image()));
    mln_vulkan_borrowed_texture_descriptor.image_view(segment, pointer(descriptor.imageView()));
    mln_vulkan_borrowed_texture_descriptor.format(segment, descriptor.format());
    mln_vulkan_borrowed_texture_descriptor.initial_layout(segment, descriptor.initialLayout());
    if (descriptor.hasFinalLayout()) {
      mln_vulkan_borrowed_texture_descriptor.final_layout(segment, descriptor.finalLayout());
    }
    return segment;
  }

  public static MemorySegment openglOwnedTextureDescriptor(
      OpenGLOwnedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_opengl_owned_texture_descriptor_default(arena);
    fillExtent(mln_opengl_owned_texture_descriptor.extent(segment), descriptor.extent());
    fillOpenGLContext(mln_opengl_owned_texture_descriptor.context(segment), descriptor.context());
    return segment;
  }

  public static MemorySegment openglBorrowedTextureDescriptor(
      OpenGLBorrowedTextureDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_opengl_borrowed_texture_descriptor_default(arena);
    fillExtent(mln_opengl_borrowed_texture_descriptor.extent(segment), descriptor.extent());
    fillOpenGLContext(
        mln_opengl_borrowed_texture_descriptor.context(segment), descriptor.context());
    mln_opengl_borrowed_texture_descriptor.texture(segment, descriptor.texture());
    mln_opengl_borrowed_texture_descriptor.target(segment, descriptor.target());
    return segment;
  }

  public static MemorySegment metalSurfaceDescriptor(
      MetalSurfaceDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_metal_surface_descriptor_default(arena);
    fillExtent(mln_metal_surface_descriptor.extent(segment), descriptor.extent());
    fillMetalContext(mln_metal_surface_descriptor.context(segment), descriptor.context());
    mln_metal_surface_descriptor.layer(segment, pointer(descriptor.layer()));
    return segment;
  }

  public static MemorySegment vulkanSurfaceDescriptor(
      VulkanSurfaceDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_vulkan_surface_descriptor_default(arena);
    fillExtent(mln_vulkan_surface_descriptor.extent(segment), descriptor.extent());
    fillVulkanContext(mln_vulkan_surface_descriptor.context(segment), descriptor.context());
    mln_vulkan_surface_descriptor.surface(segment, pointer(descriptor.surface()));
    return segment;
  }

  public static MemorySegment openglSurfaceDescriptor(
      OpenGLSurfaceDescriptor descriptor, Arena arena) {
    var segment = MapLibreNativeC.mln_opengl_surface_descriptor_default(arena);
    fillExtent(mln_opengl_surface_descriptor.extent(segment), descriptor.extent());
    fillOpenGLContext(mln_opengl_surface_descriptor.context(segment), descriptor.context());
    mln_opengl_surface_descriptor.surface(segment, pointer(descriptor.surface()));
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

  public static MemorySegment openglOwnedTextureFrame(Arena arena) {
    var segment = mln_opengl_owned_texture_frame.allocate(arena);
    mln_opengl_owned_texture_frame.size(segment, (int) mln_opengl_owned_texture_frame.sizeof());
    return segment;
  }

  private static void fillExtent(MemorySegment segment, RenderTargetExtent extent) {
    if (extent.width() < 0 || extent.height() < 0) {
      throw invalidArgument("render target width and height must be non-negative");
    }
    if (!Double.isFinite(extent.scaleFactor()) || extent.scaleFactor() <= 0.0) {
      throw invalidArgument("render target scale factor must be positive and finite");
    }
    mln_render_target_extent.size(segment, (int) mln_render_target_extent.sizeof());
    mln_render_target_extent.width(segment, extent.width());
    mln_render_target_extent.height(segment, extent.height());
    mln_render_target_extent.scale_factor(segment, extent.scaleFactor());
  }

  private static InvalidArgumentException invalidArgument(String diagnostic) {
    return new InvalidArgumentException(MapLibreNativeC.MLN_STATUS_INVALID_ARGUMENT(), diagnostic);
  }

  private static void fillMetalContext(MemorySegment segment, MetalContextDescriptor context) {
    mln_metal_context_descriptor.size(segment, (int) mln_metal_context_descriptor.sizeof());
    mln_metal_context_descriptor.device(segment, pointer(context.device()));
  }

  private static void fillVulkanContext(MemorySegment segment, VulkanContextDescriptor context) {
    mln_vulkan_context_descriptor.size(segment, (int) mln_vulkan_context_descriptor.sizeof());
    mln_vulkan_context_descriptor.instance(segment, pointer(context.instance()));
    mln_vulkan_context_descriptor.physical_device(segment, pointer(context.physicalDevice()));
    mln_vulkan_context_descriptor.device(segment, pointer(context.device()));
    mln_vulkan_context_descriptor.graphics_queue(segment, pointer(context.graphicsQueue()));
    mln_vulkan_context_descriptor.graphics_queue_family_index(
        segment, context.graphicsQueueFamilyIndex());
    mln_vulkan_context_descriptor.get_instance_proc_addr(
        segment, pointer(context.getInstanceProcAddr()));
    mln_vulkan_context_descriptor.get_device_proc_addr(
        segment, pointer(context.getDeviceProcAddr()));
  }

  private static void fillOpenGLContext(MemorySegment segment, OpenGLContextDescriptor context) {
    mln_opengl_context_descriptor.size(segment, (int) mln_opengl_context_descriptor.sizeof());
    var data = mln_opengl_context_descriptor.data(segment);
    switch (context) {
      case WglContextDescriptor wgl -> {
        mln_opengl_context_descriptor.platform(
            segment, MapLibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_WGL());
        fillWglContext(mln_opengl_context_descriptor.data.wgl(data), wgl);
      }
      case EglContextDescriptor egl -> {
        mln_opengl_context_descriptor.platform(
            segment, MapLibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_EGL());
        fillEglContext(mln_opengl_context_descriptor.data.egl(data), egl);
      }
    }
  }

  private static void fillWglContext(MemorySegment segment, WglContextDescriptor context) {
    mln_wgl_context_descriptor.size(segment, (int) mln_wgl_context_descriptor.sizeof());
    mln_wgl_context_descriptor.device_context(segment, pointer(context.deviceContext()));
    mln_wgl_context_descriptor.share_context(segment, pointer(context.shareContext()));
    mln_wgl_context_descriptor.get_proc_address(segment, pointer(context.getProcAddress()));
  }

  private static void fillEglContext(MemorySegment segment, EglContextDescriptor context) {
    mln_egl_context_descriptor.size(segment, (int) mln_egl_context_descriptor.sizeof());
    mln_egl_context_descriptor.display(segment, pointer(context.display()));
    mln_egl_context_descriptor.config(segment, pointer(context.config()));
    mln_egl_context_descriptor.share_context(segment, pointer(context.shareContext()));
    mln_egl_context_descriptor.get_proc_address(segment, pointer(context.getProcAddress()));
  }

  private static MemorySegment pointer(NativePointer pointer) {
    return pointer.isNull() ? MemorySegment.NULL : MemorySegment.ofAddress(pointer.address());
  }
}
