package org.maplibre.nativeffi.internal.loader;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Objects;
import java.util.Optional;

/** Loads the native Maplibre C ABI library for the Java FFM binding. */
public final class NativeLibrary {
  public static final String LIBRARY_NAME = "maplibre-native-c";
  public static final String LIBRARY_PATH_PROPERTY = "org.maplibre.nativeffi.library.path";
  public static final String LIBRARY_PATH_ENV = "MAPLIBRE_NATIVE_FFI_LIBRARY_PATH";

  private static final Object LOCK = new Object();

  private static volatile LoadedLibrary loadedLibrary;

  private NativeLibrary() {}

  /**
   * Loads the native library once.
   *
   * <p>Lookup order:
   *
   * <ol>
   *   <li>exact library file path from {@value #LIBRARY_PATH_PROPERTY};
   *   <li>exact library file path from {@value #LIBRARY_PATH_ENV};
   *   <li>{@link System#loadLibrary(String)} with {@value #LIBRARY_NAME}.
   * </ol>
   */
  public static void load() {
    if (isLoaded()) {
      return;
    }

    synchronized (LOCK) {
      if (isLoaded()) {
        return;
      }

      var configuredPath =
          configuredPath(LIBRARY_PATH_PROPERTY, System.getProperty(LIBRARY_PATH_PROPERTY));
      if (configuredPath == null) {
        configuredPath = configuredPath(LIBRARY_PATH_ENV, System.getenv(LIBRARY_PATH_ENV));
      }

      if (configuredPath != null) {
        loadPath(configuredPath.path(), configuredPath.source());
        return;
      }

      try {
        System.loadLibrary(LIBRARY_NAME);
        loadedLibrary = new LoadedLibrary(null, "java.library.path");
      } catch (UnsatisfiedLinkError error) {
        throw loadFailure(error);
      }
    }
  }

  /** Loads an exact native library file path once. */
  public static void load(Path libraryPath) {
    Objects.requireNonNull(libraryPath, "libraryPath");

    if (isLoaded()) {
      return;
    }

    synchronized (LOCK) {
      if (isLoaded()) {
        return;
      }

      loadPath(libraryPath, "explicit path");
    }
  }

  public static boolean isLoaded() {
    return loadedLibrary != null;
  }

  public static Optional<Path> loadedPath() {
    var loaded = loadedLibrary;
    return loaded == null ? Optional.empty() : Optional.ofNullable(loaded.path());
  }

  public static Optional<String> loadedSource() {
    var loaded = loadedLibrary;
    return loaded == null ? Optional.empty() : Optional.of(loaded.source());
  }

  private static void loadPath(Path libraryPath, String source) {
    var absolutePath = libraryPath.toAbsolutePath().normalize();
    if (!Files.isRegularFile(absolutePath)) {
      throw new UnsatisfiedLinkError(
          "Native library from %s is not a regular file: %s".formatted(source, absolutePath));
    }

    System.load(absolutePath.toString());
    loadedLibrary = new LoadedLibrary(absolutePath, source);
  }

  private static UnsatisfiedLinkError loadFailure(UnsatisfiedLinkError cause) {
    var error =
        new UnsatisfiedLinkError(
            ("Unable to load native library %s. Set %s or %s to an exact library file path, or "
                    + "make the library available on java.library.path.")
                .formatted(LIBRARY_NAME, LIBRARY_PATH_PROPERTY, LIBRARY_PATH_ENV));
    error.addSuppressed(cause);
    return error;
  }

  private static ConfiguredPath configuredPath(String source, String value) {
    if (value == null || value.isBlank()) {
      return null;
    }
    return new ConfiguredPath(Path.of(value), source);
  }

  private record ConfiguredPath(Path path, String source) {}

  private record LoadedLibrary(Path path, String source) {}
}
