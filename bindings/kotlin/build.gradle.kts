import java.time.Duration
import org.gradle.api.tasks.testing.Test
import org.gradle.api.tasks.testing.logging.TestLogEvent
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.targets.native.tasks.KotlinNativeSimulatorTest
import org.jetbrains.kotlin.gradle.targets.native.tasks.KotlinNativeTest
import org.jetbrains.kotlin.gradle.tasks.KotlinJvmCompile
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact
import org.maplibre.nativeffi.gradle.canonicalizeKmpRootMetadata

plugins {
  id("org.jetbrains.kotlin.multiplatform")
  id("com.android.kotlin.multiplatform.library")
  id("com.vanniktech.maven.publish")
  alias(libs.plugins.dokka)
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
val checkedInJextractSources = layout.projectDirectory.dir("src/jvmMain/generated")
val packagedAndroidBindingLibs = layout.buildDirectory.dir("generated/jniLibs/androidMain")
val generatedJavaCppSources =
  layout.buildDirectory.dir("generated/sources/javacpp/androidMain/java")
val mavenGroup = providers.gradleProperty("maplibre.maven.group").get()
val mavenVersion = providers.gradleProperty("maplibre.maven.version").get()
val mavenArtifact = "maplibre-native-ffi"

kotlin {
  androidNativeArm64()
  androidNativeX64()
  iosArm64()
  iosSimulatorArm64()
  tvosArm64()
  tvosSimulatorArm64()
  linuxArm64()
  linuxX64()
  macosArm64()

  jvmToolchain(libs.versions.java.toolchain.get().toInt())

  compilerOptions { freeCompilerArgs.add("-Xexpect-actual-classes") }

  jvm { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

  android {
    namespace = "org.maplibre.nativeffi"
    compileSdk = libs.versions.android.compileSdk.get().toInt()
    minSdk = libs.versions.android.minSdk.get().toInt()

    withJava()
    withDeviceTestBuilder { sourceSetTreeName = "test" }
      .configure {
        instrumentationRunner = "org.maplibre.nativeffi.MaplibreTestRunner"
        execution = "HOST"
      }

    optimization {
      consumerKeepRules.file(
        "src/androidMain/resources/META-INF/proguard/maplibre-native-ffi-javacpp.pro"
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

    if (name == "linuxX64" || name == "linuxArm64") {
      val eglLibDir =
        if (name == "linuxX64") "/usr/lib/x86_64-linux-gnu" else "/usr/lib/aarch64-linux-gnu"
      binaries.all { linkerOpts("-L$eglLibDir") }
      compilations.getByName("test") {
        cinterops {
          create("egl") {
            defFile(project.file("src/linuxTest/cinterop/egl.def"))
            includeDirs(project.file("src/linuxTest/cinterop"))
            compilerOpts("-I${project.file("src/linuxTest/cinterop")}")
          }
        }
      }
    }
  }

  sourceSets {
    androidMain { dependencies { implementation(libs.javacpp) } }

    named("androidDeviceTest") {
      dependencies {
        implementation(kotlin("test"))
        implementation(libs.androidx.test.runner)
        implementation(project(":bindings:kotlin:runtimes:$androidBackend"))
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

dokka { moduleName.set(mavenArtifact) }

canonicalizeKmpRootMetadata(
  group = mavenGroup,
  version = mavenVersion,
  targetModules =
    mapOf(
      "android" to "$mavenArtifact-android",
      "androidNativeArm64" to "$mavenArtifact-androidnativearm64",
      "androidNativeX64" to "$mavenArtifact-androidnativex64",
      "iosArm64" to "$mavenArtifact-iosarm64",
      "iosSimulatorArm64" to "$mavenArtifact-iossimulatorarm64",
      "tvosArm64" to "$mavenArtifact-tvosarm64",
      "tvosSimulatorArm64" to "$mavenArtifact-tvossimulatorarm64",
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

val lwjglNative = hostPlatform.lwjglNativeClassifier

dependencies {
  add("javaCppTool", libs.javacpp)
  "jvmTestImplementation"(platform(libs.lwjgl.bom))
  "jvmTestImplementation"(libs.lwjgl)
  "jvmTestImplementation"(libs.lwjgl.egl)
  "jvmTestRuntimeOnly"(variantOf(libs.lwjgl) { classifier(lwjglNative) })
}

apply(from = "gradle/jextract-jvm.gradle.kts")

extensions.extraProperties["maplibreAndroidSdkDirectory"] =
  androidComponents.sdkComponents.sdkDirectory

extensions.extraProperties["maplibreAndroidBindingLibsDirectory"] = packagedAndroidBindingLibs

apply(from = "gradle/javacpp-android.gradle.kts")

tasks.named<KotlinJvmCompile>("compileKotlinJvm") { source(checkedInJextractSources) }

androidComponents {
  onVariants { variant ->
    // Android KMP does not currently expose a task-provider-backed generated Java source hook.
    // Keep the explicit task dependencies below in sync with this static source directory.
    variant.sources.java?.addStaticSourceDirectory(
      generatedJavaCppSources.get().asFile.absolutePath
    )
    // The JavaCPP bridge is private to this binding, so it ships in this AAR
    // rather than in the shared runtime AARs.
    androidTargets.forEach { target ->
      variant.sources.jniLibs?.addStaticSourceDirectory(
        packagedAndroidBindingLibs.get().dir(target.cargoTarget).asFile.absolutePath
      )
    }
  }
}

tasks.configureEach {
  when (name) {
    "androidSourcesJar",
    "compileAndroidMainJavaWithJavac",
    "compileAndroidMain",
    "extractAndroidMainAnnotations" -> dependsOn("generateAndroidJavaCppBindings")
    "mergeAndroidMainJniLibFolders" -> dependsOn("packageAndroidBindingLibraries")
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

tasks.withType<KotlinNativeTest>().configureEach {
  timeout.set(Duration.ofMinutes(5))
  testLogging {
    events(TestLogEvent.STARTED, TestLogEvent.PASSED, TestLogEvent.SKIPPED, TestLogEvent.FAILED)
  }
}

tasks.withType<KotlinNativeSimulatorTest>().configureEach {
  standalone.set(false)
  providers.environmentVariable("MLN_FFI_IOS_SIMULATOR_DEVICE_ID").orNull?.let(device::set)
}

// AGP's KMP library plugin registers no lint variant, so `NewApi` never runs for
// androidMain. This task stands in for it.
val checkAndroidApiFloor =
  tasks.register<Exec>("checkAndroidApiFloor") {
    group = "verification"
    description = "Verifies androidMain bytecode stays on the android-minSdk floor."
    dependsOn("compileAndroidMain", "compileAndroidMainJavaWithJavac")
    workingDir = rootProject.layout.projectDirectory.asFile
    commandLine(
      rootProject.layout.projectDirectory.file(".mise/tasks/kotlin/check-android-api-floor").asFile
    )
  }

tasks.register("androidBuild") {
  group = "build"
  description = "Builds the Android binding and selected native runtime AAR."
  dependsOn(
    "packageAndroidNativeLibraries",
    "assembleAndroidMain",
    checkAndroidApiFloor,
    ":bindings:kotlin:runtimes:$androidBackend:assembleAndroidMain",
  )
}
