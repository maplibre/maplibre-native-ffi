plugins { id("com.android.library") }

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
    inputs.file(verifierSourceFile).optional()
    doLast {
      check(inputs.files.singleFile.isFile) {
        "Missing patched Rustls platform-verifier source; run `mise deps` from the repository root"
      }
    }
  }

android {
  namespace = "org.maplibre.nativeffi.internal.rustlsplatformverifier"
  compileSdk = 36

  defaultConfig {
    minSdk = 24
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
