package org.maplibre.nativeffi.internal.loader;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.file.Files;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.error.NativeErrorException;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class NativeLibraryTest {
  @Test
  void loadNativeLibrary() {
    NativeTestSupport.resetNativeLibraryLoadedState();
    var libraryPath = NativeTestSupport.nativeLibraryPath();
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

  @Test
  void abiVersionMismatchUsesStableBindingError() {
    var error = assertThrows(NativeErrorException.class, () -> NativeAccess.checkAbiVersion(1));

    assertEquals(MaplibreStatus.NATIVE_ERROR, error.status());
    assertTrue(error.diagnostic().contains("Unsupported Maplibre C ABI version 1"));
    assertTrue(error.diagnostic().contains("expected 0"));
  }

  private static void restoreProperty(String originalProperty) {
    if (originalProperty == null) {
      System.clearProperty(NativeLibrary.LIBRARY_PATH_PROPERTY);
    } else {
      System.setProperty(NativeLibrary.LIBRARY_PATH_PROPERTY, originalProperty);
    }
  }
}
