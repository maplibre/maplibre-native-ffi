package org.maplibre.nativeffi.gradle

import groovy.json.JsonSlurper
import java.io.File

class MaplibreNativeCArtifact(val installDir: File, val hostLibraryDirs: List<File>) {
  val libraryPath: File
    get() = runtimeLibraryDir.resolve(libraryFileName())

  val includeDirs: List<File>
    get() = listOf(installDir.resolve("include"))

  val linkDirs: List<File>
    get() = listOf(installDir.resolve("lib"))

  val runtimeLibraryDirs: List<File>
    get() = listOf(runtimeLibraryDir)

  val loaderLibraryDirs: List<File>
    get() = runtimeLibraryDirs + hostLibraryDirs

  val runtimeLinkLibraryDirs: List<File>
    get() = runtimeLibraryDirs + hostLibraryDirs

  val linkLibraries: List<String>
    get() = listOf("maplibre-native-c") + artifactStringList("staticLinkLibraries")

  val frameworks: List<String>
    get() = artifactStringList("staticLinkFrameworks")

  private val artifactMetadata: Map<*, *> by lazy {
    val descriptor = installDir.resolve("share/maplibre-native-c/artifact.json")
    if (descriptor.isFile) JsonSlurper().parse(descriptor) as Map<*, *> else emptyMap<Any, Any>()
  }

  private fun artifactStringList(name: String): List<String> =
    (artifactMetadata[name] as? List<*>)?.map { value ->
      require(value is String) { "$name in the native artifact descriptor must contain strings" }
      value
    } ?: emptyList()

  private val runtimeLibraryDir: File
    get() = installDir.resolve(if (targetIsWindows()) "bin" else "lib")

  private fun libraryFileName(): String =
    when {
      targetIsWindows() -> "maplibre-native-c.dll"
      targetIsMac() -> "libmaplibre-native-c.dylib"
      else -> "libmaplibre-native-c.so"
    }

  private fun targetIsMac(): Boolean = cargoBuildTarget()?.contains("apple-darwin") ?: isHostMac()

  private fun targetIsWindows(): Boolean =
    cargoBuildTarget()?.contains("windows-msvc") ?: isHostWindows()

  private fun cargoBuildTarget(): String? = System.getenv("CARGO_BUILD_TARGET")

  private fun isHostMac(): Boolean = System.getProperty("os.name").lowercase().contains("mac")

  private fun isHostWindows(): Boolean =
    System.getProperty("os.name").lowercase().contains("windows")
}
