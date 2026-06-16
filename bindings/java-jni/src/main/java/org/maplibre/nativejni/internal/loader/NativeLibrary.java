package org.maplibre.nativejni.internal.loader;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Objects;
import org.maplibre.nativejni.error.AbiVersionMismatchException;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/** Loads the JavaCPP JNI bridge library exactly once per class loader. */
public final class NativeLibrary {
  public static final String LIBRARY_PATH_PROPERTY = "org.maplibre.nativejni.library.path";
  public static final String LIBRARY_PATH_ENV = "MAPLIBRE_NATIVE_JNI_LIBRARY_PATH";
  public static final String LIBRARY_NAME = "jniMaplibreNativeC";

  private static final int EXPECTED_C_ABI_VERSION = 0;
  private static final Object LOCK = new Object();
  private static volatile boolean loaded;
  private static Path loadedExactLibrary;

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
        if (Files.isDirectory(path)) {
          prependJavaLibraryPath(path);
        } else if (Files.exists(path)) {
          throw new UnsatisfiedLinkError(
              "Configured library path exists but is not a regular file or directory: " + path);
        } else {
          throw new UnsatisfiedLinkError("Configured library path does not exist: " + path);
        }
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
    if (!libraryPath.equals(loadedExactLibrary)) {
      System.load(libraryPath.toString());
      loadedExactLibrary = libraryPath;
    }
    checkCAbiVersion(true);
    loaded = true;
  }

  private static void loadJavaCppBridge() {
    checkCAbiVersion(false);
    loaded = true;
  }

  private static void checkCAbiVersion(boolean exactLibraryLoaded) {
    final int version;
    try {
      version = MaplibreNativeC.mln_c_version();
    } catch (UnsatisfiedLinkError error) {
      var missing = new UnsatisfiedLinkError(loadFailureMessage(exactLibraryLoaded));
      missing.addSuppressed(error);
      throw missing;
    }
    checkCAbiVersion(version, EXPECTED_C_ABI_VERSION);
  }

  static void checkCAbiVersion(int version, int expectedVersion) {
    if (version != expectedVersion) {
      throw new AbiVersionMismatchException(version, expectedVersion);
    }
  }

  private static String loadFailureMessage(boolean exactLibraryLoaded) {
    return exactLibraryLoaded
        ? "Loaded native library does not expose the Maplibre C ABI symbols."
        : "Unable to load the JavaCPP JNI bridge library. Set "
            + LIBRARY_PATH_PROPERTY
            + ", "
            + LIBRARY_PATH_ENV
            + ", or java.library.path for "
            + LIBRARY_NAME
            + ".";
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
