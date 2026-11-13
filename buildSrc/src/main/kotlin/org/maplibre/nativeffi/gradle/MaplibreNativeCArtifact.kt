package org.maplibre.nativeffi.gradle

import groovy.json.JsonSlurper
import java.io.File

/** The C library in one install prefix, as this host's linker and JVM see it. */
class MaplibreNativeCArtifact(val installDir: File) {
  val runtimeLibraryDir: File
    get() = installDir.resolve(if (HostPlatform.current().isWindows) "bin" else "lib")

  /** The tests/graphics library that a test build installs beside the C library. */
  val testGraphicsLibraryPath: File
    get() =
      runtimeLibraryDir.resolve(
        when {
          HostPlatform.current().isWindows -> "mln_test_graphics.dll"
          HostPlatform.current().isMac -> "libmln_test_graphics.dylib"
          else -> "libmln_test_graphics.so"
        },
      )

  val linkDir: File
    get() = installDir.resolve("lib")

  /**
   * The libraries that a link of the C library names: the shared library where the install has one,
   * which carries its own dependencies, or else the static archive and what it needs. An MSVC
   * archive's needs, such as `legacy_stdio_definitions`, don't exist for Kotlin/Native's MinGW
   * linker, which links the DLL.
   */
  val linkLibraries: List<String>
    get() = listOf("maplibre-native-c") + staticOnly("libraries")

  /** The Apple frameworks that the static archive needs, where the C library is only that. */
  val frameworks: List<String>
    get() = staticOnly("frameworks")

  private val hasSharedLibrary: Boolean
    get() =
      listOf(
          "bin/maplibre-native-c.dll",
          "lib/libmaplibre-native-c.so",
          "lib/libmaplibre-native-c.dylib",
        )
        .any { installDir.resolve(it).isFile }

  private fun staticOnly(name: String): List<String> =
    if (hasSharedLibrary) emptyList() else linkStringList(name)

  private val artifactMetadata: Map<*, *> by lazy {
    val descriptor = installDir.resolve(NATIVE_DESCRIPTOR_PATH)
    if (descriptor.isFile) JsonSlurper().parse(descriptor) as Map<*, *> else emptyMap<Any, Any>()
  }

  private fun linkStringList(name: String): List<String> =
    ((artifactMetadata["link"] as? Map<*, *>)?.get(name) as? List<*>)?.map { value ->
      require(value is String) {
        "link.$name in the native artifact descriptor must contain strings"
      }
      value
    } ?: emptyList()
}
