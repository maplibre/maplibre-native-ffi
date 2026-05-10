import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.jvm.toolchain.JavaLanguageVersion

allprojects {
  pluginManager.withPlugin("java") {
    extensions.configure<JavaPluginExtension>("java") {
      toolchain { languageVersion = JavaLanguageVersion.of(25) }
    }
  }
}
