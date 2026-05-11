package org.maplibre.nativeffi.test;

import java.lang.reflect.Field;
import java.nio.file.Path;
import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.internal.loader.NativeLibrary;

public final class NativeTestSupport {
  private NativeTestSupport() {}

  public static void loadNativeLibrary() {
    Maplibre.loadNativeLibrary(nativeLibraryPath());
  }

  public static Path nativeLibraryPath() {
    var property = System.getProperty(NativeLibrary.LIBRARY_PATH_PROPERTY);
    if (hasText(property)) {
      return Path.of(property);
    }

    var environment = System.getenv(NativeLibrary.LIBRARY_PATH_ENV);
    if (hasText(environment)) {
      return Path.of(environment);
    }

    var buildDirectory = System.getenv("MLN_FFI_BUILD_DIR");
    if (hasText(buildDirectory)) {
      return Path.of(buildDirectory).resolve(System.mapLibraryName(NativeLibrary.LIBRARY_NAME));
    }

    throw new IllegalStateException(
        "Set MLN_FFI_BUILD_DIR, %s, or %s for Java FFM native-library tests."
            .formatted(NativeLibrary.LIBRARY_PATH_PROPERTY, NativeLibrary.LIBRARY_PATH_ENV));
  }

  public static void resetNativeLibraryLoadedState() {
    try {
      Field loadedLibrary = NativeLibrary.class.getDeclaredField("loadedLibrary");
      loadedLibrary.setAccessible(true);
      loadedLibrary.set(null, null);
    } catch (ReflectiveOperationException error) {
      throw new AssertionError(error);
    }
  }

  private static boolean hasText(String value) {
    return value != null && !value.isBlank();
  }
}
