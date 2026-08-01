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
val packagedAndroidRuntimeLibs =
  project(":bindings:kotlin").layout.buildDirectory.dir("generated/jniLibs/runtime")

kotlin {
  jvm { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

  linuxX64()

  android {
    namespace = "org.maplibre.nativeffi.runtime.vulkan"
    compileSdk = libs.versions.android.compileSdk.get().toInt()
    minSdk = libs.versions.android.minSdk.get().toInt()

    // The AAR carries the Rustls platform verifier that the native TLS stack
    // calls over JNI. Nothing on a JVM classpath references it, so an app that
    // minifies keeps it only through this rule.
    optimization {
      consumerKeepRules.file(
        rootProject.file("bindings/rustls-platform-verifier-android/consumer-rules.pro")
      )
      consumerKeepRules.publish = true
    }
  }
}

androidComponents {
  onVariants { variant ->
    androidTargets.forEach { target ->
      variant.sources.jniLibs?.addStaticSourceDirectory(
        packagedAndroidRuntimeLibs.get().dir("vulkan/${target.cargoTarget}").asFile.absolutePath
      )
    }
  }
}

configureMaplibreRuntime(
  backend = MaplibreRuntimeBackend.VULKAN,
  targetFamily = MaplibreRuntimeTargetFamily.LINUX,
)
