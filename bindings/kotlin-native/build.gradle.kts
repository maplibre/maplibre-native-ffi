import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins { kotlin("multiplatform") version "2.2.21" }

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories { mavenCentral() }

val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
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

  sourceSets { commonTest.dependencies { implementation(kotlin("test")) } }
}
