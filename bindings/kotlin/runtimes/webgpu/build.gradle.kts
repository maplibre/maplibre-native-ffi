import org.jetbrains.kotlin.gradle.ExperimentalWasmDsl
import org.maplibre.nativeffi.gradle.MaplibreRuntime
import org.maplibre.nativeffi.gradle.configureMaplibreRuntime

plugins {
  id("org.jetbrains.kotlin.multiplatform")
  id("com.vanniktech.maven.publish")
}

kotlin {
  @OptIn(ExperimentalWasmDsl::class) wasmJs { browser() }
}

configureMaplibreRuntime(MaplibreRuntime.WEBGPU)
