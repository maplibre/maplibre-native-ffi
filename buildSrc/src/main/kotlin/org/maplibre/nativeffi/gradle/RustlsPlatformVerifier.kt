package org.maplibre.nativeffi.gradle

import groovy.json.JsonSlurper
import java.io.File
import org.gradle.api.Project
import org.gradle.api.provider.Provider

object RustlsPlatformVerifier {
  private val ignoredEnvironmentVariables =
    setOf(
      "MISE_ENV",
      "CARGO_BUILD_TARGET",
      "MLN_FFI_RENDER_BACKEND",
      "MLN_FFI_NATIVE_INSTALL_DIR",
      "MLN_FFI_HOST_LIBRARY_DIRS",
      "ANDROID_NDK_HOME",
    )

  fun mavenDir(project: Project): Provider<File> =
    project.providers
      .exec {
        workingDir = project.rootProject.projectDir
        environment.keys.removeAll(ignoredEnvironmentVariables)
        commandLine(
          "cargo",
          "metadata",
          "--locked",
          "--format-version",
          "1",
          "--manifest-path",
          "src/platform/rust/Cargo.toml",
        )
      }
      .standardOutput
      .asText
      .map { metadata ->
        @Suppress("UNCHECKED_CAST")
        val packages =
          (JsonSlurper().parseText(metadata) as Map<String, Any>)["packages"]
            as List<Map<String, Any>>
        val manifestPath =
          packages.first { it["name"] == "rustls-platform-verifier-android" }["manifest_path"]
            as String
        File(File(manifestPath).parentFile, "maven")
      }
}
