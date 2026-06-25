import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.compile.JavaCompile
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins {
  kotlin("jvm")
  id("org.jetbrains.compose") version "1.11.1"
  id("org.jetbrains.kotlin.plugin.compose") version "2.2.21"
}

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories {
  google()
  mavenCentral()
}

val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val lwjglVersion = "3.4.1"

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
  implementation(platform("org.lwjgl:lwjgl-bom:$lwjglVersion"))
  implementation("org.lwjgl:lwjgl")
  implementation("org.lwjgl:lwjgl-egl")
  implementation("org.lwjgl:lwjgl-glfw")
  implementation("org.lwjgl:lwjgl-opengl")
  implementation("org.lwjgl:lwjgl-opengles")
  implementation("org.lwjgl:lwjgl-vulkan")
  runtimeOnly("org.lwjgl:lwjgl::${lwjglNativeClassifier()}")
  runtimeOnly("org.lwjgl:lwjgl-glfw::${lwjglNativeClassifier()}")
  runtimeOnly("org.lwjgl:lwjgl-opengl::${lwjglNativeClassifier()}")
  runtimeOnly("org.lwjgl:lwjgl-opengles::${lwjglNativeClassifier()}")
}

tasks.withType<JavaCompile>().configureEach { options.release = 24 }

val nativeLibraryPathProperty = "org.maplibre.nativeffi.library.path"
val nativeLibraryPath = maplibreNativeC.libraryPath
val nativeRuntimeLibraryPath =
  maplibreNativeC.runtimeLibraryDirs.joinToString(File.pathSeparator) { it.absolutePath }

tasks.withType<JavaExec>().configureEach {
  jvmArgs(composeMapJvmArgs)
  systemProperty("org.lwjgl.librarypath", nativeRuntimeLibraryPath)
  systemProperty(nativeLibraryPathProperty, nativeLibraryPath.absolutePath)
  inputs.file(nativeLibraryPath).withPropertyName("maplibreNativeCLibrary")
  inputs
    .files(maplibreNativeC.runtimeLibraryDirs)
    .withPropertyName("maplibreNativeCRuntimeLibraryDirs")
  inputs.file(maplibreNativeC.propertiesFile).withPropertyName("maplibreNativeCProperties")
}
