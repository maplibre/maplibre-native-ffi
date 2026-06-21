pluginManagement {
  repositories {
    google()
    mavenCentral()
    gradlePluginPortal()
  }
}

rootProject.name = "maplibre-native-ffi"

include(":bindings:kotlin")

include(":examples:lwjgl-map")
