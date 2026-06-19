import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget

plugins { kotlin("multiplatform") version "2.2.21" }

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories { mavenCentral() }

@Suppress("UNCHECKED_CAST")
val maplibreNativeCList = extra["maplibreNativeCList"] as (String) -> List<String>
val nativeIncludeDirs = maplibreNativeCList("maplibreNativeC.includeDirs")
val nativeLinkDirs = maplibreNativeCList("maplibreNativeC.linkDirs")
val nativeRuntimeLibraryDirs = maplibreNativeCList("maplibreNativeC.runtimeLibraryDirs")
val nativeLinkLibraries = maplibreNativeCList("maplibreNativeC.linkLibraries")
val nativeFrameworks = maplibreNativeCList("maplibreNativeC.frameworks")
val hostOs = System.getProperty("os.name").lowercase()
val hostArch = System.getProperty("os.arch").lowercase()

kotlin {
  when {
    hostOs.contains("mac") && (hostArch == "aarch64" || hostArch == "arm64") -> macosArm64()
    hostOs.contains("mac") -> macosX64()
    hostOs.contains("linux") && (hostArch == "aarch64" || hostArch == "arm64") -> linuxArm64()
    hostOs.contains("linux") -> linuxX64()
  }

  targets.withType<KotlinNativeTarget>().configureEach {
    binaries.all {
      linkerOpts(nativeLinkDirs.map { "-L$it" })
      linkerOpts(nativeLinkLibraries.map { "-l$it" })
      if (hostOs.contains("mac") || hostOs.contains("linux")) {
        linkerOpts(nativeRuntimeLibraryDirs.map { "-Wl,-rpath,$it" })
      }
      if (hostOs.contains("mac")) {
        linkerOpts(nativeFrameworks.flatMap { listOf("-framework", it) })
      }
    }

    compilations.getByName("main") {
      cinterops {
        val maplibreNativeC by creating {
          defFile(project.file("src/nativeInterop/cinterop/maplibreNativeC.def"))
          includeDirs.headerFilterOnly(*nativeIncludeDirs.map { file(it) }.toTypedArray())
          compilerOpts(nativeIncludeDirs.map { "-I$it" })
        }
      }
    }
  }

  sourceSets { commonTest.dependencies { implementation(kotlin("test")) } }
}
