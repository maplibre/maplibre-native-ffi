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
    throw new IllegalStateException(
        "Set %s to an exact native library path for Java FFM tests."
            .formatted(NativeLibrary.LIBRARY_PATH_PROPERTY));
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
