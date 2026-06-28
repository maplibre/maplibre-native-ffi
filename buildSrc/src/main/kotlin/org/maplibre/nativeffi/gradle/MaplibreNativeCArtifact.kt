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
    get() = installDir.resolve(if (isWindows()) "bin" else "lib")

  private fun libraryFileName(): String =
    when {
      isWindows() -> "maplibre-native-c.dll"
      isMac() -> "libmaplibre-native-c.dylib"
      else -> "libmaplibre-native-c.so"
    }

  private fun isMac(): Boolean = System.getProperty("os.name").lowercase().contains("mac")

  private fun isWindows(): Boolean = System.getProperty("os.name").lowercase().contains("windows")
}
