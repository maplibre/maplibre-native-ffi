import javax.inject.Inject
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.InputFile
import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.TaskAction
import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.process.ExecOperations
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

abstract class CompileMacMetalBridgeTask
@Inject
constructor(private val execOperations: ExecOperations) : DefaultTask() {
  @get:InputFile abstract val sourceFile: RegularFileProperty

  @get:OutputFile abstract val outputLibrary: RegularFileProperty

  @get:Input abstract val javaHome: Property<String>

  @TaskAction
  fun compile() {
    val output = outputLibrary.get().asFile
    output.parentFile.mkdirs()
    execOperations.exec {
      commandLine(
        "xcrun",
        "clang++",
        "-std=c++17",
        "-dynamiclib",
        "-fobjc-arc",
        "-framework",
        "Foundation",
        "-framework",
        "Metal",
        "-I${javaHome.get()}/include",
        "-I${javaHome.get()}/include/darwin",
        sourceFile.get().asFile.absolutePath,
        "-o",
        output.absolutePath,
      )
    }
  }
}

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
val hostOs = System.getProperty("os.name").lowercase()
val hostIsMac = hostOs.contains("mac")
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
}

tasks.withType<JavaCompile>().configureEach { options.release = 24 }

val nativeLibraryPathProperty = "org.maplibre.nativeffi.library.path"
val nativeLibraryPath = maplibreNativeC.libraryPath
val composeMapBridgePathProperty = "org.maplibre.nativeffi.composemap.bridge.path"
val composeMapBridgeLibrary =
  layout.buildDirectory.file("native/composemap-bridge/libcomposemap_bridge.dylib")

val compileComposeMapMetalBridgeMacos =
  tasks.register<CompileMacMetalBridgeTask>("compileComposeMapMetalBridgeMacos") {
    enabled = hostIsMac
    sourceFile.set(layout.projectDirectory.file("src/main/native/macos/ComposeMapMetalBridge.mm"))
    outputLibrary.set(composeMapBridgeLibrary)
    javaHome.set(System.getProperty("java.home"))
  }

tasks.withType<JavaExec>().configureEach {
  jvmArgs(composeMapJvmArgs)
  systemProperty(nativeLibraryPathProperty, nativeLibraryPath.absolutePath)
  if (hostIsMac) {
    dependsOn(compileComposeMapMetalBridgeMacos)
    systemProperty(composeMapBridgePathProperty, composeMapBridgeLibrary.get().asFile.absolutePath)
    inputs.file(composeMapBridgeLibrary).withPropertyName("composeMapBridgeLibrary")
  }
  inputs.file(nativeLibraryPath).withPropertyName("maplibreNativeCLibrary")
  inputs.file(maplibreNativeC.propertiesFile).withPropertyName("maplibreNativeCProperties")
}
