import org.gradle.api.tasks.Exec
import org.maplibre.nativeffi.gradle.AndroidTarget

plugins { alias(libs.plugins.android.application) }

val androidBackend =
  AndroidTarget.parseBackend(
    providers.gradleProperty("maplibre.android.backend").getOrElse(AndroidTarget.DEFAULT_BACKEND)
  )
val androidTargets =
  AndroidTarget.parseAbis(
    providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS)
  )

android {
  namespace = "org.maplibre.nativeffi.examples.androidmap"
  compileSdk = libs.versions.android.compileSdk.get().toInt()
  ndkVersion = libs.versions.android.ndk.get()

  defaultConfig {
    applicationId = "org.maplibre.nativeffi.examples.androidmap"
    minSdk = libs.versions.android.minSdk.get().toInt()
    targetSdk = libs.versions.android.targetSdk.get().toInt()
    versionCode = 1
    versionName = "0"

    ndk { abiFilters += androidTargets.map { it.ndkAbi } }

    if (androidBackend == "vulkan") {
      externalNativeBuild { cmake { arguments += "-DANDROID_STL=c++_static" } }
    }

    buildConfigField("String", "RENDER_BACKEND", "\"$androidBackend\"")
  }

  buildFeatures { buildConfig = true }

  if (androidBackend == "vulkan") {
    externalNativeBuild {
      cmake {
        path = file("src/main/cpp/CMakeLists.txt")
        version = libs.versions.android.cmake.get()
      }
    }
  }
}

dependencies { implementation(project(":bindings:kotlin")) }

tasks.register<Exec>("runDebug") {
  group = "application"
  description = "Installs and starts the debug Android map application."
  dependsOn("installDebug")
  executable(androidComponents.sdkComponents.adb.get().asFile)
  args(
    "shell",
    "am",
    "start",
    "-S",
    "-n",
    "org.maplibre.nativeffi.examples.androidmap/.MainActivity",
  )
}
