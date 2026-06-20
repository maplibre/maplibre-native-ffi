package org.maplibre.nativeffi.internal.struct;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_egl_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_metal_surface_descriptor;
import org.maplibre.nativeffi.internal.c.mln_opengl_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_opengl_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_opengl_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_opengl_surface_descriptor;
import org.maplibre.nativeffi.internal.c.mln_render_target_extent;
import org.maplibre.nativeffi.internal.c.mln_vulkan_borrowed_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_context_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_owned_texture_descriptor;
import org.maplibre.nativeffi.internal.c.mln_vulkan_surface_descriptor;
import org.maplibre.nativeffi.internal.c.mln_wgl_context_descriptor;
import org.maplibre.nativeffi.render.EglContextDescriptor;
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalContextDescriptor;
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor;
import org.maplibre.nativeffi.render.NativePointer;
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor;
import org.maplibre.nativeffi.render.RenderTargetExtent;
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanContextDescriptor;
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor;
import org.maplibre.nativeffi.render.WglContextDescriptor;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class RenderStructsTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

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
                7)
            .procAddresses(NativePointer.ofAddress(0x50), NativePointer.ofAddress(0x60));
    var wglContext =
        new WglContextDescriptor(NativePointer.ofAddress(0xa0), NativePointer.ofAddress(0xb0))
            .getProcAddress(NativePointer.ofAddress(0xc0));
    var eglContext =
        new EglContextDescriptor(
                NativePointer.ofAddress(0xd0),
                NativePointer.ofAddress(0xe0),
                NativePointer.ofAddress(0xf0))
            .getProcAddress(NativePointer.ofAddress(0x100));

    try (var arena = Arena.ofConfined()) {
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

      var openglOwnedTexture =
          RenderStructs.openglOwnedTextureDescriptor(
              new OpenGLOwnedTextureDescriptor().extent(extent).context(wglContext), arena);
      assertEquals(
          (int) mln_opengl_owned_texture_descriptor.sizeof(),
          mln_opengl_owned_texture_descriptor.size(openglOwnedTexture));
      assertExtent(extent, mln_opengl_owned_texture_descriptor.extent(openglOwnedTexture));
      assertWglContext(wglContext, mln_opengl_owned_texture_descriptor.context(openglOwnedTexture));

      var openglBorrowedTexture =
          RenderStructs.openglBorrowedTextureDescriptor(
              new OpenGLBorrowedTextureDescriptor()
                  .extent(extent)
                  .context(eglContext)
                  .texture(12)
                  .target(0x0de1),
              arena);
      assertEquals(
          (int) mln_opengl_borrowed_texture_descriptor.sizeof(),
          mln_opengl_borrowed_texture_descriptor.size(openglBorrowedTexture));
      assertExtent(extent, mln_opengl_borrowed_texture_descriptor.extent(openglBorrowedTexture));
      assertEglContext(
          eglContext, mln_opengl_borrowed_texture_descriptor.context(openglBorrowedTexture));
      assertEquals(12, mln_opengl_borrowed_texture_descriptor.texture(openglBorrowedTexture));
      assertEquals(0x0de1, mln_opengl_borrowed_texture_descriptor.target(openglBorrowedTexture));

      var openglSurface =
          RenderStructs.openglSurfaceDescriptor(
              new OpenGLSurfaceDescriptor()
                  .extent(extent)
                  .context(wglContext)
                  .surface(NativePointer.ofAddress(0x110)),
              arena);
      assertEquals(
          (int) mln_opengl_surface_descriptor.sizeof(),
          mln_opengl_surface_descriptor.size(openglSurface));
      assertExtent(extent, mln_opengl_surface_descriptor.extent(openglSurface));
      assertWglContext(wglContext, mln_opengl_surface_descriptor.context(openglSurface));
      assertPointer(0x110, mln_opengl_surface_descriptor.surface(openglSurface));
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
    assertPointer(
        expected.getInstanceProcAddr().address(),
        mln_vulkan_context_descriptor.get_instance_proc_addr(segment));
    assertPointer(
        expected.getDeviceProcAddr().address(),
        mln_vulkan_context_descriptor.get_device_proc_addr(segment));
  }

  private static void assertWglContext(WglContextDescriptor expected, MemorySegment segment) {
    assertEquals(
        MapLibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_WGL(),
        mln_opengl_context_descriptor.platform(segment));
    var wgl = mln_opengl_context_descriptor.data.wgl(mln_opengl_context_descriptor.data(segment));
    assertPointer(
        expected.deviceContext().address(), mln_wgl_context_descriptor.device_context(wgl));
    assertPointer(expected.shareContext().address(), mln_wgl_context_descriptor.share_context(wgl));
    assertPointer(
        expected.getProcAddress().address(), mln_wgl_context_descriptor.get_proc_address(wgl));
  }

  private static void assertEglContext(EglContextDescriptor expected, MemorySegment segment) {
    assertEquals(
        MapLibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_EGL(),
        mln_opengl_context_descriptor.platform(segment));
    var egl = mln_opengl_context_descriptor.data.egl(mln_opengl_context_descriptor.data(segment));
    assertPointer(expected.display().address(), mln_egl_context_descriptor.display(egl));
    assertPointer(expected.config().address(), mln_egl_context_descriptor.config(egl));
    assertPointer(expected.shareContext().address(), mln_egl_context_descriptor.share_context(egl));
    assertPointer(
        expected.getProcAddress().address(), mln_egl_context_descriptor.get_proc_address(egl));
  }

  private static void assertPointer(long expectedAddress, MemorySegment segment) {
    assertEquals(expectedAddress, segment.address());
  }
}
