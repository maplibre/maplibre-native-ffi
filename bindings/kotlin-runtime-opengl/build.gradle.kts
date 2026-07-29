import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.MaplibreRuntimeBackend
import org.maplibre.nativeffi.gradle.MaplibreRuntimeTargetFamily
import org.maplibre.nativeffi.gradle.configureMaplibreRuntime

plugins {
  id("org.jetbrains.kotlin.multiplatform")
  id("com.android.kotlin.multiplatform.library")
  id("com.vanniktech.maven.publish")
}

val androidTargets =
  AndroidTarget.parseAbis(
    providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS)
  )
val packagedAndroidNativeLibs =
  project(":bindings:kotlin").layout.buildDirectory.dir("generated/jniLibs/androidMain")

kotlin {
  jvm { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

  linuxX64()

  android {
    namespace = "org.maplibre.nativeffi.runtime.opengl"
    compileSdk = libs.versions.android.compileSdk.get().toInt()
    minSdk = libs.versions.android.minSdk.get().toInt()
  }
}

androidComponents {
  onVariants { variant ->
    androidTargets.forEach { target ->
      variant.sources.jniLibs?.addStaticSourceDirectory(
        packagedAndroidNativeLibs.get().dir("opengl/${target.cargoTarget}").asFile.absolutePath
      )
    }
  }
}

configureMaplibreRuntime(
  backend = MaplibreRuntimeBackend.OPENGL,
  targetFamily = MaplibreRuntimeTargetFamily.LINUX,
)
