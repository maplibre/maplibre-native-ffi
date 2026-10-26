package org.maplibre.nativeffi

import java.nio.file.Path
import org.maplibre.nativeffi.internal.loader.NativeAccess

/**
 * Loads the native library from a runtime JAR on the classpath, or else with
 * `System.loadLibrary("maplibre-native-c")`, which searches `java.library.path`.
 */
public actual fun loadNativeLibrary() {
  NativeAccess.ensureLoaded()
}

/**
 * Loads the native library from an exact file path, after the graphics libraries that sit beside
 * it.
 */
public fun loadNativeLibrary(libraryPath: Path) {
  NativeAccess.load(libraryPath)
}
