package org.maplibre.nativeffi.internal.loader

import java.nio.file.Files
import java.nio.file.Path
import java.util.Optional

/** Loads the native Maplibre C ABI library for the Kotlin/JVM FFM bridge. */
internal object NativeLibrary {
  const val LIBRARY_NAME: String = "maplibre-native-c"
  const val LIBRARY_PATH_PROPERTY: String = "org.maplibre.nativeffi.library.path"
  const val LIBRARY_PATH_ENV: String = "MAPLIBRE_NATIVE_FFI_LIBRARY_PATH"
  const val LIBRARY_DIRS_PROPERTY: String = "org.maplibre.nativeffi.library.dirs"
  const val LIBRARY_DIRS_ENV: String = "MAPLIBRE_NATIVE_FFI_LIBRARY_DIRS"

  private val lock = Any()
  private val loadedRuntimeDependencies = mutableSetOf<Path>()

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
        loadRuntimeDependencies(configuredRuntimeDependencyDirs())
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

    loadRuntimeDependenciesForPath(absolutePath)
    System.load(absolutePath.toString())
    loadedLibrary = LoadedLibrary(absolutePath, source)
  }

  private fun loadRuntimeDependenciesForPath(libraryPath: Path) {
    loadRuntimeDependencies(listOfNotNull(libraryPath.parent) + configuredRuntimeDependencyDirs())
  }

  private fun loadRuntimeDependencies(directories: Iterable<Path>) {
    directories.forEach { directory ->
      runtimeDependencyFileNames().forEach { fileName ->
        val path = directory.resolve(fileName).toAbsolutePath().normalize()
        if (!Files.isRegularFile(path)) {
          return@forEach
        }
        val canonicalPath = path.toRealPath()
        if (!loadedRuntimeDependencies.add(canonicalPath)) {
          return@forEach
        }

        try {
          System.load(canonicalPath.toString())
        } catch (error: UnsatisfiedLinkError) {
          loadedRuntimeDependencies.remove(canonicalPath)
          throw UnsatisfiedLinkError(
              "Unable to load MapLibre native runtime dependency from $canonicalPath"
            )
            .also { it.addSuppressed(error) }
        }
      }
    }
  }

  private fun loadFailure(cause: UnsatisfiedLinkError): UnsatisfiedLinkError {
    val error =
      UnsatisfiedLinkError(
        "Unable to load native library $LIBRARY_NAME. Set $LIBRARY_PATH_PROPERTY or " +
          "$LIBRARY_PATH_ENV to an exact library file path, or make the library available on " +
          "java.library.path. Set $LIBRARY_DIRS_PROPERTY or $LIBRARY_DIRS_ENV when host " +
          "runtime dependency directories are needed."
      )
    error.addSuppressed(cause)
    return error
  }

  private fun configuredPath(source: String, value: String?): ConfiguredPath? =
    value?.takeUnless { it.isBlank() }?.let { ConfiguredPath(Path.of(it), source) }

  private fun configuredRuntimeDependencyDirs(): List<Path> =
    configuredPathList(System.getProperty(LIBRARY_DIRS_PROPERTY))
      ?: configuredPathList(System.getenv(LIBRARY_DIRS_ENV))
      ?: emptyList()

  private fun configuredPathList(value: String?): List<Path>? =
    value
      ?.takeUnless { it.isBlank() }
      ?.split(System.getProperty("path.separator"))
      ?.filter { it.isNotBlank() }
      ?.map { Path.of(it) }

  private fun runtimeDependencyFileNames(): List<String> {
    val osName = System.getProperty("os.name").lowercase()
    return when {
      osName.contains("windows") ->
        listOf("vulkan-1.dll", "libEGL.dll", "libGLESv2.dll", "EGL.dll", "GLESv2.dll")
      osName.contains("mac") ->
        listOf("libvulkan.1.dylib", "libvulkan.dylib", "libEGL.dylib", "libGLESv2.dylib")
      else ->
        listOf(
          "libvulkan.so.1",
          "libvulkan.so",
          "libEGL.so.1",
          "libEGL.so",
          "libGLESv2.so.2",
          "libGLESv2.so",
        )
    }
  }

  private data class ConfiguredPath(val path: Path, val source: String)

  private data class LoadedLibrary(val path: Path?, val source: String)
}
