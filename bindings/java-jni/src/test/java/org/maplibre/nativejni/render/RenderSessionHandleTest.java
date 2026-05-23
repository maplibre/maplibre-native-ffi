package org.maplibre.nativejni.render;

import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.MaplibreException;
import org.maplibre.nativejni.internal.bridge.RenderSessionNative;
import org.maplibre.nativejni.internal.bridge.TextureNative;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.map.MapOptions;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.test.NativeTestSupport;

class RenderSessionHandleTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void rejectsNegativeAttachDimensionsBeforeNativeCast() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        var descriptor =
            new MetalOwnedTextureDescriptor().extent(new RenderTargetExtent(-1, 64, 1.0));

        assertThrows(InvalidArgumentException.class, () -> map.attachMetalOwnedTexture(descriptor));
      }
    }
  }

  @Test
  void rejectsNegativeResizeDimensionsBeforeNativeCast() {
    assertThrows(
        InvalidArgumentException.class,
        () -> Status.check(RenderSessionNative.mln_render_session_resize(0, -1, 64, 1.0)));
  }

  @Test
  void rejectsScaledResizeDimensionsOutsideJavaIntRange() {
    assertThrows(
        InvalidArgumentException.class,
        () ->
            Status.check(
                RenderSessionNative.mln_render_session_resize(0, Integer.MAX_VALUE, 64, 2.0)));
  }

  @Test
  void rejectsNegativeVulkanUnsignedFieldsBeforeNativeCast() {
    assertThrows(
        InvalidArgumentException.class,
        () ->
            Status.check(
                TextureNative.mln_vulkan_owned_texture_attach(
                    0, 64, 64, 1.0, 0, 0, 0, 0, -1, new long[1])));
    assertThrows(
        InvalidArgumentException.class,
        () ->
            Status.check(
                TextureNative.mln_vulkan_borrowed_texture_attach(
                    0, 64, 64, 1.0, 0, 0, 0, 0, 0, 0, 0, -1, 0, null, new long[1])));
    assertThrows(
        InvalidArgumentException.class,
        () ->
            Status.check(
                TextureNative.mln_vulkan_borrowed_texture_attach(
                    0, 64, 64, 1.0, 0, 0, 0, 0, 0, 0, 0, 0, -1, null, new long[1])));
    assertThrows(
        InvalidArgumentException.class,
        () ->
            Status.check(
                TextureNative.mln_vulkan_borrowed_texture_attach(
                    0, 64, 64, 1.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, new long[1])));
  }

  @Test
  void attachAttemptsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        assertThrows(
            MaplibreException.class,
            () -> map.attachMetalOwnedTexture(new MetalOwnedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachMetalBorrowedTexture(new MetalBorrowedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachVulkanOwnedTexture(new VulkanOwnedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachVulkanBorrowedTexture(new VulkanBorrowedTextureDescriptor()));
        assertThrows(
            MaplibreException.class, () -> map.attachMetalSurface(new MetalSurfaceDescriptor()));
        assertThrows(
            MaplibreException.class, () -> map.attachVulkanSurface(new VulkanSurfaceDescriptor()));
      }
    }
  }
}
