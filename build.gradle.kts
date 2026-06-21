import org.gradle.api.artifacts.dsl.RepositoryHandler
import org.gradle.api.artifacts.repositories.MavenArtifactRepository
import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.jvm.toolchain.JavaLanguageVersion
import org.maplibre.nativeffi.gradle.RustlsPlatformVerifier

plugins {
  alias(libs.plugins.kotlin.jvm) apply false
  alias(libs.plugins.kotlin.multiplatform) apply false
  alias(libs.plugins.android.application) apply false
  alias(libs.plugins.android.kotlin.multiplatform.library) apply false
}

val rustlsPlatformVerifierMavenDir = RustlsPlatformVerifier.mavenDir(rootProject)

allprojects {
  pluginManager.withPlugin("com.android.application") { repositories.android() }

  pluginManager.withPlugin("com.android.kotlin.multiplatform.library") { repositories.android() }

  pluginManager.withPlugin("java") {
    extensions.configure<JavaPluginExtension>("java") {
      toolchain {
        languageVersion = JavaLanguageVersion.of(libs.versions.java.toolchain.get().toInt())
      }
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
