import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.compile.JavaCompile
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins {
  id("org.jetbrains.kotlin.jvm")
  alias(libs.plugins.compose.multiplatform)
  alias(libs.plugins.compose.compiler)
}

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories {
  providers.gradleProperty("maplibre.maven.localRepository").orNull?.let {
    maven { url = rootProject.uri(it) }
  }
  google()
  mavenCentral()
}

val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val hostPlatform = HostPlatform.current()
val usePublishedKotlin =
  providers.gradleProperty("maplibre.usePublishedKotlin").map(String::toBoolean).getOrElse(false)
val maplibrePublicationVersion = providers.gradleProperty("maplibre.maven.version").get()
val maplibreRuntimeBackend =
  providers.gradleProperty("maplibre.runtime.backend").getOrElse("vulkan")

val composeMapJvmArgs = listOf("--enable-native-access=ALL-UNNAMED")

kotlin { compilerOptions { jvmTarget.set(JvmTarget.JVM_24) } }

compose.desktop {
  application {
    mainClass = "org.maplibre.nativeffi.examples.composemap.Main"
    jvmArgs += composeMapJvmArgs
  }
}

dependencies {
  if (usePublishedKotlin) {
    implementation("org.maplibre.nativeffi:maplibre-native-ffi-jvm:$maplibrePublicationVersion")
    runtimeOnly(
      "org.maplibre.nativeffi:maplibre-native-ffi-runtime-$maplibreRuntimeBackend-jvm:" +
        "$maplibrePublicationVersion:${hostPlatform.maplibreNativeClassifier}"
    )
  } else {
    implementation(project(":bindings:kotlin"))
  }
  implementation(compose.desktop.currentOs)
  implementation(platform(libs.lwjgl.bom))
  implementation(libs.lwjgl)
  implementation(libs.lwjgl.egl)
  implementation(libs.lwjgl.glfw)
  implementation(libs.lwjgl.opengl)
  implementation(libs.lwjgl.opengles)
  implementation(libs.lwjgl.vulkan)
  runtimeOnly(variantOf(libs.lwjgl) { classifier(hostPlatform.lwjglNativeClassifier) })
  runtimeOnly(variantOf(libs.lwjgl.glfw) { classifier(hostPlatform.lwjglNativeClassifier) })
  runtimeOnly(variantOf(libs.lwjgl.opengl) { classifier(hostPlatform.lwjglNativeClassifier) })
  runtimeOnly(variantOf(libs.lwjgl.opengles) { classifier(hostPlatform.lwjglNativeClassifier) })
}

tasks.withType<JavaCompile>().configureEach { options.release = 24 }

val nativeLibraryPathProperty = "org.maplibre.nativeffi.library.path"
val nativeLibraryDirsProperty = "org.maplibre.nativeffi.library.dirs"
val nativeLibraryPath = maplibreNativeC.libraryPath
val nativeHostLibraryPath =
  maplibreNativeC.hostLibraryDirs.joinToString(File.pathSeparator) { it.absolutePath }
val nativeLoaderLibraryPath =
  maplibreNativeC.loaderLibraryDirs.joinToString(File.pathSeparator) { it.absolutePath }

tasks.withType<JavaExec>().configureEach {
  jvmArgs(composeMapJvmArgs)
  if (!usePublishedKotlin) {
    systemProperty("org.lwjgl.librarypath", nativeHostLibraryPath)
    systemProperty(nativeLibraryPathProperty, nativeLibraryPath.absolutePath)
    systemProperty(nativeLibraryDirsProperty, nativeLoaderLibraryPath)
    inputs.file(nativeLibraryPath).withPropertyName("maplibreNativeCLibrary")
    inputs
      .files(maplibreNativeC.loaderLibraryDirs)
      .withPropertyName("maplibreNativeCLoaderLibraryDirs")
    inputs.dir(maplibreNativeC.installDir).withPropertyName("maplibreNativeCInstallDir")
  }
}
