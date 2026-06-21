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
  }
}

dependencies { implementation(project(":bindings:kotlin")) }
