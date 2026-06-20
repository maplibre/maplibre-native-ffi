import java.net.URI
import java.security.MessageDigest
import org.gradle.api.DefaultTask
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.Sync
import org.gradle.api.tasks.TaskAction
import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.testing.Test
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.tasks.KotlinJvmCompile
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

abstract class DownloadJextractTask : DefaultTask() {
  @get:Input abstract val url: Property<String>
  @get:Input abstract val expectedSha256: Property<String>
  @get:OutputFile abstract val archive: RegularFileProperty

  @TaskAction
  fun download() {
    val archiveFile = archive.get().asFile
    archiveFile.parentFile.mkdirs()
    if (!archiveFile.isFile) {
      URI(url.get()).toURL().openStream().use { input ->
        archiveFile.outputStream().use { output -> input.copyTo(output) }
      }
    }
    val actualSha256 = sha256(archiveFile)
    check(actualSha256 == expectedSha256.get()) {
      "Invalid jextract archive checksum for $archiveFile: expected ${expectedSha256.get()}, got $actualSha256"
    }
  }

  private fun sha256(file: File): String {
    val digest = MessageDigest.getInstance("SHA-256")
    file.inputStream().use { input ->
      val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
      while (true) {
        val byteCount = input.read(buffer)
        if (byteCount < 0) break
        digest.update(buffer, 0, byteCount)
      }
    }
    return digest.digest().joinToString("") { "%02x".format(it) }
  }
}

plugins {
  kotlin("multiplatform")
  id("com.android.kotlin.multiplatform.library")
}

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories {
  google()
  mavenCentral()
}

val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val javaCppVersion = "1.5.11"
val javaCppToolClasspath =
  configurations.detachedConfiguration(dependencies.create("org.bytedeco:javacpp:$javaCppVersion"))
val generatedJavaCppSources =
  layout.buildDirectory.dir("generated/sources/javacpp/androidMain/java")
val generatedJextractSources = layout.buildDirectory.dir("generated/sources/jextract/jvmMain/java")
val javaCppConfigClasses = layout.buildDirectory.dir("classes/javacppConfig")
val hostOs = System.getProperty("os.name").lowercase()
val hostArch = System.getProperty("os.arch").lowercase()
val jextractArchive = layout.buildDirectory.file("jextract/openjdk-25-jextract.tar.gz")
val jextractInstallDir = layout.buildDirectory.dir("jextract/tool")
val jextractExecutable = jextractInstallDir.map { dir ->
  dir
    .file("jextract-25/bin/${if (hostOs.contains("windows")) "jextract.bat" else "jextract"}")
    .asFile
}

kotlin {
  jvmToolchain(25)

  compilerOptions { freeCompilerArgs.add("-Xexpect-actual-classes") }

  jvm { compilerOptions { jvmTarget.set(JvmTarget.JVM_24) } }

  android {
    namespace = "org.maplibre.nativeffi"
    compileSdk = 36
    minSdk = 24

    withJava()

    compilerOptions { jvmTarget.set(JvmTarget.JVM_17) }
  }

  when {
    hostOs.contains("mac") && (hostArch == "aarch64" || hostArch == "arm64") -> macosArm64()
    hostOs.contains("mac") -> macosX64()
    hostOs.contains("linux") && (hostArch == "aarch64" || hostArch == "arm64") -> linuxArm64()
    hostOs.contains("linux") -> linuxX64()
  }

  targets.withType<KotlinNativeTarget>().configureEach {
    binaries.all {
      linkerOpts(maplibreNativeC.linkDirs.map { "-L$it" })
      linkerOpts(maplibreNativeC.linkLibraries.map { "-l$it" })
      if (hostOs.contains("mac") || hostOs.contains("linux")) {
        linkerOpts(maplibreNativeC.runtimeLibraryDirs.map { "-Wl,-rpath,$it" })
      }
      if (hostOs.contains("mac")) {
        linkerOpts(maplibreNativeC.frameworks.flatMap { listOf("-framework", it) })
      }
    }

    compilations.getByName("main") {
      cinterops {
        create("maplibreNativeC") {
          defFile(project.file("src/nativeInterop/cinterop/maplibreNativeC.def"))
          includeDirs.headerFilterOnly(*maplibreNativeC.includeDirs.toTypedArray())
          compilerOpts(maplibreNativeC.includeDirs.map { "-I$it" })
        }
      }
    }
  }

  sourceSets {
    androidMain.dependencies { implementation("org.bytedeco:javacpp:$javaCppVersion") }

    commonTest.dependencies { implementation(kotlin("test")) }
  }
}

