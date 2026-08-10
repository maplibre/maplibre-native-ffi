package org.maplibre.nativeffi.gradle

import org.gradle.api.Project

enum class MaplibreRuntimeBackend(val id: String, val displayName: String) {
  OPENGL("opengl", "OpenGL"),
  VULKAN("vulkan", "Vulkan"),
  METAL("metal", "Metal"),
}

class MaplibreRuntimeConvention(val backend: MaplibreRuntimeBackend)

fun Project.configureMaplibreRuntime(backend: MaplibreRuntimeBackend) {
  extensions.add("maplibreRuntime", MaplibreRuntimeConvention(backend))
  apply(mapOf("from" to rootProject.file("gradle/kotlin-runtime.gradle.kts")))
}
