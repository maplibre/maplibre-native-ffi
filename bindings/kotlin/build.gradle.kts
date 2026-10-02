import java.time.Duration
import org.gradle.api.tasks.testing.AbstractTestTask
import org.gradle.api.tasks.testing.Test
import org.gradle.api.tasks.testing.logging.TestExceptionFormat
import org.gradle.api.tasks.testing.logging.TestLogEvent
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.plugin.KotlinPlatformType
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.NativeBuildType
import org.jetbrains.kotlin.gradle.plugin.mpp.TestExecutable
import org.jetbrains.kotlin.gradle.targets.native.tasks.KotlinNativeSimulatorTest
import org.jetbrains.kotlin.gradle.targets.native.tasks.KotlinNativeTest
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
// plugin.h includes upstream's mln/plugin/plugin_api.h from the submodule.
val upstreamPluginHeaders =
  rootProject.layout.projectDirectory.dir("third_party/maplibre-native/include")
val androidBackend =
  AndroidTarget.parseBackend(
    providers.gradleProperty("maplibre.android.backend").getOrElse(AndroidTarget.DEFAULT_BACKEND)
  )
val androidTargets =
  AndroidTarget.parseAbis(
    providers
      .gradleProperty("maplibre.android.abis")
      .getOrElse(AndroidTarget.defaultAbis(androidBackend)),
    androidBackend,
  )
val packagedAndroidBindingLibs = layout.buildDirectory.dir("generated/jniLibs/androidMain")
val mavenGroup = providers.gradleProperty("maplibre.maven.group").get()
val mavenVersion = providers.gradleProperty("maplibre.maven.version").get()
val mavenArtifact = "maplibre-native-ffi"
val minifyAndroidDeviceTests =
  providers.gradleProperty("maplibre.android.testMinify").map(String::toBoolean).getOrElse(false)
val androidConsumerKeepRules =
  file("src/androidMain/resources/META-INF/proguard/maplibre-native-ffi-jni.pro")

