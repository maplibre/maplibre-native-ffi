import org.gradle.api.tasks.testing.Test
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.tasks.KotlinJvmCompile
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins {
  alias(libs.plugins.kotlin.multiplatform)
  alias(libs.plugins.android.kotlin.multiplatform.library)
}

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

val hostPlatform = HostPlatform.current()
val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val generatedJextractSources = layout.buildDirectory.dir("generated/sources/jextract/jvmMain/java")
val generatedJavaCppSources =
  layout.buildDirectory.dir("generated/sources/javacpp/androidMain/java")
val packagedAndroidNativeLibs = layout.buildDirectory.dir("generated/jniLibs/androidMain")

kotlin {
  when (hostPlatform.kotlinNativeTargetPresetName) {
    "macosArm64" -> macosArm64()
    "macosX64" -> macosX64()
    "linuxArm64" -> linuxArm64()
    "linuxX64" -> linuxX64()
  }

  jvmToolchain(libs.versions.java.toolchain.get().toInt())

  compilerOptions { freeCompilerArgs.add("-Xexpect-actual-classes") }

  jvm { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

  android {
    namespace = "org.maplibre.nativeffi"
    compileSdk = libs.versions.android.compileSdk.get().toInt()
    minSdk = libs.versions.android.minSdk.get().toInt()

    withJava()

    compilerOptions {
      jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.android.release.get()))
    }
  }

  targets.withType<KotlinNativeTarget>().configureEach {
    binaries.all {
      linkerOpts(maplibreNativeC.linkDirs.map { "-L$it" })
      linkerOpts(maplibreNativeC.linkLibraries.map { "-l$it" })
      if (hostPlatform.isMac || hostPlatform.isLinux) {
        linkerOpts(maplibreNativeC.runtimeLibraryDirs.map { "-Wl,-rpath,$it" })
      }
      if (hostPlatform.isMac) {
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
    androidMain.dependencies {
      implementation(libs.javacpp)
      implementation(libs.rustls.platform.verifier)
    }

    commonTest.dependencies { implementation(kotlin("test")) }
  }
}

configurations.register("javaCppTool") {
  isCanBeConsumed = false
  isCanBeResolved = true
}

dependencies.add("javaCppTool", libs.javacpp)

apply(from = "gradle/jextract-jvm.gradle.kts")

apply(from = "gradle/javacpp-android.gradle.kts")

tasks.named<KotlinJvmCompile>("compileKotlinJvm") {
  dependsOn("generateJvmJextractBindings")
  source(generatedJextractSources)
}

androidComponents {
  onVariants { variant ->
    // Android KMP does not currently expose a task-provider-backed generated Java source hook.
    // Keep the explicit task dependencies below in sync with this static source directory.
    variant.sources.java?.addStaticSourceDirectory(
      generatedJavaCppSources.get().asFile.absolutePath
    )
    variant.sources.jniLibs?.addStaticSourceDirectory(
      packagedAndroidNativeLibs.get().asFile.absolutePath
    )
  }
}

tasks.configureEach {
  when (name) {
    "compileAndroidMainJavaWithJavac",
    "compileAndroidMain",
    "extractAndroidMainAnnotations" -> dependsOn("generateAndroidJavaCppBindings")
    "preAndroidMainBuild",
    "mergeAndroidMainJniLibFolders" -> {
      dependsOn("packageAndroidNativeLibraries")
      inputs.file(maplibreNativeC.libraryPath).withPropertyName("maplibreNativeCLibrary")
    }
  }
}

tasks.named<Test>("jvmTest") {
  jvmArgs("--enable-native-access=ALL-UNNAMED")
  systemProperty("org.maplibre.nativeffi.library.path", maplibreNativeC.libraryPath.absolutePath)
  inputs.file(maplibreNativeC.libraryPath).withPropertyName("maplibreNativeCLibrary")
  inputs.file(maplibreNativeC.propertiesFile).withPropertyName("maplibreNativeCProperties")
}

tasks.register("nativeTest") {
  group = "verification"
  description = "Runs tests for the host Kotlin/Native target."
  dependsOn(kotlin.targets.withType<KotlinNativeTarget>().map { "${it.name}Test" })
}

tasks.register("androidBuild") {
  group = "build"
  description = "Builds the Android variant of the Kotlin Multiplatform binding."
  dependsOn("packageAndroidNativeLibraries", "assembleAndroidMain")
}
