import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.maplibre.nativeffi.gradle.MaplibreRuntimeBackend
import org.maplibre.nativeffi.gradle.configureMaplibreRuntime

plugins {
  id("org.jetbrains.kotlin.multiplatform")
  id("com.vanniktech.maven.publish")
}

kotlin {
  jvm { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

  iosArm64()
  iosSimulatorArm64()
  macosArm64()
}

configureMaplibreRuntime(MaplibreRuntimeBackend.METAL)
