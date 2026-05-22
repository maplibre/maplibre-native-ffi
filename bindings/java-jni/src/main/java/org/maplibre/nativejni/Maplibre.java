package org.maplibre.nativejni;

import java.nio.file.Path;
import java.util.Objects;
import org.maplibre.nativejni.internal.bridge.NativeBridge;
import org.maplibre.nativejni.internal.loader.NativeLibrary;

/** Process-global entry points for the Java JNI binding. */
public final class Maplibre {
  private Maplibre() {}

  /** Loads the JNI bridge library using the binding's standard lookup order. */
  public static void loadNativeLibrary() {
    NativeLibrary.load();
  }

  /** Loads the JNI bridge library from an exact file path. */
  public static void loadNativeLibrary(Path libraryPath) {
    NativeLibrary.load(Objects.requireNonNull(libraryPath, "libraryPath"));
  }

  /** Returns the native C ABI contract version. */
  public static long cVersion() {
    NativeLibrary.ensureLoaded();
    return NativeBridge.cVersion();
  }
}
