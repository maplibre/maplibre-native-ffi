import groovy.json.JsonSlurper
import org.gradle.api.artifacts.dsl.RepositoryHandler
import org.gradle.api.artifacts.repositories.MavenArtifactRepository
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins { id("com.android.application") }

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories {
  google()
  mavenCentral()
  rustlsPlatformVerifier()
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

dependencies {
  implementation(project(":bindings:kotlin"))
  implementation("rustls:rustls-platform-verifier:latest.release")
}

fun RepositoryHandler.rustlsPlatformVerifier(): MavenArtifactRepository {
  @Suppress("UnstableApiUsage")
  val metadata =
    providers
      .exec {
        workingDir = rootProject.projectDir
        commandLine(
          "cargo",
          "metadata",
          "--format-version",
          "1",
          "--filter-platform",
          "aarch64-linux-android",
          "--manifest-path",
          "src/platform/rust/Cargo.toml",
        )
      }
      .standardOutput
      .asText
      .get()

  @Suppress("UNCHECKED_CAST")
  val packages =
    (JsonSlurper().parseText(metadata) as Map<String, Any>)["packages"] as List<Map<String, Any>>
  val manifestPath =
    packages.first { it["name"] == "rustls-platform-verifier-android" }["manifest_path"] as String
  return maven {
    url = uri(File(File(manifestPath).parentFile, "maven"))
    metadataSources.artifact()
  }
}

tasks
  .matching { it.name == "preBuild" }
  .configureEach {
    dependsOn(packageMaplibreNativeCLibrary)
    dependsOn(":bindings:kotlin:generateAndroidJavaCppBindings")
    inputs.file(maplibreNativeC.libraryPath).withPropertyName("maplibreNativeCLibrary")
    inputs.file(maplibreNativeC.propertiesFile).withPropertyName("maplibreNativeCProperties")
  }
