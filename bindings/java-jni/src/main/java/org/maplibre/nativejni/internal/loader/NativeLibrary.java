package org.maplibre.nativejni.internal.loader;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Objects;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/** Loads the JavaCPP JNI bridge library exactly once per class loader. */
public final class NativeLibrary {
  public static final String LIBRARY_PATH_PROPERTY = "org.maplibre.nativejni.library.path";
  public static final String LIBRARY_PATH_ENV = "MAPLIBRE_NATIVE_JNI_LIBRARY_PATH";
  public static final String LIBRARY_NAME = "jniMaplibreNativeC";

  private static final Object LOCK = new Object();
  private static volatile boolean loaded;

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
      if (configuredPath != null && !configuredPath.isBlank()) {
        var path = Path.of(configuredPath).toAbsolutePath();
        if (Files.isRegularFile(path)) {
          loadExact(path);
          return;
        }
        prependJavaLibraryPath(path);
      }
      loadJavaCppBridge();
    }
  }

  public static void load(Path libraryPath) {
    Objects.requireNonNull(libraryPath, "libraryPath");
    synchronized (LOCK) {
      if (loaded) {
        return;
      }
      loadExact(libraryPath.toAbsolutePath());
    }
  }

  private static void loadExact(Path libraryPath) {
    System.load(libraryPath.toString());
    loaded = true;
  }

  private static void loadJavaCppBridge() {
    MaplibreNativeC.mln_c_version();
    loaded = true;
  }

  private static void prependJavaLibraryPath(Path directory) {
    if (directory == null) {
      return;
    }
    var path = System.getProperty("java.library.path", "");
    var prefix = directory.toString();
    if (path.isBlank()) {
      System.setProperty("java.library.path", prefix);
    } else if (!path.equals(prefix) && !path.startsWith(prefix + File.pathSeparator)) {
      System.setProperty("java.library.path", prefix + File.pathSeparator + path);
    }
  }
}
