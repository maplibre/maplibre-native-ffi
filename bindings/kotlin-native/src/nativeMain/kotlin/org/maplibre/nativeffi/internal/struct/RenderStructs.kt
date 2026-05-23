package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.alloc
import kotlinx.cinterop.get
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toCPointer
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_metal_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_metal_surface_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_surface_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_render_target_extent
import org.maplibre.nativeffi.internal.c.mln_texture_image_info
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_vulkan_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_frame
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_descriptor_default
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.TextureImageInfo
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor

/** Internal materializers and readers for render target descriptors and frames. */
@OptIn(ExperimentalForeignApi::class)
internal object RenderStructs {
  fun metalOwnedTextureDescriptor(
    descriptor: MetalOwnedTextureDescriptor,
    scope: MemScope,
  ): CPointer<mln_metal_owned_texture_descriptor> {
    val native = scope.alloc<mln_metal_owned_texture_descriptor>()
    mln_metal_owned_texture_descriptor_default().place(native.ptr)
    fillExtent(native.extent, descriptor.extent)
    fillMetalContext(native.context, descriptor.context)
    return native.ptr
  }

  fun metalBorrowedTextureDescriptor(
    descriptor: MetalBorrowedTextureDescriptor,
    scope: MemScope,
  ): CPointer<mln_metal_borrowed_texture_descriptor> {
    val native = scope.alloc<mln_metal_borrowed_texture_descriptor>()
    mln_metal_borrowed_texture_descriptor_default().place(native.ptr)
    fillExtent(native.extent, descriptor.extent)
    native.texture = pointer(descriptor.texture)
    return native.ptr
  }

  fun vulkanOwnedTextureDescriptor(
    descriptor: VulkanOwnedTextureDescriptor,
    scope: MemScope,
  ): CPointer<mln_vulkan_owned_texture_descriptor> {
    val native = scope.alloc<mln_vulkan_owned_texture_descriptor>()
    mln_vulkan_owned_texture_descriptor_default().place(native.ptr)
    fillExtent(native.extent, descriptor.extent)
    fillVulkanContext(native.context, descriptor.context)
    return native.ptr
  }

  fun vulkanBorrowedTextureDescriptor(
    descriptor: VulkanBorrowedTextureDescriptor,
    scope: MemScope,
  ): CPointer<mln_vulkan_borrowed_texture_descriptor> {
    val native = scope.alloc<mln_vulkan_borrowed_texture_descriptor>()
    mln_vulkan_borrowed_texture_descriptor_default().place(native.ptr)
    fillExtent(native.extent, descriptor.extent)
    fillVulkanContext(native.context, descriptor.context)
    native.image = pointer(descriptor.image)
    native.image_view = pointer(descriptor.imageView)
    native.format = descriptor.format
    native.initial_layout = descriptor.initialLayout
    descriptor.finalLayout?.let { native.final_layout = it }
    return native.ptr
  }

  fun metalSurfaceDescriptor(
    descriptor: MetalSurfaceDescriptor,
    scope: MemScope,
  ): CPointer<mln_metal_surface_descriptor> {
    val native = scope.alloc<mln_metal_surface_descriptor>()
    mln_metal_surface_descriptor_default().place(native.ptr)
    fillExtent(native.extent, descriptor.extent)
    fillMetalContext(native.context, descriptor.context)
    native.layer = pointer(descriptor.layer)
    return native.ptr
  }

  fun vulkanSurfaceDescriptor(
    descriptor: VulkanSurfaceDescriptor,
    scope: MemScope,
  ): CPointer<mln_vulkan_surface_descriptor> {
    val native = scope.alloc<mln_vulkan_surface_descriptor>()
    mln_vulkan_surface_descriptor_default().place(native.ptr)
    fillExtent(native.extent, descriptor.extent)
    fillVulkanContext(native.context, descriptor.context)
    native.surface = pointer(descriptor.surface)
    return native.ptr
  }

  fun textureImageInfo(value: mln_texture_image_info): TextureImageInfo =
    TextureImageInfo(value.width, value.height, value.stride, value.byte_length)

  fun metalOwnedTextureFrame(scope: MemScope): CPointer<mln_metal_owned_texture_frame> {
    val native = scope.alloc<mln_metal_owned_texture_frame>()
    native.size = sizeOf<mln_metal_owned_texture_frame>().toUInt()
    return native.ptr
  }

  fun vulkanOwnedTextureFrame(scope: MemScope): CPointer<mln_vulkan_owned_texture_frame> {
    val native = scope.alloc<mln_vulkan_owned_texture_frame>()
    native.size = sizeOf<mln_vulkan_owned_texture_frame>().toUInt()
    return native.ptr
  }

  private fun fillExtent(native: mln_render_target_extent, extent: RenderTargetExtent) {
    native.size = sizeOf<mln_render_target_extent>().toUInt()
    native.width = extent.width
    native.height = extent.height
    native.scale_factor = extent.scaleFactor
  }

  private fun fillMetalContext(
    native: mln_metal_context_descriptor,
    context: MetalContextDescriptor,
  ) {
    native.size = sizeOf<mln_metal_context_descriptor>().toUInt()
    native.device = pointer(context.device)
  }

  private fun fillVulkanContext(
    native: mln_vulkan_context_descriptor,
    context: VulkanContextDescriptor,
  ) {
    native.size = sizeOf<mln_vulkan_context_descriptor>().toUInt()
    native.instance = pointer(context.instance)
    native.physical_device = pointer(context.physicalDevice)
    native.device = pointer(context.device)
    native.graphics_queue = pointer(context.graphicsQueue)
    native.graphics_queue_family_index = context.graphicsQueueFamilyIndex
  }

  private fun pointer(pointer: NativePointer): kotlinx.cinterop.COpaquePointer? =
    if (pointer.isNull) null else pointer.address.toLong().toCPointer()
}
