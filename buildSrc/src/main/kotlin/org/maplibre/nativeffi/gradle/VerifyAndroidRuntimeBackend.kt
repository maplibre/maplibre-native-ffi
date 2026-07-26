package org.maplibre.nativeffi.gradle

import org.gradle.api.DefaultTask
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.TaskAction

abstract class VerifyAndroidRuntimeBackend : DefaultTask() {
  @get:Input abstract val selectedBackend: Property<String>

  @get:Input abstract val expectedBackend: Property<String>

  @TaskAction
  fun verify() {
    val selected = AndroidTarget.parseBackend(selectedBackend.get())
    val expected = expectedBackend.get()
    require(selected == expected) {
      "Publishing the $expected Android runtime requires -Pmaplibre.android.backend=$expected"
    }
  }
}
