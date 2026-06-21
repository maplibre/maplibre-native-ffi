import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.compile.JavaCompile
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins {
  application
  alias(libs.plugins.kotlin.jvm)
}

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories { mavenCentral() }

val hostPlatform = HostPlatform.current()
val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val lwjglNative = hostPlatform.lwjglNativeClassifier
val lwjglMapJvmArgs = buildList {
  add("--enable-native-access=ALL-UNNAMED")
  if (hostPlatform.isMac) {
    add("-XstartOnFirstThread")
  }
}

kotlin { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

application {
  mainClass = "org.maplibre.nativeffi.examples.lwjglmap.Main"
  applicationDefaultJvmArgs = lwjglMapJvmArgs
}

dependencies {
  implementation(project(":bindings:kotlin"))
  implementation(platform(libs.lwjgl.bom))
  implementation(libs.lwjgl)
  implementation(libs.lwjgl.egl)
  implementation(libs.lwjgl.glfw)
  implementation(libs.lwjgl.opengl)
  implementation(libs.lwjgl.opengles)
  implementation(libs.lwjgl.vulkan)
  implementation(libs.lwjgl.shaderc)
  runtimeOnly(variantOf(libs.lwjgl) { classifier(lwjglNative) })
  runtimeOnly(variantOf(libs.lwjgl.glfw) { classifier(lwjglNative) })
  runtimeOnly(variantOf(libs.lwjgl.opengl) { classifier(lwjglNative) })
  runtimeOnly(variantOf(libs.lwjgl.opengles) { classifier(lwjglNative) })
  runtimeOnly(variantOf(libs.lwjgl.shaderc) { classifier(lwjglNative) })
}

tasks.withType<JavaCompile>().configureEach {
  options.release = libs.versions.java.release.get().toInt()
}

val nativeLibraryPathProperty = "org.maplibre.nativeffi.library.path"
val nativeLibraryPath = maplibreNativeC.libraryPath
val nativeRuntimeLibraryPath =
  maplibreNativeC.runtimeLibraryDirs.joinToString(File.pathSeparator) { it.absolutePath }

tasks.withType<JavaExec>().configureEach {
  jvmArgs(lwjglMapJvmArgs)
  systemProperty("org.lwjgl.librarypath", nativeRuntimeLibraryPath)
  systemProperty(nativeLibraryPathProperty, nativeLibraryPath.absolutePath)
  inputs.file(nativeLibraryPath).withPropertyName("maplibreNativeCLibrary")
  inputs
    .files(maplibreNativeC.runtimeLibraryDirs)
    .withPropertyName("maplibreNativeCRuntimeLibraryDirs")
  inputs.file(maplibreNativeC.propertiesFile).withPropertyName("maplibreNativeCProperties")
}
