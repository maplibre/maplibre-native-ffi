import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.api.publish.PublishingExtension
import org.gradle.jvm.toolchain.JavaLanguageVersion

plugins {
  // The typed runtime convention compiles against these plugins in buildSrc, so their versions
  // come from that classpath and the project applies them by ID.
  id("org.jetbrains.kotlin.multiplatform") apply false
  id("com.android.kotlin.multiplatform.library") apply false
  id("com.vanniktech.maven.publish") apply false
  alias(libs.plugins.dokka) apply false
}

allprojects {
  repositories.mavenCentral()

  pluginManager.withPlugin("com.android.kotlin.multiplatform.library") {
    repositories.google()
    repositories.rustlsPlatformVerifier()
  }
  pluginManager.withPlugin("com.android.application") {
    repositories.google()
    repositories.rustlsPlatformVerifier()
  }

  pluginManager.withPlugin("maven-publish") {
    rootProject.providers.gradleProperty("maplibre.maven.localRepository").orNull?.let {
      repositoryPath ->
      extensions.configure<PublishingExtension>("publishing") {
        repositories.maven {
          name = "mavenStaging"
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

/**
 * The Maven repository on GitHub that rustls-platform-verifier publishes its Android component to,
 * for that component alone.
 */
fun RepositoryHandler.rustlsPlatformVerifier() {
  maven {
    url =
      uri(
        "https://github.com/rustls/rustls-platform-verifier/raw/maven-archive/android-release-support/maven/",
      )
    content { includeGroup("org.rustls") }
  }
}