kotlin {
  androidNativeArm32()
  androidNativeArm64()
  androidNativeX64()
  iosArm64()
  iosSimulatorArm64()
  tvosArm64()
  tvosSimulatorArm64()
  linuxArm64()
  linuxX64()
  macosArm64()

  // jvmAndroidMain holds the code the JVM and Android share through java.lang.ref and
  // java.util.concurrent.
  applyDefaultHierarchyTemplate {
    common {
      group("jvmAndroid") {
        withJvm()
        withCompilations { it.target.platformType == KotlinPlatformType.androidJvm }
      }
    }
  }

  jvmToolchain(libs.versions.java.toolchain.get().toInt())

  compilerOptions { freeCompilerArgs.add("-Xexpect-actual-classes") }

  jvm { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

  android {
    namespace = "org.maplibre.nativeffi"
    compileSdk = libs.versions.android.compileSdk.get().toInt()
    minSdk = libs.versions.android.minSdk.get().toInt()

    // Device-test APK assets are collected only when Android resource processing
    // is enabled. The published AAR has no res/ or assets/ of its own.
    androidResources {
      enable = true
      noCompress += "pmtiles"
    }
    withDeviceTestBuilder { sourceSetTreeName = "test" }
      .configure {
        instrumentationRunner = "org.maplibre.nativeffi.MaplibreTestRunner"
        execution = "HOST"
      }

    optimization {
      // CI enables minification while building the device-test artifact. Reuse
      // the published consumer rules so the JNI shim's natives and upcalls are
      // kept exactly as they are in a shrinking Android application.
      minify = minifyAndroidDeviceTests
      keepRules.file(androidConsumerKeepRules)
      testKeepRules.file(androidConsumerKeepRules)
      testKeepRules.file("src/androidDeviceTest/resources/proguard-rules.pro")
      consumerKeepRules.file(androidConsumerKeepRules)
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

    // Kotlin tests compile against a klib. macOS CI compiles the generated
    // Objective-C header so a public name that collides with a C macro fails
    // the same way an Xcode framework consumer would.
    if (hostPlatform.isMac && name == "macosArm64") {
      binaries.framework("objcExportCheck", listOf(NativeBuildType.DEBUG)) {
        baseName = "MaplibreNativeFfi"
        isStatic = true
      }
      val framework = binaries.getFramework("objcExportCheck", NativeBuildType.DEBUG)
      val headerCheckScript = file("scripts/check-objc-export-header.sh")
      val generatedHeader = framework.outputFile.resolve("Headers/${framework.baseName}.h")
      val check =
        tasks.register<Exec>("checkObjcExportHeader") {
          group = "verification"
          description = "Compiles the generated Objective-C header with clang."
          dependsOn(framework.linkTaskProvider)
          inputs.file(headerCheckScript)
          inputs.file(generatedHeader)
          commandLine("bash", headerCheckScript, "macosx", generatedHeader)
        }
      tasks.matching { it.name == "macosArm64Test" }.configureEach { dependsOn(check) }
    }

    compilations.getByName("main") {
      cinterops {
        create("maplibreNativeC") {
          defFile(project.file("src/nativeInterop/cinterop/maplibreNativeC.def"))
          includeDirs.headerFilterOnly(checkedInCHeaders.asFile)
          compilerOpts("-I${checkedInCHeaders.asFile}", "-I${upstreamPluginHeaders.asFile}")
        }
      }
    }

    // Every native test compiles against tests/graphics through cinterop. A target whose tests
    // run links graphics.c, compiled here for that target; Android Kotlin/Native tests only
    // compile, so their binaries never link it.
    val fixture = rootProject.file("tests/graphics")
    compilations.getByName("test") {
      cinterops.create("testGraphics") {
        defFile(project.file("src/nativeTest/cinterop/graphics.def"))
        includeDirs(fixture.resolve("include"))
      }
    }
    val graphicsCompiler =
      when (name) {
        "linuxX64" -> listOf("clang", "--target=x86_64-linux-gnu")
        "linuxArm64" -> listOf("clang", "--target=aarch64-linux-gnu")
        "macosArm64" ->
          listOf("xcrun", "--sdk", "macosx", "clang", "-target", "arm64-apple-macos11.0")
        "iosArm64" ->
          listOf("xcrun", "--sdk", "iphoneos", "clang", "-target", "arm64-apple-ios14.0")
        "iosSimulatorArm64" ->
          listOf(
            "xcrun",
            "--sdk",
            "iphonesimulator",
            "clang",
            "-target",
            "arm64-apple-ios14.0-simulator",
          )
        "tvosArm64" ->
          listOf("xcrun", "--sdk", "appletvos", "clang", "-target", "arm64-apple-tvos14.0")
        "tvosSimulatorArm64" ->
          listOf(
            "xcrun",
            "--sdk",
            "appletvsimulator",
            "clang",
            "-target",
            "arm64-apple-tvos14.0-simulator",
          )
        else -> null
      }
    if (graphicsCompiler != null) {
      val linksDl = name.startsWith("linux")
      val vulkanHeaders =
        rootProject.file("third_party/maplibre-native/vendor/Vulkan-Headers/include")
      val objectFile = layout.buildDirectory.file("graphics-test/$name/graphics.o")
      val compileGraphics =
        tasks.register<Exec>("compileTestGraphics${name.replaceFirstChar { it.uppercase() }}") {
          inputs.dir(fixture)
          inputs.dir(vulkanHeaders)
          outputs.file(objectFile)
          doFirst { objectFile.get().asFile.parentFile.mkdirs() }
          commandLine(
            graphicsCompiler +
              listOf(
                "-std=c11",
                "-fPIC",
                "-c",
                "-I${fixture.resolve("include")}",
                "-I$vulkanHeaders",
                fixture.resolve("graphics.c").absolutePath,
                "-o",
                objectFile.get().asFile.absolutePath,
              )
          )
        }
      binaries.withType<TestExecutable>().configureEach {
        linkTaskProvider.configure {
          dependsOn(compileGraphics)
          inputs.file(objectFile)
        }
        linkerOpts(objectFile.get().asFile.absolutePath)
        if (linksDl) linkerOpts("-ldl")
      }
    }
  }

  sourceSets {
    // Deferred is part of the public binding surface.
    commonMain.dependencies { api(libs.coroutines) }

    named("androidDeviceTest") {
      dependencies {
        implementation(kotlin("test"))
        implementation(libs.androidx.test.runner)
        implementation(project(":bindings:kotlin:runtimes:$androidBackend"))
      }
    }

    commonTest.dependencies { implementation(kotlin("test")) }

    configureEach {
      if (
        name.startsWith("native") ||
          name.startsWith("androidNative") ||
          name.startsWith("apple") ||
          name.startsWith("ios") ||
          name.startsWith("tvos") ||
          name.startsWith("linux") ||
          name.startsWith("macos")
      ) {
        // C interop commonizes size_t as an unsafe number because it is UInt on
        // Android ARM32 and ULong on the other native targets. These declarations
        // remain internal and are used only at the target-local C boundary.
        languageSettings.optIn("kotlinx.cinterop.UnsafeNumber")
      }
    }
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
      "androidNativeArm32" to "$mavenArtifact-androidnativearm32",
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

extensions.extraProperties["maplibreAndroidSdkDirectory"] =
  androidComponents.sdkComponents.sdkDirectory

extensions.extraProperties["maplibreAndroidBindingLibsDirectory"] = packagedAndroidBindingLibs

apply(from = "gradle/jni-android.gradle.kts")

apply(from = "gradle/graphics-tests.gradle.kts")

androidComponents {
  onVariants { variant ->
    variant.deviceTests.values.forEach { test ->
      androidTargets.forEach { target ->
        test.sources.jniLibs?.addStaticSourceDirectory(
          layout.buildDirectory
            .dir("generated/jniLibs/graphicsTest/${target.cargoTarget}")
            .get()
            .asFile
            .absolutePath
        )
      }
    }
    // The JNI shim is private to this binding, so it ships in this AAR rather
    // than in the shared runtime AARs.
    androidTargets.forEach { target ->
      variant.sources.jniLibs?.addStaticSourceDirectory(
        packagedAndroidBindingLibs.get().dir(target.cargoTarget).asFile.absolutePath
      )
    }
  }
}

tasks.configureEach {
  if (name == "mergeAndroidMainJniLibFolders") dependsOn("packageAndroidBindingLibraries")
}

val hostNativeInstallConfigured = providers.gradleProperty("maplibreNativeCInstallDir").isPresent

class TestClasspathArguments(@get:Classpath val classpath: FileCollection) :
  CommandLineArgumentProvider {
  override fun asArguments() = listOf("-Dorg.maplibre.nativeffi.test.classpath=${classpath.asPath}")
}

tasks.named<Test>("jvmTest") {
  // Each test bounds its own waits; this catches a hang no test-level timeout can interrupt.
  timeout.set(Duration.ofMinutes(5))
  jvmArgs("--enable-native-access=ALL-UNNAMED")
  jvmArgumentProviders.add(TestClasspathArguments(classpath))
  systemProperty("org.maplibre.nativeffi.library.path", maplibreNativeC.libraryPath.absolutePath)
  systemProperty(
    "org.maplibre.nativeffi.test.graphics.library",
    maplibreNativeC.testGraphicsLibraryPath.absolutePath,
  )
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
  testLogging { exceptionFormat = TestExceptionFormat.FULL }
}

// A coverage run needs the tests to execute, because Gradle does not track the
// native profiles they write. An up-to-date or cached result would write none.
if (providers.environmentVariable("LLVM_PROFILE_FILE").isPresent) {
  tasks.withType<AbstractTestTask>().configureEach {
    doNotTrackState("A coverage run records native profiles on every run")
  }
}

tasks.withType<KotlinNativeTest>().configureEach {
  timeout.set(Duration.ofMinutes(5))
  testLogging {
    events(TestLogEvent.STARTED, TestLogEvent.PASSED, TestLogEvent.SKIPPED, TestLogEvent.FAILED)
    exceptionFormat = TestExceptionFormat.FULL
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
    dependsOn("compileAndroidMain")
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
