package org.maplibre.nativejni.test;

import static org.junit.jupiter.api.Assertions.fail;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

import java.nio.file.Files;
import java.nio.file.Path;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.internal.loader.NativeLibrary;

public final class NativeTestSupport {
  public static final String REQUIRE_NATIVE_TESTS_PROPERTY =
      "org.maplibre.nativejni.tests.requireNative";

  private NativeTestSupport() {}

  public static void loadNativeLibraryOrSkip() {
    var libraryPath = configuredLibraryPath();
    var requireNativeTests = Boolean.getBoolean(REQUIRE_NATIVE_TESTS_PROPERTY);
    if (libraryPath != null && !libraryPath.isBlank()) {
      if (!Files.isRegularFile(Path.of(libraryPath))) {
        skipOrFail(requireNativeTests, "Missing JNI bridge: " + libraryPath);
        return;
      }
      Maplibre.loadNativeLibrary();
      return;
    }

    try {
      Maplibre.loadNativeLibrary();
    } catch (UnsatisfiedLinkError error) {
      skipOrFail(
          requireNativeTests,
          "Set -D"
              + NativeLibrary.LIBRARY_PATH_PROPERTY
              + ", "
              + NativeLibrary.LIBRARY_PATH_ENV
              + ", or java.library.path for "
              + NativeLibrary.LIBRARY_NAME
              + ": "
              + error.getMessage());
    }
  }

  public static String configuredLibraryPath() {
    var libraryPath = System.getProperty(NativeLibrary.LIBRARY_PATH_PROPERTY);
    if (libraryPath == null || libraryPath.isBlank()) {
      libraryPath = System.getenv(NativeLibrary.LIBRARY_PATH_ENV);
    }
    return libraryPath;
  }

  private static void skipOrFail(boolean requireNativeTests, String message) {
    if (requireNativeTests) {
      fail(message);
    }
    assumeTrue(false, message);
  }
}
