package org.maplibre.nativeffi.gradle

import groovy.json.JsonSlurper
import java.io.File
import org.gradle.api.DefaultTask
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.Optional
import org.gradle.api.tasks.TaskAction
import org.gradle.work.DisableCachingByDefault

/** Checks that the local build holds a native install for one preset. */
@DisableCachingByDefault(because = "The task only validates a small native artifact descriptor")
abstract class VerifyLocalBuild : DefaultTask() {
  @get:Input @get:Optional abstract val localBuildPath: Property<String>

  @get:Input abstract val preset: Property<String>

  @get:Input abstract val expectedTargetPlatform: Property<String>

  @get:Input @get:Optional abstract val expectedBackend: Property<String>

  @TaskAction
  fun verify() {
    val preset = preset.get()
    val localBuild =
      localBuildPath.orNull?.let(::File)
        ?: error(
          "Set -P$LOCAL_BUILD_PROPERTY to an install prefix or to a build root holding " +
            "$preset/install",
        )
    val descriptor = localBuildPrefix(localBuild, preset).resolve(NATIVE_DESCRIPTOR_PATH)
    require(descriptor.isFile) {
      "$LOCAL_BUILD_PROPERTY names $localBuild, which is neither an install prefix nor a build " +
        "root holding $preset"
    }

    @Suppress("UNCHECKED_CAST") val metadata = JsonSlurper().parse(descriptor) as Map<String, Any?>
    val builtPlatform = metadata["targetPlatform"]
    val builtBackend = metadata["renderBackend"]
    val platform = expectedTargetPlatform.get()
    val backend = expectedBackend.orNull
    require(builtPlatform == platform && (backend == null || builtBackend == backend)) {
      "$LOCAL_BUILD_PROPERTY names an install built for $builtPlatform $builtBackend, which " +
        "cannot serve $platform${backend?.let { " $it" } ?: ""}"
    }
  }
}
