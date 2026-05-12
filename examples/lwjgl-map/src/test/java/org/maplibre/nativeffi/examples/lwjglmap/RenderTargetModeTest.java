package org.maplibre.nativeffi.examples.lwjglmap;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.Test;

final class RenderTargetModeTest {
  @Test
  void parsesSupportedCliNames() {
    assertEquals(RenderTargetMode.OWNED_TEXTURE, RenderTargetMode.parse("owned-texture"));
    assertEquals(RenderTargetMode.NATIVE_SURFACE, RenderTargetMode.parse("native-surface"));
  }

  @Test
  void rejectsUnknownCliNames() {
    var error = assertThrows(IllegalArgumentException.class, () -> RenderTargetMode.parse("metal"));

    assertEquals("unknown render target 'metal'", error.getMessage());
  }

  @Test
  void describesRenderTargets() {
    assertEquals(
        "samples MapLibre-owned Vulkan frames into the GLFW swapchain",
        RenderTargetMode.OWNED_TEXTURE.status());
    assertEquals(
        "renders directly to the GLFW Vulkan surface", RenderTargetMode.NATIVE_SURFACE.status());
  }
}
