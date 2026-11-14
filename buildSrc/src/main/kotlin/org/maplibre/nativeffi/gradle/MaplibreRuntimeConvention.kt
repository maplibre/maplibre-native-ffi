package org.maplibre.nativeffi.gradle

import org.gradle.api.Project

/**
 * A runtime artifact of the Kotlin binding: [NATIVE] carries the C library of every native
 * platform, each with all of the platform's render backends, and [WEBGL] and [WEBGPU] each carry
 * the browser package of one backend, [webBackend], since a page downloads one.
 */
enum class MaplibreRuntime(
  val id: String,
  val artifact: String,
  val displayName: String,
  val webBackend: String?,
) {
  NATIVE("native", "maplibre-native-ffi-runtime", "native", null),
  WEBGL("webgl", "maplibre-native-ffi-runtime-webgl", "WebGL", "opengl"),
  WEBGPU("webgpu", "maplibre-native-ffi-runtime-webgpu", "WebGPU", "webgpu"),
}

class MaplibreRuntimeConvention(val runtime: MaplibreRuntime)

fun Project.configureMaplibreRuntime(runtime: MaplibreRuntime) {
  extensions.add("maplibreRuntime", MaplibreRuntimeConvention(runtime))
  apply(mapOf("from" to rootProject.file("gradle/kotlin-runtime.gradle.kts")))
}
