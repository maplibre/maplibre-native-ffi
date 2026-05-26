package org.maplibre.nativeffi.examples.lwjglmap;

enum RenderTargetMode {
  OWNED_TEXTURE("owned-texture"),
  BORROWED_TEXTURE("borrowed-texture"),
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
      case OWNED_TEXTURE -> "samples MapLibre-owned frames into the GLFW swapchain";
      case BORROWED_TEXTURE -> "renders into a caller-owned OpenGL texture";
      case NATIVE_SURFACE -> "renders directly to the GLFW native surface";
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
