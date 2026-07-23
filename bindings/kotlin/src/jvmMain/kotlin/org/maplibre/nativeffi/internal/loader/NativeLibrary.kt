package org.maplibre.nativeffi.internal.loader

import java.net.URL
import java.nio.file.AtomicMoveNotSupportedException
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.StandardCopyOption
import java.security.MessageDigest
import java.util.Optional

/** Loads the native Maplibre C ABI library for the Kotlin/JVM FFM bridge. */
internal object NativeLibrary {
  const val LIBRARY_NAME: String = "maplibre-native-c"
  const val LIBRARY_PATH_PROPERTY: String = "org.maplibre.nativeffi.library.path"
  const val LIBRARY_PATH_ENV: String = "MAPLIBRE_NATIVE_FFI_LIBRARY_PATH"
  const val LIBRARY_DIRS_PROPERTY: String = "org.maplibre.nativeffi.library.dirs"
  const val LIBRARY_DIRS_ENV: String = "MAPLIBRE_NATIVE_FFI_LIBRARY_DIRS"
  private const val RESOURCE_ROOT: String = "META-INF/maplibre-native-ffi"

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
   * 3. the matching native runtime JAR on the classpath
   * 4. [System.loadLibrary] with [LIBRARY_NAME]
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
        if (loadClasspathRuntime()) {
          return
        }
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

  private fun loadClasspathRuntime(): Boolean {
    val classifier = nativeClassifier()
    val resourceDirectory = "$RESOURCE_ROOT/$classifier"
    val libraryResourceName = "$resourceDirectory/${System.mapLibraryName(LIBRARY_NAME)}"
    val classLoader = NativeLibrary::class.java.classLoader
    val libraryResources = classLoader.getResources(libraryResourceName).toList()
    if (libraryResources.isEmpty()) {
      return false
    }
    if (libraryResources.size > 1) {
      throw UnsatisfiedLinkError(
        "Multiple MapLibre native runtime JARs provide $classifier: " +
          libraryResources.joinToString()
      )
    }

    val libraryResource = libraryResources.single()
    val container = resourceContainer(libraryResource)
    val resources =
      (runtimeDependencyFileNames() + System.mapLibraryName(LIBRARY_NAME)).distinct().mapNotNull {
        fileName ->
        val resourceName = "$resourceDirectory/$fileName"
        val resource =
          classLoader.getResources(resourceName).toList().firstOrNull {
            resourceContainer(it) == container
          }
        resource?.let {
          RuntimeResource(fileName, it.openStream().use { stream -> stream.readBytes() })
        }
      }

    val library = resources.lastOrNull()
    if (library?.name != System.mapLibraryName(LIBRARY_NAME)) {
      throw UnsatisfiedLinkError("Native runtime JAR is missing $libraryResourceName")
    }

    val digest =
      MessageDigest.getInstance("SHA-256").run {
        resources.forEach { resource ->
          update(resource.name.toByteArray(Charsets.UTF_8))
          update(resource.bytes)
        }
        digest().joinToString("") { byte -> "%02x".format(byte.toInt() and 0xff) }
      }
    val extractionDirectory =
      Path.of(System.getProperty("java.io.tmpdir"), "maplibre-native-ffi", classifier, digest)
    Files.createDirectories(extractionDirectory)

    val extracted = resources.associateWith { resource ->
      extractResource(resource, extractionDirectory)
    }
    resources.dropLast(1).forEach { resource ->
      System.load(extracted.getValue(resource).toString())
    }
    val libraryPath = extracted.getValue(library)
    System.load(libraryPath.toString())
    loadedLibrary = LoadedLibrary(libraryPath, "classpath resource $libraryResource")
    return true
  }

  private fun extractResource(resource: RuntimeResource, directory: Path): Path {
    val destination = directory.resolve(resource.name)
    if (
      Files.isRegularFile(destination) &&
        Files.size(destination) == resource.bytes.size.toLong() &&
        MessageDigest.isEqual(sha256(destination), resource.sha256)
    ) {
      return destination
    }

    val temporary = Files.createTempFile(directory, ".${resource.name}.", ".tmp")
    try {
      Files.write(temporary, resource.bytes)
      try {
        Files.move(
          temporary,
          destination,
          StandardCopyOption.ATOMIC_MOVE,
          StandardCopyOption.REPLACE_EXISTING,
        )
      } catch (_: AtomicMoveNotSupportedException) {
        Files.move(temporary, destination, StandardCopyOption.REPLACE_EXISTING)
      }
    } finally {
      Files.deleteIfExists(temporary)
    }
    return destination
  }

  private fun sha256(path: Path): ByteArray {
    val digest = MessageDigest.getInstance("SHA-256")
    Files.newInputStream(path).use { input ->
      val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
      while (true) {
        val byteCount = input.read(buffer)
        if (byteCount < 0) {
          break
        }
        digest.update(buffer, 0, byteCount)
      }
    }
    return digest.digest()
  }

  private fun resourceContainer(resource: URL): String {
    val externalForm = resource.toExternalForm()
    val separator = externalForm.indexOf("!/")
    return if (separator >= 0) externalForm.substring(0, separator) else externalForm
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
          "the classpath or java.library.path. Set $LIBRARY_DIRS_PROPERTY or $LIBRARY_DIRS_ENV when host " +
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

  private fun nativeClassifier(): String {
    val osName = System.getProperty("os.name").lowercase()
    val architecture = System.getProperty("os.arch").lowercase()
    val arm64 = architecture == "aarch64" || architecture == "arm64"
    return when {
      osName.contains("windows") && arm64 -> "natives-windows-arm64"
      osName.contains("windows") -> "natives-windows-x64"
      osName.contains("mac") && arm64 -> "natives-macos-arm64"
      osName.contains("mac") -> "natives-macos-x64"
      osName.contains("linux") && arm64 -> "natives-linux-arm64"
      osName.contains("linux") -> "natives-linux-x64"
      else ->
        throw UnsatisfiedLinkError("Unsupported native runtime platform: $osName/$architecture")
    }
  }

  private data class ConfiguredPath(val path: Path, val source: String)

  private data class LoadedLibrary(val path: Path?, val source: String)

  private data class RuntimeResource(val name: String, val bytes: ByteArray) {
    val sha256: ByteArray = MessageDigest.getInstance("SHA-256").digest(bytes)
  }
}
