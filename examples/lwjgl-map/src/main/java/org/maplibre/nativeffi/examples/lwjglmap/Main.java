package org.maplibre.nativeffi.examples.lwjglmap;

import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.render.RenderBackend;

public final class Main {
  private Main() {}

  public static void main(String[] args) throws Exception {
    var mode = parseArgs(args);
    if (mode == null) {
      return;
    }
    var backends = Maplibre.supportedRenderBackends();
    System.out.println("native render backends: " + backends);
    if (!supportsUsableBackend(backends)) {
      throw new IllegalStateException(
          "The loaded MapLibre native library does not support a backend usable by lwjgl-map on"
              + " this platform");
    }
    Maplibre.setLogCallback(
        record -> {
          System.err.printf(
              "MapLibre %s %s %d: %s%n",
              record.severity(), record.event(), record.code(), record.message());
          return true;
        });
    var propertyPath = System.getProperty("org.maplibre.nativeffi.library.path");
    if (propertyPath != null) {
      System.out.println("MapLibre native library: " + propertyPath);
    }

    try {
      Shell.run(mode, backends);
    } finally {
      Maplibre.clearLogCallback();
    }
  }

  private static RenderTargetMode parseArgs(String[] args) {
    if (args.length == 1 && args[0].equals("--help")) {
      printUsage();
      return null;
    }
    if (args.length != 1 || args[0].startsWith("-")) {
      printUsage();
      System.exit(1);
    }
    try {
      return RenderTargetMode.parse(args[0]);
    } catch (IllegalArgumentException error) {
      System.err.println(error.getMessage());
      printUsage();
      System.exit(1);
      throw error;
    }
  }

  private static void printUsage() {
    System.err.println(
        """
        Usage: lwjgl-map <mode>

        Modes:
          owned-texture     session-owned texture render target
          borrowed-texture  caller-owned texture render target
          native-surface    native surface render target
        """);
  }

  private static boolean supportsUsableBackend(java.util.Set<RenderBackend> backends) {
    if (GraphicsContext.isMac()) {
      return backends.contains(RenderBackend.METAL) || backends.contains(RenderBackend.VULKAN);
    }
    return backends.contains(RenderBackend.OPENGL) || backends.contains(RenderBackend.VULKAN);
  }
}
