pluginManagement {
  repositories {
    google()
    mavenCentral()
    gradlePluginPortal()
  }
}

rootProject.name = "maplibre-native-ffi"

include(":bindings:kotlin")

include(":bindings:kotlin:runtimes:native")

include(":bindings:kotlin:runtimes:webgl")

include(":bindings:kotlin:runtimes:webgpu")

include(":examples:compose")
