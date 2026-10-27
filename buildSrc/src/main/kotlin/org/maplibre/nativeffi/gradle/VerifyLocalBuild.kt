package org.maplibre.nativeffi.gradle

import org.gradle.api.DefaultTask
import org.gradle.api.GradleException
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.Optional
import org.gradle.api.tasks.TaskAction
import org.gradle.work.DisableCachingByDefault

/** Reports why the local build cannot serve one target, as [nativeInstall] found it. */
@DisableCachingByDefault(because = "The task only reports a lookup made at configuration time")
abstract class VerifyLocalBuild : DefaultTask() {
  @get:Input @get:Optional abstract val problem: Property<String>

  @TaskAction
  fun verify() {
    problem.orNull?.let { throw GradleException(it) }
  }
}
