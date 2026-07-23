import org.gradle.api.tasks.Sync
import org.gradle.api.tasks.testing.Test
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.tasks.KotlinJvmCompile
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.CargoPackage
import org.maplibre.nativeffi.gradle.ExtractAarClassesJar
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact
import org.maplibre.nativeffi.gradle.canonicalizeKmpRootMetadata

plugins {
  id("org.jetbrains.kotlin.multiplatform")
  id("com.android.kotlin.multiplatform.library")
  id("com.vanniktech.maven.publish")
}

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

val hostPlatform = HostPlatform.current()
val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val checkedInCHeaders = rootProject.layout.projectDirectory.dir("include")
val androidBackend =
  AndroidTarget.parseBackend(
    providers.gradleProperty("maplibre.android.backend").getOrElse(AndroidTarget.DEFAULT_BACKEND)
  )
val androidTargets =
  AndroidTarget.parseAbis(
    providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS)
  )
val generatedJextractSources = layout.buildDirectory.dir("generated/sources/jextract/jvmMain/java")
val generatedJavaCppSources =
  layout.buildDirectory.dir("generated/sources/javacpp/androidMain/java")
val mavenGroup = providers.gradleProperty("maplibre.maven.group").get()
val mavenVersion = providers.gradleProperty("maplibre.maven.version").get()
val mavenArtifact = "maplibre-native-ffi"
val rustlsPlatformVerifierPackage = CargoPackage.directory(project, "rustls-platform-verifier")
val rustlsPlatformVerifierAndroidPackage =
  CargoPackage.directory(project, "rustls-platform-verifier-android")
val rustlsPlatformVerifierAndroidAar =
  rustlsPlatformVerifierAndroidPackage.map { packageDirectory ->
    packageDirectory.resolve("maven").walkTopDown().single { it.isFile && it.extension == "aar" }
  }
val rustlsPlatformVerifierAndroidJar =
  layout.buildDirectory.file(
    "generated/dependencies/rustlsPlatformVerifierAndroid/rustls-platform-verifier-android.jar"
  )
val extractRustlsPlatformVerifierAndroidJar =
  tasks.register<ExtractAarClassesJar>("extractRustlsPlatformVerifierAndroidJar") {
    aarFile.set(layout.file(rustlsPlatformVerifierAndroidAar))
    outputJar.set(rustlsPlatformVerifierAndroidJar)
  }
val generateRustlsPlatformVerifierLicenses =
  tasks.register<Sync>("generateRustlsPlatformVerifierLicenses") {
    val packageDirectory = rustlsPlatformVerifierPackage
    from(packageDirectory.map { it.resolve("LICENSE-APACHE") }) {
      into("META-INF/licenses/rustls-platform-verifier")
    }
    from(packageDirectory.map { it.resolve("LICENSE-MIT") }) {
      into("META-INF/licenses/rustls-platform-verifier")
    }
    into(layout.buildDirectory.dir("generated/resources/rustlsPlatformVerifier/androidMain"))
  }

kotlin {
  iosArm64()
  iosSimulatorArm64()
  linuxX64()
  linuxArm64()
  macosArm64()

  jvmToolchain(libs.versions.java.toolchain.get().toInt())

  compilerOptions { freeCompilerArgs.add("-Xexpect-actual-classes") }

  jvm { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

  android {
    namespace = "org.maplibre.nativeffi"
    compileSdk = libs.versions.android.compileSdk.get().toInt()
    minSdk = libs.versions.android.minSdk.get().toInt()

    withJava()

    optimization {
      consumerKeepRules.file(
        "src/androidMain/resources/META-INF/proguard/maplibre-native-ffi-rustls.pro"
      )
      consumerKeepRules.publish = true
    }

    compilerOptions {
      jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.android.release.get()))
    }
  }

  targets.withType<KotlinNativeTarget>().configureEach {
    binaries.all {
      linkerOpts(maplibreNativeC.linkDirs.map { "-L$it" })
      linkerOpts(maplibreNativeC.linkLibraries.map { "-l$it" })
      if (hostPlatform.isMac || hostPlatform.isLinux) {
        linkerOpts(maplibreNativeC.runtimeLinkLibraryDirs.map { "-Wl,-rpath,$it" })
      }
      if (hostPlatform.isMac) {
        linkerOpts(maplibreNativeC.frameworks.flatMap { listOf("-framework", it) })
      }
    }

    compilations.getByName("main") {
      cinterops {
        create("maplibreNativeC") {
          defFile(project.file("src/nativeInterop/cinterop/maplibreNativeC.def"))
          includeDirs.headerFilterOnly(checkedInCHeaders.asFile)
          compilerOpts("-I${checkedInCHeaders.asFile}")
        }
      }
    }
  }

  sourceSets {
    androidMain {
      resources.srcDir(generateRustlsPlatformVerifierLicenses)
      dependencies {
        implementation(libs.javacpp)
        implementation(
          files(rustlsPlatformVerifierAndroidJar).builtBy(extractRustlsPlatformVerifierAndroidJar)
        )
      }
    }

    commonTest.dependencies { implementation(kotlin("test")) }
  }
}

