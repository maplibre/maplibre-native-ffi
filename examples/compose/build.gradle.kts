plugins {
  // The Android Gradle plugin comes from buildSrc, so the project applies it by ID.
  id("com.android.application")
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
  // An app outside this repository uses the Maven coordinates that the README shows instead.
  implementation(project(":bindings:kotlin"))
  implementation(project(":bindings:kotlin:runtimes:native"))
  implementation(libs.androidx.activity.compose)
  implementation(platform(libs.androidx.compose.bom))
  implementation(libs.androidx.compose.foundation)
  implementation(libs.androidx.lifecycle.viewmodel.compose)
}
