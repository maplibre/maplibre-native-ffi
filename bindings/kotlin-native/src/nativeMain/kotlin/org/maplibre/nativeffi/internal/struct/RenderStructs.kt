package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.alloc
import kotlinx.cinterop.get
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toCPointer
import org.maplibre.nativeffi.internal.c.MLN_OPENGL_CONTEXT_PLATFORM_EGL
import org.maplibre.nativeffi.internal.c.MLN_OPENGL_CONTEXT_PLATFORM_WGL
import org.maplibre.nativeffi.internal.c.mln_egl_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_metal_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_metal_surface_descriptor
import org.maplibre.nativeffi.internal.c.mln_metal_surface_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_opengl_borrowed_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_opengl_borrowed_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_opengl_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_opengl_surface_descriptor
import org.maplibre.nativeffi.internal.c.mln_opengl_surface_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_render_target_extent
import org.maplibre.nativeffi.internal.c.mln_texture_image_info
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_vulkan_context_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_descriptor
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_descriptor_default
import org.maplibre.nativeffi.internal.c.mln_wgl_context_descriptor
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLContextDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.TextureImageInfo
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.render.WglContextDescriptor

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
    native.format = descriptor.format.toUInt()
    native.initial_layout = descriptor.initialLayout.toUInt()
    descriptor.finalLayout?.let { native.final_layout = it.toUInt() }
    return native.ptr
  }

  fun openglOwnedTextureDescriptor(
    descriptor: OpenGLOwnedTextureDescriptor,
    scope: MemScope,
  ): CPointer<mln_opengl_owned_texture_descriptor> {
    val native = scope.alloc<mln_opengl_owned_texture_descriptor>()
    mln_opengl_owned_texture_descriptor_default().place(native.ptr)
    fillExtent(native.extent, descriptor.extent)
    fillOpenGLContext(native.context, descriptor.context)
    return native.ptr
  }

  fun openglBorrowedTextureDescriptor(
    descriptor: OpenGLBorrowedTextureDescriptor,
    scope: MemScope,
  ): CPointer<mln_opengl_borrowed_texture_descriptor> {
    val native = scope.alloc<mln_opengl_borrowed_texture_descriptor>()
    mln_opengl_borrowed_texture_descriptor_default().place(native.ptr)
    fillExtent(native.extent, descriptor.extent)
    fillOpenGLContext(native.context, descriptor.context)
    native.texture = descriptor.texture.toUInt()
    native.target = descriptor.target.toUInt()
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

  fun openglSurfaceDescriptor(
    descriptor: OpenGLSurfaceDescriptor,
    scope: MemScope,
  ): CPointer<mln_opengl_surface_descriptor> {
    val native = scope.alloc<mln_opengl_surface_descriptor>()
    mln_opengl_surface_descriptor_default().place(native.ptr)
    fillExtent(native.extent, descriptor.extent)
    fillOpenGLContext(native.context, descriptor.context)
    native.surface = pointer(descriptor.surface)
    return native.ptr
  }

  fun textureImageInfo(value: mln_texture_image_info): TextureImageInfo =
    TextureImageInfo(
      checkedInt(value.width, "texture width"),
      checkedInt(value.height, "texture height"),
      checkedInt(value.stride, "texture stride"),
      checkedLong(value.byte_length, "texture byte length"),
    )

  private fun checkedInt(value: UInt, name: String): Int {
    require(value <= Int.MAX_VALUE.toUInt()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  private fun checkedLong(value: ULong, name: String): Long {
    require(value <= Long.MAX_VALUE.toULong()) { "$name exceeds Long.MAX_VALUE" }
    return value.toLong()
  }

  private fun fillExtent(native: mln_render_target_extent, extent: RenderTargetExtent) {
    native.size = sizeOf<mln_render_target_extent>().toUInt()
    native.width = extent.width.toUInt()
    native.height = extent.height.toUInt()
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
    native.graphics_queue_family_index = context.graphicsQueueFamilyIndex.toUInt()
    native.get_instance_proc_addr = pointer(context.getInstanceProcAddr)
    native.get_device_proc_addr = pointer(context.getDeviceProcAddr)
  }

  private fun fillOpenGLContext(
    native: mln_opengl_context_descriptor,
    context: OpenGLContextDescriptor,
  ) {
    native.size = sizeOf<mln_opengl_context_descriptor>().toUInt()
    when (context) {
      is WglContextDescriptor -> {
        native.platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL
        fillWglContext(native.data.wgl, context)
      }

      is EglContextDescriptor -> {
        native.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL
        fillEglContext(native.data.egl, context)
      }
    }
  }

  private fun fillWglContext(native: mln_wgl_context_descriptor, context: WglContextDescriptor) {
    native.size = sizeOf<mln_wgl_context_descriptor>().toUInt()
    native.device_context = pointer(context.deviceContext)
    native.share_context = pointer(context.shareContext)
    native.get_proc_address = pointer(context.getProcAddress)
  }

  private fun fillEglContext(native: mln_egl_context_descriptor, context: EglContextDescriptor) {
    native.size = sizeOf<mln_egl_context_descriptor>().toUInt()
    native.display = pointer(context.display)
    native.config = pointer(context.config)
    native.share_context = pointer(context.shareContext)
    native.get_proc_address = pointer(context.getProcAddress)
  }

  private fun pointer(pointer: NativePointer): kotlinx.cinterop.COpaquePointer? =
    if (pointer.isNull) null else pointer.address.toCPointer()
}
