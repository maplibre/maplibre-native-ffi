plugins {
  alias(libs.plugins.android.application)
  alias(libs.plugins.kotlin.compose)
}

android {
  namespace = "org.maplibre.nativeffi.example.compose"
  compileSdk = 37

  defaultConfig {
    applicationId = "org.maplibre.nativeffi.example.compose"
    minSdk = 24
    targetSdk = 36
    versionCode = 1
    versionName = "1.0"
  }

  buildTypes {
    release {
      isMinifyEnabled = true
      proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"))
      // An example, so its release build signs with the debug key to install anywhere.
      signingConfig = signingConfigs.getByName("debug")
    }
  }

  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
  }

  buildFeatures { compose = true }
}

dependencies {
  implementation(libs.maplibre.native.ffi)
  implementation(libs.maplibre.native.ffi.runtime.opengl)
  implementation(libs.androidx.activity.compose)
  implementation(platform(libs.androidx.compose.bom))
  implementation(libs.androidx.compose.foundation)
  implementation(libs.androidx.lifecycle.viewmodel.compose)
}
