package org.maplibre.nativejni.test;

import java.nio.file.Files;
import java.nio.file.Path;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.internal.loader.NativeLibrary;

public final class NativeTestSupport {
  private NativeTestSupport() {}

  public static void loadNativeLibrary() {
    var libraryPath = configuredLibraryPath();
    if (libraryPath == null || libraryPath.isBlank()) {
      throw new IllegalStateException(
          "Set "
              + NativeLibrary.LIBRARY_PATH_PROPERTY
              + " (system property) or "
              + NativeLibrary.LIBRARY_PATH_ENV
              + " (env var) to an exact JNI bridge path for Java JNI tests.");
    }
    var path = Path.of(libraryPath);
    if (!Files.isRegularFile(path)) {
      throw new AssertionError("Missing JNI bridge: " + path);
    }
    Maplibre.loadNativeLibrary(path);
  }

  public static String configuredLibraryPath() {
    var libraryPath = System.getProperty(NativeLibrary.LIBRARY_PATH_PROPERTY);
    if (libraryPath == null || libraryPath.isBlank()) {
      libraryPath = System.getenv(NativeLibrary.LIBRARY_PATH_ENV);
    }
    return libraryPath;
  }
}
