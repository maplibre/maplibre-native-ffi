package org.maplibre.nativejni.render;

import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.MaplibreException;
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
