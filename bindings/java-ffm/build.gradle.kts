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
val hostOs = System.getProperty("os.name").lowercase()
val hostIsMac = hostOs.contains("mac")
val hostIsLinux = hostOs.contains("linux")
val hostIsWindows = hostOs.contains("windows")
val pixiVulkanLoader =
  when {
    hostIsMac ->
      rootProject.layout.projectDirectory.file(".pixi/envs/default/lib/libvulkan.1.dylib")
    hostIsLinux -> rootProject.layout.projectDirectory.file(".pixi/envs/default/lib/libvulkan.so.1")
    hostIsWindows ->
      rootProject.layout.projectDirectory.file(".pixi/envs/default/Library/bin/vulkan-1.dll")
    else -> null
  }
val pixiRuntimeBin =
  when {
    hostIsWindows -> rootProject.layout.projectDirectory.dir(".pixi/envs/default/Library/bin")
    else -> null
  }
val lwjglTestJvmArgs = buildList {
  add("--enable-native-access=ALL-UNNAMED")
  if (pixiVulkanLoader?.asFile?.exists() == true) {
    add("-Dorg.lwjgl.vulkan.libname=${pixiVulkanLoader.asFile.absolutePath}")
  }
}

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
val nativeBuildDirForTests =
  providers
    .environmentVariable("MLN_FFI_BUILD_DIR")
    .orElse(rootProject.layout.buildDirectory.dir("host").map { it.asFile.absolutePath })
val nativeBuildConfigForTests =
  providers.environmentVariable("MLN_FFI_CMAKE_BUILD_CONFIG").orElse("")
val nativeLibraryDirForTests =
  nativeBuildDirForTests.zip(nativeBuildConfigForTests) { buildDir, config ->
    val configDir = "$buildDir/$config"
    if (config.isNotEmpty() && file(configDir).isDirectory) configDir else buildDir
  }
val nativeLibraryPathForTests = nativeLibraryDirForTests.map {
  "$it/${System.mapLibraryName("maplibre-native-c")}"
}

tasks.withType<Test>().configureEach {
  useJUnitPlatform()
  jvmArgs(lwjglTestJvmArgs)
  systemProperty(nativeLibraryPathProperty, nativeLibraryPathForTests.get())
  inputs.file(nativeLibraryPathForTests).withPropertyName("maplibreNativeCLibrary")
  if (pixiRuntimeBin?.asFile?.isDirectory == true) {
    environment(
      "PATH",
      "${pixiRuntimeBin.asFile.absolutePath}${File.pathSeparator}${System.getenv("PATH")}",
    )
  }
}
