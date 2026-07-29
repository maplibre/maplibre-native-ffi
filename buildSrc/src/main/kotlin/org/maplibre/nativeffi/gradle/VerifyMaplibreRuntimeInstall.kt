package org.maplibre.nativeffi.gradle

import groovy.json.JsonSlurper
import java.io.File
import org.gradle.api.DefaultTask
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.TaskAction
import org.gradle.work.DisableCachingByDefault

@DisableCachingByDefault(because = "The task only validates a small native artifact descriptor")
abstract class VerifyMaplibreRuntimeInstall : DefaultTask() {
  @get:Input abstract val installDirectoryPath: Property<String>

  @get:Input abstract val installPropertyName: Property<String>

  @get:Input abstract val explicitlyConfigured: Property<Boolean>

  @get:Input abstract val requireExplicitInput: Property<Boolean>

  @get:Input abstract val expectedBackend: Property<String>

  @get:Input abstract val expectedTargetPlatform: Property<String>

  @TaskAction
  fun verify() {
    if (requireExplicitInput.get() && !explicitlyConfigured.get()) {
      error(
        "Publishing this runtime requires -P${installPropertyName.get()}=<native install directory>"
      )
    }

    val installDirectory = File(installDirectoryPath.get())
    val descriptor = installDirectory.resolve("share/maplibre-native-c/artifact.json")
    require(descriptor.isFile) {
      "Native runtime install ${installDirectory.absolutePath} is missing " +
        "share/maplibre-native-c/artifact.json"
    }

    @Suppress("UNCHECKED_CAST") val metadata = JsonSlurper().parse(descriptor) as Map<String, Any?>
    val actualBackend = metadata["renderBackend"]
    val actualTargetPlatform = metadata["targetPlatform"]
    require(actualBackend == expectedBackend.get()) {
      "Native runtime install ${installDirectory.absolutePath} uses backend " +
        "'$actualBackend'; expected '${expectedBackend.get()}'"
    }
    require(actualTargetPlatform == expectedTargetPlatform.get()) {
      "Native runtime install ${installDirectory.absolutePath} targets " +
        "'$actualTargetPlatform'; expected '${expectedTargetPlatform.get()}'"
    }

    val requiredLicenses = buildSet {
      addAll(listOf("icu.txt", "maplibre-native.md", "nunicode.txt", "pmtiles.txt"))
      if (
        expectedTargetPlatform.get().startsWith("android-") ||
          expectedTargetPlatform.get().startsWith("linux-") ||
          expectedTargetPlatform.get().startsWith("windows-")
      ) {
        add("rust.md")
      }
      if (
        expectedTargetPlatform.get().startsWith("linux-") ||
          expectedTargetPlatform.get().startsWith("windows-")
      ) {
        addAll(listOf("libuv-extra.txt", "libuv.txt", "zlib.txt"))
      }
      if (expectedBackend.get() == "vulkan") {
        addAll(listOf("glslang.txt", "vulkan-headers.md", "vulkan-memory-allocator.txt"))
      }
      if (expectedBackend.get() == "opengl" && expectedTargetPlatform.get().startsWith("macos-")) {
        add("angle.txt")
      }
    }
    val licenseDirectory = installDirectory.resolve("share/maplibre-native-c/licenses")
    val actualLicenses =
      licenseDirectory.listFiles()?.filter(File::isFile)?.map(File::getName)?.toSet().orEmpty()
    require(actualLicenses.containsAll(requiredLicenses)) {
      "Native runtime install ${installDirectory.absolutePath} is missing licenses: " +
        (requiredLicenses - actualLicenses).sorted().joinToString()
    }
  }
}
