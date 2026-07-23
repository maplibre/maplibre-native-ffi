import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.compile.JavaCompile
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins {
  alias(libs.plugins.kotlin.jvm)
  alias(libs.plugins.compose.multiplatform)
  alias(libs.plugins.compose.compiler)
}

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories {
  google()
  mavenCentral()
}

val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()

fun lwjglNativeClassifier(): String {
  val os = System.getProperty("os.name").lowercase()
  val arch = System.getProperty("os.arch").lowercase()
  return when {
    os.contains("mac") && (arch == "aarch64" || arch == "arm64") -> "natives-macos-arm64"
    os.contains("mac") -> "natives-macos"
    os.contains("linux") && (arch == "aarch64" || arch == "arm64") -> "natives-linux-arm64"
    os.contains("linux") -> "natives-linux"
    os.contains("windows") && (arch == "aarch64" || arch == "arm64") -> "natives-windows-arm64"
    os.contains("windows") -> "natives-windows"
    else -> throw GradleException("Unsupported LWJGL native platform: $os/$arch")
  }
}

val composeMapJvmArgs = buildList { add("--enable-native-access=ALL-UNNAMED") }

kotlin { compilerOptions { jvmTarget.set(JvmTarget.JVM_24) } }

compose.desktop {
  application {
    mainClass = "org.maplibre.nativeffi.examples.composemap.Main"
    jvmArgs += composeMapJvmArgs
  }
}

dependencies {
  implementation(project(":bindings:kotlin"))
  implementation(compose.desktop.currentOs)
  implementation(platform(libs.lwjgl.bom))
  implementation(libs.lwjgl)
  implementation(libs.lwjgl.egl)
  implementation(libs.lwjgl.glfw)
  implementation(libs.lwjgl.opengl)
  implementation(libs.lwjgl.opengles)
  implementation(libs.lwjgl.vulkan)
  runtimeOnly(variantOf(libs.lwjgl) { classifier(lwjglNativeClassifier()) })
  runtimeOnly(variantOf(libs.lwjgl.glfw) { classifier(lwjglNativeClassifier()) })
  runtimeOnly(variantOf(libs.lwjgl.opengl) { classifier(lwjglNativeClassifier()) })
  runtimeOnly(variantOf(libs.lwjgl.opengles) { classifier(lwjglNativeClassifier()) })
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
  systemProperty("org.lwjgl.librarypath", nativeHostLibraryPath)
  systemProperty(nativeLibraryPathProperty, nativeLibraryPath.absolutePath)
  systemProperty(nativeLibraryDirsProperty, nativeLoaderLibraryPath)
  inputs.file(nativeLibraryPath).withPropertyName("maplibreNativeCLibrary")
  inputs
    .files(maplibreNativeC.loaderLibraryDirs)
    .withPropertyName("maplibreNativeCLoaderLibraryDirs")
  inputs.dir(maplibreNativeC.installDir).withPropertyName("maplibreNativeCInstallDir")
}
