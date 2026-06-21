pluginManagement {
  repositories {
    google()
    mavenCentral()
    gradlePluginPortal()
  }
}

rootProject.name = "maplibre-native-ffi"

include(":bindings:kotlin")

include(":examples:android-map")

include(":examples:lwjgl-map")
