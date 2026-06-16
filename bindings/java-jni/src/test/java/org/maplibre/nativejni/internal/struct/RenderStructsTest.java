package org.maplibre.nativejni.internal.struct;

import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.render.EglContextDescriptor;
import org.maplibre.nativejni.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.MetalContextDescriptor;
import org.maplibre.nativejni.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativejni.render.MetalSurfaceDescriptor;
import org.maplibre.nativejni.render.NativePointer;
import org.maplibre.nativejni.render.OpenGLBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.OpenGLOwnedTextureDescriptor;
import org.maplibre.nativejni.render.OpenGLSurfaceDescriptor;
import org.maplibre.nativejni.render.RenderTargetExtent;
import org.maplibre.nativejni.render.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.VulkanContextDescriptor;
import org.maplibre.nativejni.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativejni.render.VulkanSurfaceDescriptor;
import org.maplibre.nativejni.render.WglContextDescriptor;

// Support invariant for BND-161 and BND-162: render descriptor structs are the
// JNI materialization seam for backend handles and attach descriptors.
final class RenderStructsTest {
  @Test
  void bnd161AndBnd162RenderTargetDescriptorsMaterializeNestedFields() {
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
    var wglContext =
        new WglContextDescriptor(NativePointer.ofAddress(0x100), NativePointer.ofAddress(0x101))
            .getProcAddress(NativePointer.ofAddress(0x102));
    var eglContext =
        new EglContextDescriptor(
                NativePointer.ofAddress(0x200),
                NativePointer.ofAddress(0x201),
                NativePointer.ofAddress(0x202))
            .getProcAddress(NativePointer.ofAddress(0x203));
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
    var openglOwnedTexture =
        RenderStructs.openglOwnedTextureDescriptor(
            new OpenGLOwnedTextureDescriptor().extent(extent).context(wglContext));
    assertExtent(extent, openglOwnedTexture.extent());
    assertWglContext(wglContext, openglOwnedTexture.context());
    var openglBorrowedTexture =
        RenderStructs.openglBorrowedTextureDescriptor(
            new OpenGLBorrowedTextureDescriptor()
                .extent(extent)
                .context(eglContext)
                .texture(12)
                .target(0x0de1));
    assertExtent(extent, openglBorrowedTexture.extent());
    assertEglContext(eglContext, openglBorrowedTexture.context());
    assertEquals(12, openglBorrowedTexture.texture());
    assertEquals(0x0de1, openglBorrowedTexture.target());
    var openglSurface =
        RenderStructs.openglSurfaceDescriptor(
            new OpenGLSurfaceDescriptor()
                .extent(extent)
                .context(wglContext)
                .surface(NativePointer.ofAddress(0x110)));
    assertExtent(extent, openglSurface.extent());
    assertWglContext(wglContext, openglSurface.context());
    assertEquals(0x110, openglSurface.surface());
  }

