import org.gradle.api.tasks.Exec
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.requiredEnvironmentVariable

plugins { id("com.android.application") }

val androidBackend =
  AndroidTarget.parseBackend(
    providers.gradleProperty("maplibre.android.backend").getOrElse(AndroidTarget.DEFAULT_BACKEND)
  )
val androidTargets =
  AndroidTarget.parseAbis(
    providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS)
  )
val androidCmakeVersion = requiredEnvironmentVariable("MLN_FFI_ANDROID_CMAKE_VERSION")
val androidNdkVersion = requiredEnvironmentVariable("MLN_FFI_ANDROID_NDK_VERSION")
val usePublishedKotlin =
  providers.gradleProperty("maplibre.usePublishedKotlin").map(String::toBoolean).getOrElse(false)
val maplibrePublicationVersion = providers.gradleProperty("maplibre.maven.version").get()

repositories {
  providers.gradleProperty("maplibre.maven.localRepository").orNull?.let {
    maven { url = rootProject.uri(it) }
  }
}

android {
  namespace = "org.maplibre.nativeffi.examples.androidmap"
  compileSdk = libs.versions.android.compileSdk.get().toInt()
  ndkVersion = androidNdkVersion

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
        version = androidCmakeVersion
      }
    }
  }
}

// The runtime AAR carries the native library and the TLS component; the binding
// adds the Kotlin API over it.
dependencies {
  if (usePublishedKotlin) {
    implementation("org.maplibre.nativeffi:maplibre-native-ffi:$maplibrePublicationVersion")
    implementation(
      "org.maplibre.nativeffi:maplibre-native-ffi-runtime-$androidBackend:" +
        maplibrePublicationVersion
    )
  } else {
    implementation(project(":bindings:kotlin"))
    implementation(project(":bindings:kotlin-runtime-$androidBackend"))
  }
}

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