androidComponents {
  onVariants { variant ->
    // Android KMP does not currently expose a task-provider-backed generated Java source hook.
    // Keep the explicit task dependencies below in sync with this static source directory.
    variant.sources.java?.addStaticSourceDirectory(
      generatedJavaCppSources.get().asFile.absolutePath
    )
  }
}

data class JextractDistribution(val url: String, val sha256: String)

// The de.infolektuell.jextract Gradle plugin applies Gradle's Java plugin, which is incompatible
// with Kotlin Multiplatform projects.
val jextractDistribution =
  when {
    hostOs.contains("mac") && (hostArch == "aarch64" || hostArch == "arm64") ->
      JextractDistribution(
        "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_macos-aarch64_bin.tar.gz",
        "6783d2ba7f686ee636b9542525ee06b7bd096dfca294538613b877a4b5a057da",
      )
    hostOs.contains("mac") ->
      JextractDistribution(
        "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_macos-x64_bin.tar.gz",
        "62fd0453349b8eb48f083d2fb9c5f2ab255f894eaa8c658221366f363c7e91b9",
      )
    hostOs.contains("linux") && (hostArch == "aarch64" || hostArch == "arm64") ->
      JextractDistribution(
        "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_linux-aarch64_bin.tar.gz",
        "75a199a05e5edade798600a175f8897e711330338f7d8d2da5fff18d707d665e",
      )
    hostOs.contains("linux") ->
      JextractDistribution(
        "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_linux-x64_bin.tar.gz",
        "d826d366b5db8edbed9cfef3779e45e43ba496ca2166b8f70cdaf81ee90c0b1e",
      )
    hostOs.contains("windows") ->
      JextractDistribution(
        "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_windows-x64_bin.tar.gz",
        "22a853168512d5909c4dfccb1b9e8b2bffb1187b0cf98458aef989d41de995ac",
      )
    else -> throw GradleException("Unsupported jextract platform: $hostOs/$hostArch")
  }

val downloadJextract =
  tasks.register<DownloadJextractTask>("downloadJextract") {
    url = jextractDistribution.url
    expectedSha256 = jextractDistribution.sha256
    archive = jextractArchive
  }

val extractJextract =
  tasks.register<Sync>("extractJextract") {
    val installDir = jextractInstallDir.get().asFile
    dependsOn(downloadJextract)
    from(tarTree(jextractArchive))
    into(jextractInstallDir)
    doFirst { installDir.deleteRecursively() }
  }

val generateJvmJextractBindings =
  tasks.register<Exec>("generateJvmJextractBindings") {
    group = "build"
    description = "Generates JVM FFM declarations for the MapLibre Native C ABI with jextract."
    dependsOn(extractJextract)
    inputs.file("src/jextract/maplibre-native-c.includes")
    inputs.files(maplibreNativeC.includeDirs).withPropertyName("maplibreNativeCIncludeDirs")
    outputs.dir(generatedJextractSources)
    executable = jextractExecutable.get().absolutePath
    args(
      "--output",
      generatedJextractSources.get().asFile.absolutePath,
      "--target-package",
      "org.maplibre.nativeffi.internal.c",
      "--header-class-name",
      "MapLibreNativeC",
      "@${layout.projectDirectory.file("src/jextract/maplibre-native-c.includes").asFile.absolutePath}",
      *maplibreNativeC.includeDirs.flatMap { listOf("-I", it.absolutePath) }.toTypedArray(),
      rootProject.layout.projectDirectory.file("include/maplibre_native_c.h").asFile.absolutePath,
    )
  }