mavenPublishing {
  coordinates(groupId = mavenGroup, artifactId = mavenArtifact, version = mavenVersion)
  publishToMavenCentral()
  pom {
    name.set("MapLibre Native FFI Kotlin binding")
    description.set("Low-level Kotlin Multiplatform bindings for the MapLibre Native C API.")
  }
}

canonicalizeKmpRootMetadata(
  group = mavenGroup,
  rootModule = mavenArtifact,
  version = mavenVersion,
  targetModules =
    mapOf(
      "android" to "$mavenArtifact-android",
      "iosArm64" to "$mavenArtifact-iosarm64",
      "iosSimulatorArm64" to "$mavenArtifact-iossimulatorarm64",
      "jvm" to "$mavenArtifact-jvm",
      "linuxArm64" to "$mavenArtifact-linuxarm64",
      "linuxX64" to "$mavenArtifact-linuxx64",
      "macosArm64" to "$mavenArtifact-macosarm64",
    ),
)

configurations.register("javaCppTool") {
  isCanBeConsumed = false
  isCanBeResolved = true
}

dependencies.add("javaCppTool", libs.javacpp)

apply(from = "gradle/jextract-jvm.gradle.kts")

extensions.extraProperties["maplibreAndroidSdkDirectory"] =
  androidComponents.sdkComponents.sdkDirectory

apply(from = "gradle/javacpp-android.gradle.kts")

tasks.named<KotlinJvmCompile>("compileKotlinJvm") {
  dependsOn("generateJvmJextractBindings")
  source(generatedJextractSources)
}

androidComponents {
  onVariants { variant ->
    // Android KMP does not currently expose a task-provider-backed generated Java source hook.
    // Keep the explicit task dependencies below in sync with this static source directory.
    variant.sources.java?.addStaticSourceDirectory(
      generatedJavaCppSources.get().asFile.absolutePath
    )
  }
}

tasks.configureEach {
  when (name) {
    "compileAndroidMainJavaWithJavac",
    "compileAndroidMain",
    "extractAndroidMainAnnotations" -> dependsOn("generateAndroidJavaCppBindings")
  }
}

val hostNativeInstallConfigured = providers.gradleProperty("maplibreNativeCInstallDir").isPresent

tasks.named<Test>("jvmTest") {
  jvmArgs("--enable-native-access=ALL-UNNAMED")
  systemProperty("org.maplibre.nativeffi.library.path", maplibreNativeC.libraryPath.absolutePath)
  systemProperty(
    "org.maplibre.nativeffi.library.dirs",
    maplibreNativeC.loaderLibraryDirs.joinToString(File.pathSeparator) { it.absolutePath },
  )
  if (hostNativeInstallConfigured) {
    inputs.file(maplibreNativeC.libraryPath).withPropertyName("maplibreNativeCLibrary")
    inputs
      .files(maplibreNativeC.loaderLibraryDirs)
      .withPropertyName("maplibreNativeCLoaderLibraryDirs")
    inputs.dir(maplibreNativeC.installDir).withPropertyName("maplibreNativeCInstallDir")
  }
}

tasks.register("androidBuild") {
  group = "build"
  description = "Builds the Android binding and selected native runtime AAR."
  dependsOn(
    "packageAndroidNativeLibraries",
    "assembleAndroidMain",
    ":bindings:kotlin-runtime-$androidBackend:assembleAndroidMain",
  )
}
