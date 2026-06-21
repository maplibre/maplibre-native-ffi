plugins { alias(libs.plugins.android.application) }

android {
  namespace = "org.maplibre.nativeffi.examples.androidmap"
  compileSdk = libs.versions.android.compileSdk.get().toInt()

  defaultConfig {
    applicationId = "org.maplibre.nativeffi.examples.androidmap"
    minSdk = libs.versions.android.minSdk.get().toInt()
    targetSdk = libs.versions.android.targetSdk.get().toInt()
    versionCode = 1
    versionName = "0"

    ndk { abiFilters += "arm64-v8a" }

    externalNativeBuild { cmake { arguments += "-DANDROID_STL=c++_shared" } }

    buildConfigField(
      "String",
      "RENDER_BACKEND",
      "\"${providers.environmentVariable("MLN_FFI_RENDER_BACKEND").getOrElse("opengl")}\"",
    )
  }

  buildFeatures { buildConfig = true }

  externalNativeBuild { cmake { path = file("src/main/cpp/CMakeLists.txt") } }
}

dependencies { implementation(project(":bindings:kotlin")) }
