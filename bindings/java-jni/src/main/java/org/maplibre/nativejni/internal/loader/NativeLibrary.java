package org.maplibre.nativejni.internal.loader;

import java.nio.file.Path;
import java.util.Objects;

/** Loads the JNI bridge library exactly once per class loader. */
public final class NativeLibrary {
  public static final String LIBRARY_PATH_PROPERTY = "org.maplibre.nativejni.library.path";
  public static final String LIBRARY_PATH_ENV = "MAPLIBRE_NATIVE_JNI_LIBRARY_PATH";
  public static final String LIBRARY_NAME = "maplibre-native-jni";

  private static final Object LOCK = new Object();
  private static boolean loaded;

  private NativeLibrary() {}

  public static void ensureLoaded() {
    if (!loaded) {
      load();
    }
  }

  public static void load() {
    synchronized (LOCK) {
      if (loaded) {
        return;
      }
      var configuredPath = System.getProperty(LIBRARY_PATH_PROPERTY);
      if (configuredPath == null || configuredPath.isBlank()) {
        configuredPath = System.getenv(LIBRARY_PATH_ENV);
      }
      if (configuredPath == null || configuredPath.isBlank()) {
        System.loadLibrary(LIBRARY_NAME);
      } else {
        System.load(Path.of(configuredPath).toAbsolutePath().toString());
      }
      loaded = true;
    }
  }

  public static void load(Path libraryPath) {
    Objects.requireNonNull(libraryPath, "libraryPath");
    synchronized (LOCK) {
      if (loaded) {
        return;
      }
      System.load(libraryPath.toAbsolutePath().toString());
      loaded = true;
    }
  }
}
