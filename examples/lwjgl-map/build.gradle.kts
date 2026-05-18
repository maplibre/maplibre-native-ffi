import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.testing.Test

plugins { application }

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
val lwjglMapJvmArgs = buildList {
  add("--enable-native-access=ALL-UNNAMED")
  if (hostIsMac) {
    add("-XstartOnFirstThread")
  }
  if (pixiVulkanLoader?.asFile?.exists() == true) {
    add("-Dorg.lwjgl.vulkan.libname=${pixiVulkanLoader.asFile.absolutePath}")
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
  implementation("org.lwjgl:lwjgl-glfw")
  implementation("org.lwjgl:lwjgl-vulkan")
  implementation("org.lwjgl:lwjgl-shaderc")
  runtimeOnly("org.lwjgl:lwjgl::$lwjglNative")
  runtimeOnly("org.lwjgl:lwjgl-glfw::$lwjglNative")
  runtimeOnly("org.lwjgl:lwjgl-shaderc::$lwjglNative")

  testImplementation(platform("org.junit:junit-bom:6.0.3"))
  testImplementation("org.junit.jupiter:junit-jupiter")
  testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

tasks.withType<JavaCompile>().configureEach { options.release = 25 }

tasks.withType<Test>().configureEach { useJUnitPlatform() }

val nativeLibraryPathProperty = "org.maplibre.nativeffi.library.path"
val nativeBuildDir =
  providers
    .environmentVariable("MLN_FFI_BUILD_DIR")
    .orElse(rootProject.layout.buildDirectory.dir("host").map { it.asFile.absolutePath })
val nativeBuildConfig = providers.environmentVariable("MLN_FFI_CMAKE_BUILD_CONFIG").orElse("")
val nativeLibraryDir =
  nativeBuildDir.zip(nativeBuildConfig) { buildDir, config ->
    val configDir = "$buildDir/$config"
    if (config.isNotEmpty() && file(configDir).isDirectory) configDir else buildDir
  }
val nativeLibraryPath = nativeLibraryDir.map { "$it/${System.mapLibraryName("maplibre-native-c")}" }

tasks.withType<JavaExec>().configureEach {
  jvmArgs(lwjglMapJvmArgs)
  systemProperty(nativeLibraryPathProperty, nativeLibraryPath.get())
  inputs.file(nativeLibraryPath).withPropertyName("maplibreNativeCLibrary").optional()
  if (pixiRuntimeBin?.asFile?.isDirectory == true) {
    environment(
      "PATH",
      "${pixiRuntimeBin.asFile.absolutePath}${File.pathSeparator}${System.getenv("PATH")}",
    )
  }
}