  @Test
  void bnd161AndBnd162NativeOpenGLDescriptorsMaterializeUnionFields() {
    var extent = new RenderTargetExtent(640, 480, 2.0);
    var wglContext =
        new WglContextDescriptor(NativePointer.ofAddress(0x100), NativePointer.ofAddress(0x101))
            .getProcAddress(NativePointer.ofAddress(0x102));
    var eglContext =
        new EglContextDescriptor(
                NativePointer.ofAddress(0x200),
                NativePointer.ofAddress(0x201),
                NativePointer.ofAddress(0x202))
            .getProcAddress(NativePointer.ofAddress(0x203));
    var openglOwnedTexture =
        RenderStructs.nativeOpenGLOwnedTextureDescriptor(
            new OpenGLOwnedTextureDescriptor().extent(extent).context(wglContext));
    assertEquals((int) openglOwnedTexture.sizeof(), openglOwnedTexture.size());
    assertNativeExtent(extent, openglOwnedTexture.extent());
    assertNativeWglContext(wglContext, openglOwnedTexture.context());
    var openglBorrowedTexture =
        RenderStructs.nativeOpenGLBorrowedTextureDescriptor(
            new OpenGLBorrowedTextureDescriptor()
                .extent(extent)
                .context(eglContext)
                .texture(12)
                .target(0x0de1));
    assertEquals((int) openglBorrowedTexture.sizeof(), openglBorrowedTexture.size());
    assertNativeExtent(extent, openglBorrowedTexture.extent());
    assertNativeEglContext(eglContext, openglBorrowedTexture.context());
    assertEquals(12, openglBorrowedTexture.texture());
    assertEquals(0x0de1, openglBorrowedTexture.target());
    var openglSurface =
        RenderStructs.nativeOpenGLSurfaceDescriptor(
            new OpenGLSurfaceDescriptor()
                .extent(extent)
                .context(wglContext)
                .surface(NativePointer.ofAddress(0x110)));
    assertEquals((int) openglSurface.sizeof(), openglSurface.size());
    assertNativeExtent(extent, openglSurface.extent());
    assertNativeWglContext(wglContext, openglSurface.context());
    assertEquals(0x110, address(openglSurface.surface()));
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

  private static void assertWglContext(
      WglContextDescriptor expected, RenderStructs.OpenGLContextValue value) {
    var wgl = (RenderStructs.WglContextValue) value;
    assertEquals(expected.deviceContext().address(), wgl.deviceContext());
    assertEquals(expected.shareContext().address(), wgl.shareContext());
    assertEquals(expected.getProcAddress().address(), wgl.getProcAddress());
  }

  private static void assertEglContext(
      EglContextDescriptor expected, RenderStructs.OpenGLContextValue value) {
    var egl = (RenderStructs.EglContextValue) value;
    assertEquals(expected.display().address(), egl.display());
    assertEquals(expected.config().address(), egl.config());
    assertEquals(expected.shareContext().address(), egl.shareContext());
    assertEquals(expected.getProcAddress().address(), egl.getProcAddress());
  }

  private static void assertNativeExtent(
      RenderTargetExtent expected, MaplibreNativeC.mln_render_target_extent value) {
    assertEquals((int) value.sizeof(), value.size());
    assertEquals(expected.width(), value.width());
    assertEquals(expected.height(), value.height());
    assertEquals(expected.scaleFactor(), value.scale_factor(), 0.0);
  }

  private static void assertNativeWglContext(
      WglContextDescriptor expected, MaplibreNativeC.mln_opengl_context_descriptor value) {
    assertEquals((int) value.sizeof(), value.size());
    assertEquals(MaplibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_WGL, value.platform());
    var wgl = value.data_wgl();
    assertEquals((int) wgl.sizeof(), wgl.size());
    assertEquals(expected.deviceContext().address(), address(wgl.device_context()));
    assertEquals(expected.shareContext().address(), address(wgl.share_context()));
    assertEquals(expected.getProcAddress().address(), address(wgl.get_proc_address()));
  }

  private static void assertNativeEglContext(
      EglContextDescriptor expected, MaplibreNativeC.mln_opengl_context_descriptor value) {
    assertEquals((int) value.sizeof(), value.size());
    assertEquals(MaplibreNativeC.MLN_OPENGL_CONTEXT_PLATFORM_EGL, value.platform());
    var egl = value.data_egl();
    assertEquals((int) egl.sizeof(), egl.size());
    assertEquals(expected.display().address(), address(egl.display()));
    assertEquals(expected.config().address(), address(egl.config()));
    assertEquals(expected.shareContext().address(), address(egl.share_context()));
    assertEquals(expected.getProcAddress().address(), address(egl.get_proc_address()));
  }

  private static long address(org.bytedeco.javacpp.Pointer pointer) {
    return pointer == null || pointer.isNull() ? 0 : pointer.address();
  }
}
