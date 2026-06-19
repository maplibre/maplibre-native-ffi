import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.compile.JavaCompile
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins { application }

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories { mavenCentral() }

val lwjglVersion = "3.4.1"
val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()

fun lwjglNativeClassifier(): String {
  val os = System.getProperty("os.name").lowercase()
  val arch = System.getProperty("os.arch").lowercase()
  return when {
    os.contains("mac") && (arch == "aarch64" || arch == "arm64") -> "natives-macos-arm64"
    os.contains("mac") -> "natives-macos"
    os.contains("linux") && (arch == "aarch64" || arch == "arm64") -> "natives-linux-arm64"
    os.contains("linux") -> "natives-linux"
    os.contains("windows") -> "natives-windows"
    else -> throw GradleException("Unsupported LWJGL native platform: $os/$arch")
  }
}

val lwjglNative = lwjglNativeClassifier()
val hostOs = System.getProperty("os.name").lowercase()
val hostIsMac = hostOs.contains("mac")
val lwjglMapJvmArgs = buildList {
  add("--enable-native-access=ALL-UNNAMED")
  if (hostIsMac) {
    add("-XstartOnFirstThread")
  }
}

application {
  mainClass = "org.maplibre.nativeffi.examples.lwjglmap.Main"
  applicationDefaultJvmArgs = lwjglMapJvmArgs
}

dependencies {
  implementation(project(":bindings:java-ffm"))
  implementation(platform("org.lwjgl:lwjgl-bom:$lwjglVersion"))
  implementation("org.lwjgl:lwjgl")
  implementation("org.lwjgl:lwjgl-egl")
  implementation("org.lwjgl:lwjgl-glfw")
  implementation("org.lwjgl:lwjgl-opengl")
  implementation("org.lwjgl:lwjgl-opengles")
  implementation("org.lwjgl:lwjgl-vulkan")
  implementation("org.lwjgl:lwjgl-shaderc")
  runtimeOnly("org.lwjgl:lwjgl::$lwjglNative")
  runtimeOnly("org.lwjgl:lwjgl-glfw::$lwjglNative")
  runtimeOnly("org.lwjgl:lwjgl-opengl::$lwjglNative")
  runtimeOnly("org.lwjgl:lwjgl-opengles::$lwjglNative")
  runtimeOnly("org.lwjgl:lwjgl-shaderc::$lwjglNative")
}

tasks.withType<JavaCompile>().configureEach { options.release = 25 }

val nativeLibraryPathProperty = "org.maplibre.nativeffi.library.path"
val nativeLibraryPath = maplibreNativeC.libraryPath

tasks.withType<JavaExec>().configureEach {
  jvmArgs(lwjglMapJvmArgs)
  systemProperty(nativeLibraryPathProperty, nativeLibraryPath.absolutePath)
  inputs.file(nativeLibraryPath).withPropertyName("maplibreNativeCLibrary")
  inputs.file(maplibreNativeC.propertiesFile).withPropertyName("maplibreNativeCProperties")
}
