import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.api.publish.PublishingExtension
import org.gradle.jvm.toolchain.JavaLanguageVersion

plugins {
  alias(libs.plugins.kotlin.jvm) apply false
  alias(libs.plugins.kotlin.multiplatform) apply false
  alias(libs.plugins.android.application) apply false
  alias(libs.plugins.android.kotlin.multiplatform.library) apply false
  alias(libs.plugins.maven.publish) apply false
}

allprojects {
  repositories.mavenCentral()

  pluginManager.withPlugin("com.android.application") { repositories.android() }

  pluginManager.withPlugin("com.android.kotlin.multiplatform.library") { repositories.android() }

  pluginManager.withPlugin("maven-publish") {
    rootProject.providers.gradleProperty("maplibre.maven.localRepository").orNull?.let {
      repositoryPath ->
      extensions.configure<PublishingExtension>("publishing") {
        repositories.maven {
          name = "snapshotStaging"
          url = rootProject.uri(repositoryPath)
        }
      }
    }
  }

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
}
