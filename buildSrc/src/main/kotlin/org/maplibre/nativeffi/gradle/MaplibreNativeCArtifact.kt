package org.maplibre.nativeffi.gradle

import java.io.File

class MaplibreNativeCArtifact(val installDir: File) {
  val libraryPath: File
    get() = runtimeLibraryDir.resolve(libraryFileName())

  val includeDirs: List<File>
    get() = listOf(installDir.resolve("include"))

  val linkDirs: List<File>
    get() = listOf(installDir.resolve("lib"))

  val runtimeLibraryDirs: List<File>
    get() = listOf(runtimeLibraryDir)

  val linkLibraries: List<String>
    get() = listOf("maplibre-native-c")

  val frameworks: List<String>
    get() = emptyList()

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
