package org.maplibre.nativeffi.gradle

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

  @get:Input abstract val expectedZigTarget: Property<String>

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

    val descriptorText = descriptor.readText()
    val actualBackend = descriptorText.jsonString("renderBackend")
    val actualZigTarget = descriptorText.jsonString("zigTarget")
    require(actualBackend == expectedBackend.get()) {
      "Native runtime install ${installDirectory.absolutePath} uses backend " +
        "'$actualBackend'; expected '${expectedBackend.get()}'"
    }
    require(actualZigTarget == expectedZigTarget.get()) {
      "Native runtime install ${installDirectory.absolutePath} targets " +
        "'$actualZigTarget'; expected '${expectedZigTarget.get()}'"
    }
  }

  private fun String.jsonString(name: String): String =
    Regex(""""${Regex.escape(name)}"\s*:\s*"([^"]+)"""").find(this)?.groupValues?.get(1)
      ?: error("Native runtime artifact descriptor is missing '$name'")
}
