package org.maplibre.nativeffi.internal.loader

import java.nio.file.Files
import java.nio.file.Path
import java.util.Optional

/** Loads the native Maplibre C ABI library for the Kotlin/JVM FFM bridge. */
internal object NativeLibrary {
  const val LIBRARY_NAME: String = "maplibre-native-c"
  const val LIBRARY_PATH_PROPERTY: String = "org.maplibre.nativeffi.library.path"
  const val LIBRARY_PATH_ENV: String = "MAPLIBRE_NATIVE_FFI_LIBRARY_PATH"

  private val lock = Any()

  @Volatile private var loadedLibrary: LoadedLibrary? = null

  /**
   * Loads the native library once.
   *
   * Lookup order:
   *
   * 1. exact library file path from [LIBRARY_PATH_PROPERTY]
   * 2. exact library file path from [LIBRARY_PATH_ENV]
   * 3. [System.loadLibrary] with [LIBRARY_NAME]
   */
  fun load() {
    if (isLoaded()) {
      return
    }

    synchronized(lock) {
      if (isLoaded()) {
        return
      }

      val configuredPath =
        configuredPath(LIBRARY_PATH_PROPERTY, System.getProperty(LIBRARY_PATH_PROPERTY))
          ?: configuredPath(LIBRARY_PATH_ENV, System.getenv(LIBRARY_PATH_ENV))

      if (configuredPath != null) {
        loadPath(configuredPath.path, configuredPath.source)
        return
      }

      try {
        System.loadLibrary(LIBRARY_NAME)
        loadedLibrary = LoadedLibrary(null, "java.library.path")
      } catch (error: UnsatisfiedLinkError) {
        throw loadFailure(error)
      }
    }
  }

  /** Loads an exact native library file path once. */
  fun load(libraryPath: Path) {
    if (isLoaded()) {
      return
    }

    synchronized(lock) {
      if (isLoaded()) {
        return
      }

      loadPath(libraryPath, "explicit path")
    }
  }

  fun isLoaded(): Boolean = loadedLibrary != null

  fun loadedPath(): Optional<Path> = Optional.ofNullable(loadedLibrary?.path)

  fun loadedSource(): Optional<String> = Optional.ofNullable(loadedLibrary?.source)

  private fun loadPath(libraryPath: Path, source: String) {
    val absolutePath = libraryPath.toAbsolutePath().normalize()
    if (!Files.isRegularFile(absolutePath)) {
      throw UnsatisfiedLinkError("Native library from $source is not a regular file: $absolutePath")
    }

    System.load(absolutePath.toString())
    loadedLibrary = LoadedLibrary(absolutePath, source)
  }

  private fun loadFailure(cause: UnsatisfiedLinkError): UnsatisfiedLinkError {
    val error =
      UnsatisfiedLinkError(
        "Unable to load native library $LIBRARY_NAME. Set $LIBRARY_PATH_PROPERTY or " +
          "$LIBRARY_PATH_ENV to an exact library file path, or make the library available on " +
          "java.library.path."
      )
    error.addSuppressed(cause)
    return error
  }

  private fun configuredPath(source: String, value: String?): ConfiguredPath? =
    value?.takeUnless { it.isBlank() }?.let { ConfiguredPath(Path.of(it), source) }

  private data class ConfiguredPath(val path: Path, val source: String)

  private data class LoadedLibrary(val path: Path?, val source: String)
}
