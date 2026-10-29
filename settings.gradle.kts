pluginManagement {
  repositories {
    google()
    mavenCentral()
    gradlePluginPortal()
  }
}

rootProject.name = "maplibre-native-ffi"

include(":bindings:kotlin")

include(":bindings:kotlin:runtimes:metal")

include(":bindings:kotlin:runtimes:opengl")

include(":bindings:kotlin:runtimes:vulkan")

include(":bindings:kotlin:runtimes:webgpu")
