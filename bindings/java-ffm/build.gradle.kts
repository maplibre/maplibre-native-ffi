import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.testing.Test

plugins {
  `java-library`
  id("de.infolektuell.jextract") version "1.4.0"
}

repositories { mavenCentral() }

val lwjglVersion = "3.4.1"

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
val lwjglTestJvmArgs = listOf("--enable-native-access=ALL-UNNAMED")

jextract.libraries {
  val maplibreNativeC by registering {
    header = rootProject.layout.projectDirectory.file("include/maplibre_native_c.h")
    includes.add(rootProject.layout.projectDirectory.dir("include"))
    headerClassName = "MapLibreNativeC"
    targetPackage = "org.maplibre.nativeffi.internal.c"
    whitelist.argFile = layout.projectDirectory.file("src/jextract/maplibre-native-c.includes")
  }

  sourceSets.named("main") { jextract.libraries.addLater(maplibreNativeC) }
}

dependencies {
  testImplementation(platform("org.junit:junit-bom:6.0.3"))
  testImplementation(platform("org.lwjgl:lwjgl-bom:$lwjglVersion"))
  testImplementation("org.junit.jupiter:junit-jupiter")
  testImplementation("org.lwjgl:lwjgl")
  testImplementation("org.lwjgl:lwjgl-vulkan")
  testRuntimeOnly("org.lwjgl:lwjgl::$lwjglNative")
  testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

tasks.withType<JavaCompile>().configureEach { options.release = 25 }

val nativeLibraryPathProperty = "org.maplibre.nativeffi.library.path"
val nativeBuildDirForTests = providers.environmentVariable("MLN_FFI_BUILD_DIR")
val nativeLibraryPathForTests = nativeBuildDirForTests.map {
  "$it/${System.mapLibraryName("maplibre-native-c")}"
}

tasks.withType<Test>().configureEach {
  useJUnitPlatform()
  jvmArgs(lwjglTestJvmArgs)
  systemProperty(nativeLibraryPathProperty, nativeLibraryPathForTests.get())
  inputs.file(nativeLibraryPathForTests).withPropertyName("maplibreNativeCLibrary")
}
