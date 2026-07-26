package org.maplibre.nativeffi.gradle

import org.gradle.api.Project

enum class MaplibreRuntimeBackend(val id: String, val displayName: String) {
  OPENGL("opengl", "OpenGL"),
  VULKAN("vulkan", "Vulkan"),
  METAL("metal", "Metal"),
}

enum class MaplibreRuntimeTargetFamily {
  LINUX,
  APPLE,
}

class MaplibreRuntimeConvention(
  val backend: MaplibreRuntimeBackend,
  val targetFamily: MaplibreRuntimeTargetFamily,
)

fun Project.configureMaplibreRuntime(
  backend: MaplibreRuntimeBackend,
  targetFamily: MaplibreRuntimeTargetFamily,
) {
  extensions.add(
    "maplibreRuntime",
    MaplibreRuntimeConvention(backend = backend, targetFamily = targetFamily),
  )
  apply(mapOf("from" to rootProject.file("gradle/kotlin-runtime.gradle.kts")))
}
