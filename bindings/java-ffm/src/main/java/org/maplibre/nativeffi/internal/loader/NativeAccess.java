package org.maplibre.nativeffi.internal.loader;

import java.nio.file.Path;
import java.util.NoSuchElementException;
import org.maplibre.nativeffi.error.NativeErrorException;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;

/** Ensures the native library is loaded before any generated downcall runs. */
public final class NativeAccess {
  private static final int EXPECTED_C_ABI_VERSION = 0;
  private static final Object LOCK = new Object();

  private static volatile boolean initialized;

  private NativeAccess() {}

  public static void ensureLoaded() {
    if (initialized) {
      return;
    }

    synchronized (LOCK) {
      if (initialized) {
        return;
      }

      NativeLibrary.load();
      checkNativeAccessAndAbi();
      initialized = true;
    }
  }

  public static void load(Path libraryPath) {
    synchronized (LOCK) {
      NativeLibrary.load(libraryPath);
      checkNativeAccessAndAbi();
      initialized = true;
    }
  }

  private static void checkNativeAccessAndAbi() {
    final int version;
    try {
      version = MapLibreNativeC.mln_c_version();
    } catch (ExceptionInInitializerError error) {
      var cause = deepestCause(error);
      if (cause instanceof IllegalCallerException) {
        throw nativeAccessFailure(cause);
      }
      if (cause instanceof NoSuchElementException || cause instanceof UnsatisfiedLinkError) {
        throw missingSymbols(error);
      }
      throw error;
    } catch (IllegalCallerException error) {
      throw nativeAccessFailure(error);
    } catch (UnsatisfiedLinkError error) {
      throw missingSymbols(error);
    }

    if (version != EXPECTED_C_ABI_VERSION) {
      throw new NativeErrorException(
          0,
          "Unsupported Maplibre C ABI version %d; expected %d"
              .formatted(version, EXPECTED_C_ABI_VERSION));
    }
  }

  private static IllegalStateException nativeAccessFailure(Throwable cause) {
    return new IllegalStateException(
        "Java FFM native access is not enabled. Run the JVM with "
            + "--enable-native-access=ALL-UNNAMED for this classpath build.",
        cause);
  }

  private static UnsatisfiedLinkError missingSymbols(Throwable cause) {
    var missing =
        new UnsatisfiedLinkError(
            "Loaded native library does not expose the Maplibre C ABI symbols.");
    missing.addSuppressed(cause);
    return missing;
  }

  private static Throwable deepestCause(Throwable error) {
    var current = error;
    while (current.getCause() != null) {
      current = current.getCause();
    }
    return current;
  }
}
