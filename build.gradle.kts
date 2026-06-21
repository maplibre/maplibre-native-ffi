import groovy.json.JsonSlurper
import org.gradle.api.artifacts.dsl.RepositoryHandler
import org.gradle.api.artifacts.repositories.MavenArtifactRepository
import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.jvm.toolchain.JavaLanguageVersion

plugins {
  kotlin("jvm") version "2.2.21" apply false
  kotlin("multiplatform") version "2.2.21" apply false
  id("com.android.application") version "9.1.1" apply false
  id("com.android.kotlin.multiplatform.library") version "9.1.1" apply false
}

val rustlsPlatformVerifierMavenDir =
  providers
    .exec {
      workingDir = rootProject.projectDir
      commandLine(
        "cargo",
        "metadata",
        "--format-version",
        "1",
        "--filter-platform",
        "aarch64-linux-android",
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

allprojects {
  pluginManager.withPlugin("com.android.application") { repositories.android() }

  pluginManager.withPlugin("com.android.kotlin.multiplatform.library") { repositories.android() }

  pluginManager.withPlugin("java") {
    extensions.configure<JavaPluginExtension>("java") {
      toolchain { languageVersion = JavaLanguageVersion.of(25) }
    }
  }
}

fun RepositoryHandler.android() {
  google()
  mavenCentral()
  rustlsPlatformVerifier()
}

fun RepositoryHandler.rustlsPlatformVerifier(): MavenArtifactRepository {
  return maven {
    url = uri(rustlsPlatformVerifierMavenDir.get())
    metadataSources.artifact()
  }
}
