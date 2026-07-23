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

    @Suppress("UNCHECKED_CAST") val metadata = JsonSlurper().parse(descriptor) as Map<String, Any?>
    val actualBackend = metadata["renderBackend"]
    val actualZigTarget = metadata["zigTarget"]
    require(actualBackend == expectedBackend.get()) {
      "Native runtime install ${installDirectory.absolutePath} uses backend " +
        "'$actualBackend'; expected '${expectedBackend.get()}'"
    }
    require(actualZigTarget == expectedZigTarget.get()) {
      "Native runtime install ${installDirectory.absolutePath} targets " +
        "'$actualZigTarget'; expected '${expectedZigTarget.get()}'"
    }
  }
}
