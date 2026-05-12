package org.maplibre.nativeffi.examples.lwjglmap;

enum RenderTargetMode {
  OWNED_TEXTURE("owned-texture"),
  NATIVE_SURFACE("native-surface");

  private final String cliName;

  RenderTargetMode(String cliName) {
    this.cliName = cliName;
  }

  String cliName() {
    return cliName;
  }

  String status() {
    return switch (this) {
      case OWNED_TEXTURE -> "samples MapLibre-owned Vulkan frames into the GLFW swapchain";
      case NATIVE_SURFACE -> "renders directly to the GLFW Vulkan surface";
    };
  }

  static RenderTargetMode parse(String value) {
    for (var mode : values()) {
      if (mode.cliName.equals(value)) {
        return mode;
      }
    }
    throw new IllegalArgumentException("unknown render target '" + value + "'");
  }
}
