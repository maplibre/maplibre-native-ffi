import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget

plugins { kotlin("multiplatform") version "2.2.21" }

repositories { mavenCentral() }

val nativeBuildDir = providers.environmentVariable("MLN_FFI_BUILD_DIR").orNull
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
    if (nativeBuildDir != null) {
      binaries.all {
        linkerOpts("-L$nativeBuildDir", "-lmaplibre-native-c")
        if (hostOs.contains("mac") || hostOs.contains("linux")) {
          linkerOpts("-Wl,-rpath,$nativeBuildDir")
        }
      }
    }

    compilations.getByName("main") {
      cinterops {
        val maplibreNativeC by creating {
          defFile(project.file("src/nativeInterop/cinterop/maplibreNativeC.def"))
          includeDirs.headerFilterOnly(rootProject.file("include"))
          compilerOpts("-I${rootProject.file("include").absolutePath}")
        }
      }
    }
  }

  sourceSets { commonTest.dependencies { implementation(kotlin("test")) } }
}