tasks.named<JavaCompile>("compileJvmMainJava") {
  dependsOn(generateJvmJextractBindings)
  source(generatedJextractSources)
  options.release = 24
}

tasks.named<KotlinJvmCompile>("compileKotlinJvm") {
  dependsOn(generateJvmJextractBindings)
  source(generatedJextractSources)
}

val compileJavaCppConfig =
  tasks.register<JavaCompile>("compileAndroidJavaCppConfig") {
    source(
      "src/androidMain/java/org/maplibre/nativeffi/internal/javacpp/MaplibreNativeCConfig.java",
      "src/androidMain/java/org/maplibre/nativeffi/internal/javacpp/AndroidNativeBridge.java",
    )
    classpath = javaCppToolClasspath
    destinationDirectory = javaCppConfigClasses
    options.release = 17
  }

val generateJavaCppBindings =
  tasks.register<JavaExec>("generateAndroidJavaCppBindings") {
    group = "build"
    description = "Generates JavaCPP declarations for the Android MapLibre Native C ABI."
    dependsOn(compileJavaCppConfig)
    classpath = files(javaCppConfigClasses) + javaCppToolClasspath
    mainClass = "org.bytedeco.javacpp.tools.Builder"
    args(
      "-classpath",
      classpath.asPath,
      "-Dplatform.includepath=${maplibreNativeC.includeDirs.joinToString(File.pathSeparator)}",
      "-d",
      generatedJavaCppSources.get().asFile.absolutePath,
      "-nogenerate",
      "org.maplibre.nativeffi.internal.javacpp.MaplibreNativeCConfig",
      "org.maplibre.nativeffi.internal.javacpp.AndroidNativeBridge",
    )
    inputs.file(
      "src/androidMain/java/org/maplibre/nativeffi/internal/javacpp/MaplibreNativeCConfig.java"
    )
    inputs.file(
      "src/androidMain/java/org/maplibre/nativeffi/internal/javacpp/AndroidNativeBridge.java"
    )
    inputs.files(maplibreNativeC.includeDirs).withPropertyName("maplibreNativeCIncludeDirs")
    outputs.file(
      generatedJavaCppSources.map {
        it.file("org/maplibre/nativeffi/internal/javacpp/MaplibreNativeC.java")
      }
    )
  }

tasks
  .matching { it.name == "compileAndroidMainJavaWithJavac" }
  .configureEach { dependsOn(generateJavaCppBindings) }

tasks
  .matching { it.name == "compileAndroidMain" || it.name == "extractAndroidMainAnnotations" }
  .configureEach { dependsOn(generateJavaCppBindings) }

tasks.withType<Test>().configureEach {
  if (name == "jvmTest") {
    jvmArgs("--enable-native-access=ALL-UNNAMED")
    systemProperty("org.maplibre.nativeffi.library.path", maplibreNativeC.libraryPath.absolutePath)
    inputs.file(maplibreNativeC.libraryPath).withPropertyName("maplibreNativeCLibrary")
    inputs.file(maplibreNativeC.propertiesFile).withPropertyName("maplibreNativeCProperties")
  }
}

tasks.register("nativeTest") {
  group = "verification"
  description = "Runs tests for the host Kotlin/Native target."
  dependsOn(kotlin.targets.withType<KotlinNativeTarget>().map { "${it.name}Test" })
}

tasks.register("androidBuild") {
  group = "build"
  description = "Builds the Android variant of the Kotlin Multiplatform binding."
  dependsOn("assembleAndroidMain")
}
