package org.maplibre.nativeffi.gradle

import java.io.File
import java.util.Properties

class MaplibreNativeCArtifact(propertiesFile: File) {
  val propertiesFile: File = propertiesFile

  private val properties: Properties =
    propertiesFile.inputStream().use { input -> Properties().apply { load(input) } }

  val libraryPath: File
    get() = File(property("maplibreNativeC.libraryPath"))

  val includeDirs: List<File>
    get() = pathList("maplibreNativeC.includeDirs").map(::File)

  val linkDirs: List<File>
    get() = pathList("maplibreNativeC.linkDirs").map(::File)

  val runtimeLibraryDirs: List<File>
    get() = pathList("maplibreNativeC.runtimeLibraryDirs").map(::File)

  val linkLibraries: List<String>
    get() = pathList("maplibreNativeC.linkLibraries")

  val frameworks: List<String>
    get() = pathList("maplibreNativeC.frameworks")

  private fun property(name: String): String = properties.getProperty(name, "")

  private fun pathList(name: String): List<String> =
    property(name).split(File.pathSeparatorChar).filter { it.isNotBlank() }
}
