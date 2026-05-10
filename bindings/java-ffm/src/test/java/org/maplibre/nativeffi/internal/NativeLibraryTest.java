package org.maplibre.nativeffi.internal;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;

final class NativeLibraryTest {
  @Test
  void loadNativeLibrary() {
    var libraryPath = nativeLibraryPath();
    var absoluteLibraryPath = libraryPath.toAbsolutePath().normalize();
    assertTrue(
        Files.isRegularFile(absoluteLibraryPath),
        () -> "Native library not found: " + absoluteLibraryPath);

    var originalProperty = System.getProperty(NativeLibrary.LIBRARY_PATH_PROPERTY);
    try {
      var missingPath =
          absoluteLibraryPath.resolveSibling(
              "missing-" + System.nanoTime() + "-" + absoluteLibraryPath.getFileName());
      assertFalse(Files.exists(missingPath));

      // Missing configured paths report the source and exact missing path.
      System.setProperty(NativeLibrary.LIBRARY_PATH_PROPERTY, missingPath.toString());
      var error = assertThrows(UnsatisfiedLinkError.class, NativeLibrary::load);
      assertTrue(error.getMessage().contains(NativeLibrary.LIBRARY_PATH_PROPERTY));
      assertTrue(error.getMessage().contains(missingPath.toString()));

      // Explicit paths load the native C ABI and make its symbols visible to jextract.
      NativeLibrary.load(absoluteLibraryPath);
      assertTrue(NativeLibrary.isLoaded());
      assertEquals(absoluteLibraryPath, NativeLibrary.loadedPath().orElseThrow());
      assertEquals(0, MapLibreNativeC.mln_c_version());

      // Later load calls are no-ops, even if configuration changes.
      System.setProperty(NativeLibrary.LIBRARY_PATH_PROPERTY, missingPath.toString());
      NativeLibrary.load();
      assertEquals(absoluteLibraryPath, NativeLibrary.loadedPath().orElseThrow());
      assertEquals(0, MapLibreNativeC.mln_c_version());
    } finally {
      restoreProperty(originalProperty);
    }
  }

  private static Path nativeLibraryPath() {
    var buildDirectory = System.getenv("MLN_FFI_BUILD_DIR");
    if (hasText(buildDirectory)) {
      return Path.of(buildDirectory).resolve(System.mapLibraryName(NativeLibrary.LIBRARY_NAME));
    }

    var property = System.getProperty(NativeLibrary.LIBRARY_PATH_PROPERTY);
    if (hasText(property)) {
      return Path.of(property);
    }

    var environment = System.getenv(NativeLibrary.LIBRARY_PATH_ENV);
    if (hasText(environment)) {
      return Path.of(environment);
    }

    throw new IllegalStateException(
        "Set MLN_FFI_BUILD_DIR, %s, or %s for Java FFM native-library tests."
            .formatted(NativeLibrary.LIBRARY_PATH_PROPERTY, NativeLibrary.LIBRARY_PATH_ENV));
  }

  private static boolean hasText(String value) {
    return value != null && !value.isBlank();
  }

  private static void restoreProperty(String originalProperty) {
    if (originalProperty == null) {
      System.clearProperty(NativeLibrary.LIBRARY_PATH_PROPERTY);
    } else {
      System.setProperty(NativeLibrary.LIBRARY_PATH_PROPERTY, originalProperty);
    }
  }
}
