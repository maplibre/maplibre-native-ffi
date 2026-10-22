pluginManagement {
  repositories {
    google()
    mavenCentral()
    gradlePluginPortal()
  }
}

rootProject.name = "maplibre-native-ffi"

include(":bindings:rustls-platform-verifier-android")

include(":bindings:kotlin")

include(":bindings:kotlin:runtimes:metal")

include(":bindings:kotlin:runtimes:opengl")

include(":bindings:kotlin:runtimes:vulkan")
