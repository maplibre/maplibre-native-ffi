import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.jvm.toolchain.JavaLanguageVersion

plugins {
  kotlin("jvm") version "2.2.21" apply false
  kotlin("multiplatform") version "2.2.21" apply false
  id("com.android.kotlin.multiplatform.library") version "9.1.1" apply false
}

allprojects {
  pluginManager.withPlugin("java") {
    extensions.configure<JavaPluginExtension>("java") {
      toolchain { languageVersion = JavaLanguageVersion.of(25) }
    }
  }
}
