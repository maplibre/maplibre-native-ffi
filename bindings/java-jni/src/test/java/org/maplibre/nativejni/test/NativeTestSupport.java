package org.maplibre.nativejni.test;

import static org.junit.jupiter.api.Assumptions.assumeTrue;

import java.nio.file.Files;
import java.nio.file.Path;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.internal.loader.NativeLibrary;

public final class NativeTestSupport {
  private NativeTestSupport() {}

  public static void loadNativeLibraryOrSkip() {
    var libraryPath = System.getProperty(NativeLibrary.LIBRARY_PATH_PROPERTY);
    assumeTrue(
        libraryPath != null && !libraryPath.isBlank(),
        () -> "Set -D" + NativeLibrary.LIBRARY_PATH_PROPERTY + " to a built JNI bridge library");
    assumeTrue(
        Files.isRegularFile(Path.of(libraryPath)), () -> "Missing JNI bridge: " + libraryPath);
    Maplibre.loadNativeLibrary(Path.of(libraryPath));
  }
}
