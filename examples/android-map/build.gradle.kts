plugins { id("com.android.application") }

android {
  namespace = "org.maplibre.nativeffi.examples.androidmap"
  compileSdk = 36

  defaultConfig {
    applicationId = "org.maplibre.nativeffi.examples.androidmap"
    minSdk = 24
    targetSdk = 36
    versionCode = 1
    versionName = "0"

    ndk { abiFilters += "arm64-v8a" }
  }
}

dependencies { implementation(project(":bindings:kotlin")) }
