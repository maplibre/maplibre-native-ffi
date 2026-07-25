import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.api.publish.PublishingExtension
import org.gradle.jvm.toolchain.JavaLanguageVersion

plugins {
  // The typed runtime convention compiles against these plugins in buildSrc, so their versions
  // come from that classpath and the project applies them by ID.
  id("org.jetbrains.kotlin.jvm") apply false
  id("org.jetbrains.kotlin.multiplatform") apply false
  alias(libs.plugins.compose.multiplatform) apply false
  alias(libs.plugins.compose.compiler) apply false
  id("com.android.application") apply false
  id("com.android.kotlin.multiplatform.library") apply false
  id("com.vanniktech.maven.publish") apply false
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
