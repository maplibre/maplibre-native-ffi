import com.vanniktech.maven.publish.AndroidSingleVariantLibrary
import com.vanniktech.maven.publish.JavadocJar
import com.vanniktech.maven.publish.SourcesJar

plugins {
  id("com.android.library")
  id("com.vanniktech.maven.publish")
}

val repositoryRoot = projectDir.resolve("../..")
val verifierSourceDirectory =
  repositoryRoot.resolve(
    "build/dependencies/rustls-platform-verifier/" +
      "android/rustls-platform-verifier/src/main/java"
  )
val verifierSourceFile =
  verifierSourceDirectory.resolve("org/rustls/platformverifier/CertificateVerifier.kt")

val verifyRustlsPlatformVerifierSource =
  tasks.register("verifyRustlsPlatformVerifierSource") {
    // Bind to a local so the action captures the file, not the enclosing script,
    // which the configuration cache cannot serialize.
    val sourceFile = verifierSourceFile
    inputs.file(sourceFile).optional()
    doLast {
      check(sourceFile.isFile) {
        "Missing patched Rustls platform-verifier source; run `mise deps` from the repository root"
      }
    }
  }

android {
  namespace = "org.maplibre.nativeffi.internal.rustlsplatformverifier"
  compileSdk = libs.versions.android.compileSdk.get().toInt()

  defaultConfig {
    minSdk = libs.versions.android.minSdk.get().toInt()
    buildConfigField("boolean", "TEST", "false")
    consumerProguardFiles("consumer-rules.pro")
  }

  buildFeatures { buildConfig = true }

  sourceSets.named("main") { kotlin.directories.add(verifierSourceDirectory.absolutePath) }
}

tasks.configureEach {
  if (name == "preBuild") {
    dependsOn(verifyRustlsPlatformVerifierSource)
  }
}

mavenPublishing {
  configure(AndroidSingleVariantLibrary(JavadocJar.Empty(), SourcesJar.Sources()))
  coordinates(
    groupId = providers.gradleProperty("maplibre.maven.group").get(),
    artifactId = "maplibre-native-ffi-android-platform",
    version = providers.gradleProperty("maplibre.maven.version").get(),
  )
  publishToMavenCentral()
  pom {
    name.set("MapLibre Native FFI Android platform support")
    description.set("Android platform support for MapLibre Native FFI hosts.")
  }
}

// Hosts that consume this AAR directly receive the upstream licenses and the
// statement of our modifications that Apache-2.0 section 4 requires.
tasks.withType<Zip>().configureEach {
  if (name == "bundleReleaseAar") {
    val licenseDirectory = "META-INF/licenses/rustls-platform-verifier"
    from(repositoryRoot.resolve("build/dependencies/rustls-platform-verifier/LICENSE-APACHE")) {
      into(licenseDirectory)
    }
    from(repositoryRoot.resolve("build/dependencies/rustls-platform-verifier/LICENSE-MIT")) {
      into(licenseDirectory)
    }
    from(rootProject.file("patches/rustls-platform-verifier/NOTICE")) { into(licenseDirectory) }
  }
}
