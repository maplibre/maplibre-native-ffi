package org.maplibre.nativeffi.gradle

import groovy.json.JsonSlurper
import java.io.File

/** The C library in one install prefix, as this host's linker and JVM see it. */
class MaplibreNativeCArtifact(val installDir: File) {
  /** The directory that holds the shared libraries. */
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

  val linkLibraries: List<String>
    get() = listOf("maplibre-native-c") + linkStringList("libraries")

  val frameworks: List<String>
    get() = linkStringList("frameworks")

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
