import org.jetbrains.kotlin.gradle.ExperimentalWasmDsl
import org.maplibre.nativeffi.gradle.MaplibreRuntimeBackend
import org.maplibre.nativeffi.gradle.configureMaplibreRuntime

plugins {
  id("org.jetbrains.kotlin.multiplatform")
  id("com.vanniktech.maven.publish")
}

kotlin {
  @OptIn(ExperimentalWasmDsl::class) wasmJs { browser() }
}

configureMaplibreRuntime(MaplibreRuntimeBackend.WEBGPU)
