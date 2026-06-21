import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins { id("com.android.application") }

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories {
  google()
  mavenCentral()
}

val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val packagedNativeLibs = layout.buildDirectory.dir("generated/jniLibs")

val packageMaplibreNativeCLibrary =
  tasks.register<Sync>("packageMaplibreNativeCLibrary") {
    from(maplibreNativeC.libraryPath)
    into(packagedNativeLibs.map { it.dir("arm64-v8a") })
  }

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

androidComponents {
  onVariants { variant ->
    variant.sources.jniLibs?.addStaticSourceDirectory(packagedNativeLibs.get().asFile.absolutePath)
  }
}

dependencies { implementation(project(":bindings:kotlin")) }

tasks
  .matching { it.name == "preBuild" }
  .configureEach {
    dependsOn(packageMaplibreNativeCLibrary)
    dependsOn(":bindings:kotlin:generateAndroidJavaCppBindings")
    inputs.file(maplibreNativeC.libraryPath).withPropertyName("maplibreNativeCLibrary")
    inputs.file(maplibreNativeC.propertiesFile).withPropertyName("maplibreNativeCProperties")
  }
