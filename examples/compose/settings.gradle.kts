pluginManagement {
  repositories {
    google()
    mavenCentral()
    gradlePluginPortal()
  }
}

dependencyResolutionManagement {
  repositoriesMode = RepositoriesMode.FAIL_ON_PROJECT_REPOS
  repositories {
    google()
    mavenCentral()
  }
}

rootProject.name = "maplibre-compose-example"

include(":app")

// The app depends on the binding by its Maven coordinates, as any app does. This build takes them
// from the repository's own Gradle build instead, which packages the C library from build/. To
// use a published version, remove this block and add its repository above.
includeBuild("../..") {
  name = "maplibre-native-ffi"
  dependencySubstitution {
    substitute(module("org.maplibre.nativeffi:maplibre-native-ffi"))
      .using(project(":bindings:kotlin"))
    substitute(module("org.maplibre.nativeffi:maplibre-native-ffi-runtime-opengl"))
      .using(project(":bindings:kotlin:runtimes:opengl"))
  }
}
