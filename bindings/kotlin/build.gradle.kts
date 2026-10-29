import java.time.Duration
import org.gradle.api.tasks.testing.Test
import org.gradle.api.tasks.testing.logging.TestExceptionFormat
import org.gradle.api.tasks.testing.logging.TestLogEvent
import org.jetbrains.kotlin.gradle.ExperimentalWasmDsl
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.plugin.KotlinPlatformType
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.NativeBuildType
import org.jetbrains.kotlin.gradle.plugin.mpp.TestExecutable
import org.jetbrains.kotlin.gradle.targets.js.testing.KotlinJsTest
import org.jetbrains.kotlin.gradle.targets.native.tasks.KotlinNativeSimulatorTest
import org.jetbrains.kotlin.gradle.targets.native.tasks.KotlinNativeTest
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact
import org.maplibre.nativeffi.gradle.canonicalizeKmpRootMetadata
import org.maplibre.nativeffi.gradle.kotlinNativeTargetPlatform
import org.maplibre.nativeffi.gradle.nativeInstall
import org.maplibre.nativeffi.gradle.registerLocalBuildVerification

plugins {
  id("org.jetbrains.kotlin.multiplatform")
  id("com.android.kotlin.multiplatform.library")
  id("com.vanniktech.maven.publish")
  alias(libs.plugins.dokka)
}

val hostPlatform = HostPlatform.current()

val checkedInCHeaders = rootProject.layout.projectDirectory.dir("include")
// plugin.h includes upstream's mln/plugin/plugin_api.h from the submodule.
val upstreamPluginHeaders =
  rootProject.layout.projectDirectory.dir("third_party/maplibre-native/include")
val androidBackend =
  AndroidTarget.parseBackend(
    providers.gradleProperty("maplibre.android.backend").getOrElse(AndroidTarget.DEFAULT_BACKEND),
  )
val androidTargets =
  AndroidTarget.parseAbis(
    providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS),
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
  androidNativeX86()
  androidNativeX64()
  iosArm64()
  iosSimulatorArm64()
  tvosArm64()
  tvosSimulatorArm64()
  linuxArm64()
  linuxX64()
  macosArm64()

  // The browser target runs on the page, which never blocks, and calls the Emscripten module that
  // bindings/kotlin/wasm links. Karma serves that module; see karma.config.d.
  @OptIn(ExperimentalWasmDsl::class)
  wasmJs { browser { testTask { useKarma { useChromeHeadless() } } } }

  // threadedMain holds the code of the targets whose threads may block and whose native threads
  // may call Kotlin directly: the JVM, Android, and Kotlin/Native. jvmAndroidMain holds the code
  // the JVM and Android share through java.lang.ref and java.util.concurrent. The wasm target
  // joins only common.
  applyDefaultHierarchyTemplate {
    common {
      group("threaded") {
        group("jvmAndroid") {
          withJvm()
          withCompilations { it.target.platformType == KotlinPlatformType.androidJvm }
        }
        group("native")
      }
    }
  }

  jvmToolchain(libs.versions.java.toolchain.get().toInt())

  compilerOptions { freeCompilerArgs.add("-Xexpect-actual-classes") }

  jvm {
    compilerOptions {
      jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get()))
      // Compiles against the release's class library, so no newer JDK API slips in.
      freeCompilerArgs.add("-Xjdk-release=${libs.versions.java.release.get()}")
    }
  }

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
    // A test links or loads the local build's install for the platform's default backend.
    val install = nativeInstall(kotlinNativeTargetPlatform(name))
    install.prefix?.let(::MaplibreNativeCArtifact)?.let { maplibreNativeC ->
      binaries.all {
        linkerOpts("-L${maplibreNativeC.linkDir}")
        linkerOpts(maplibreNativeC.linkLibraries.map { "-l$it" })
        if (hostPlatform.isMac || hostPlatform.isLinux) {
          linkerOpts("-Wl,-rpath,${maplibreNativeC.runtimeLibraryDir}")
        }
        if (hostPlatform.isMac) {
          linkerOpts(maplibreNativeC.frameworks.flatMap { listOf("-framework", it) })
        }
      }
    }
    val verifyLocalBuild =
      registerLocalBuildVerification(
        "verify${name.replaceFirstChar(Char::uppercaseChar)}LocalBuild",
        install,
      )
    binaries.withType<TestExecutable>().configureEach {
      linkTaskProvider.configure { dependsOn(verifyLocalBuild) }
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

    // Every native test compiles against tests/graphics through cinterop. Android
    // loads the installed fixture beside the native library; other targets link its object.
    if (name.startsWith("androidNative")) {
      val toolchain = providers.gradleProperty("maplibre.android.nativeTestToolchain")
      binaries.withType<TestExecutable>().configureEach {
        // Kotlin's API 21 sysroot cannot resolve the API 26 library's transitive
        // libc symbols. Android resolves them from the device's libc at load time.
        linkerOpts("-lmln_test_graphics", "-Wl,--allow-shlib-undefined")
        toolchain.orNull?.let { directory ->
          val host =
            if (hostPlatform.isMac) {
              if (hostPlatform.isArm64) "macos_arm64" else "macos_x64"
            } else "linux_x64"
          val target = konanTarget.name.lowercase()
          freeCompilerArgs += "-Xoverride-konan-properties=targetToolchain.$host-$target=$directory"
        }
      }
    }
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
          listOf("xcrun", "--sdk", "iphoneos", "clang", "-target", "arm64-apple-ios15.0")
        "iosSimulatorArm64" ->
          listOf(
            "xcrun",
            "--sdk",
            "iphonesimulator",
            "clang",
            "-target",
            "arm64-apple-ios15.0-simulator",
          )
        "tvosArm64" ->
          listOf("xcrun", "--sdk", "appletvos", "clang", "-target", "arm64-apple-tvos15.0")
        "tvosSimulatorArm64" ->
          listOf(
            "xcrun",
            "--sdk",
            "appletvsimulator",
            "clang",
            "-target",
            "arm64-apple-tvos15.0-simulator",
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
              ),
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

    // TestResult lets one test body return to a framework that cannot block for it.
    commonTest.dependencies {
      implementation(kotlin("test"))
      implementation(libs.coroutines.test)
    }

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
        // 32-bit Android and ULong on the other native targets. These declarations
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
      "androidNativeX86" to "$mavenArtifact-androidnativex86",
      "androidNativeX64" to "$mavenArtifact-androidnativex64",
      "iosArm64" to "$mavenArtifact-iosarm64",
      "iosSimulatorArm64" to "$mavenArtifact-iossimulatorarm64",
      "tvosArm64" to "$mavenArtifact-tvosarm64",
      "tvosSimulatorArm64" to "$mavenArtifact-tvossimulatorarm64",
      "jvm" to "$mavenArtifact-jvm",
      "linuxArm64" to "$mavenArtifact-linuxarm64",
      "linuxX64" to "$mavenArtifact-linuxx64",
      "macosArm64" to "$mavenArtifact-macosarm64",
      "wasmJs" to "$mavenArtifact-wasm-js",
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
            .absolutePath,
        )
      }
    }
    // The JNI shim is private to this binding, so it ships in this AAR rather
    // than in the shared runtime AARs.
    androidTargets.forEach { target ->
      variant.sources.jniLibs?.addStaticSourceDirectory(
        packagedAndroidBindingLibs.get().dir(target.cargoTarget).asFile.absolutePath,
      )
    }
  }
}

tasks.configureEach {
  if (name == "mergeAndroidMainJniLibFolders") dependsOn("packageAndroidBindingLibraries")
}

val jvmInstall = nativeInstall(hostPlatform.targetPlatform)
val verifyJvmLocalBuild = registerLocalBuildVerification("verifyJvmLocalBuild", jvmInstall)

class TestClasspathArguments(@get:Classpath val classpath: FileCollection) :
  CommandLineArgumentProvider {
  override fun asArguments() = listOf("-Dorg.maplibre.nativeffi.test.classpath=${classpath.asPath}")
}

tasks.named<Test>("jvmTest") {
  // Each test bounds its own waits; this catches a hang no test-level timeout can interrupt.
  timeout.set(Duration.ofMinutes(5))
  jvmArgs("--enable-native-access=ALL-UNNAMED")
  jvmArgumentProviders.add(TestClasspathArguments(classpath))
  dependsOn(verifyJvmLocalBuild)
  jvmInstall.prefix?.let(::MaplibreNativeCArtifact)?.let { maplibreNativeC ->
    systemProperty("java.library.path", maplibreNativeC.runtimeLibraryDir.absolutePath)
    systemProperty(
      "org.maplibre.nativeffi.test.graphics.library",
      maplibreNativeC.testGraphicsLibraryPath.absolutePath,
    )
    inputs.dir(maplibreNativeC.installDir).withPropertyName("maplibreNativeCInstall")
  }
  testLogging { exceptionFormat = TestExceptionFormat.FULL }
}

// Alpine and the FreeBSD virtual machine see only the repository, so the suite's classpath is
// copied into it, and a JVM there runs the suite through DeviceSuite.
tasks.register<Sync>("jvmDeviceTestClasspath") {
  val test = kotlin.jvm().compilations.getByName("test")
  val classpath = files(test.output.allOutputs, test.runtimeDependencyFiles)
  into(layout.buildDirectory.dir("jvm-device-test"))
  from(classpath.filter(File::isFile)) { into("lib") }
  from(classpath.filter(File::isDirectory)) { into("classes") }
}

// The wasm suite runs in headless Chromium against the browser module that bindings/kotlin/wasm
// links. The mise test:wasm task passes that module's directory, its render backend, and the
// Chromium to run. The canary run selects only HarnessCanaryTest, which fails on purpose; every
// other run leaves it out. On WebGPU, headless Chromium loses every WebGPU device of a page once a
// WebGPU canvas renders, and later adapter requests resolve to null. CanvasSurfaceTest therefore
// runs on a page of its own there: the canvas run selects only it, and the main run leaves it out.
val wasmModuleDir = providers.gradleProperty("maplibre.wasm.moduleDir")
val wasmRenderBackend = providers.gradleProperty("maplibre.wasm.renderBackend")
val wasmChromium = providers.gradleProperty("maplibre.wasm.chromium")
val wasmCanary =
  providers.gradleProperty("maplibre.wasm.canary").map(String::toBoolean).getOrElse(false)
val wasmCanvasRun =
  providers.gradleProperty("maplibre.wasm.canvasRun").map(String::toBoolean).getOrElse(false)

tasks.named<KotlinJsTest>("wasmJsBrowserTest") {
  val moduleDir = wasmModuleDir.orNull?.let(rootProject::file)
  if (moduleDir != null) {
    environment("MLN_KOTLIN_WASM_MODULE_DIR", moduleDir.absolutePath)
    inputs.dir(moduleDir).withPropertyName("maplibreWasmModule")
  }
  val renderBackend = wasmRenderBackend.orNull
  renderBackend?.let { environment("MLN_KOTLIN_WASM_RENDER_BACKEND", it) }
  wasmChromium.orNull?.let { environment("CHROME_BIN", it) }
  val configured = moduleDir != null && renderBackend != null && wasmChromium.isPresent
  doFirst {
    if (!configured) {
      throw GradleException(
        "wasmJsBrowserTest needs -Pmaplibre.wasm.moduleDir, -Pmaplibre.wasm.renderBackend, and " +
          "-Pmaplibre.wasm.chromium; run it through mise run //bindings/kotlin:test:wasm",
      )
    }
  }
  filter.apply {
    if (wasmCanary) includeTestsMatching("org.maplibre.nativeffi.browser.HarnessCanaryTest")
    else excludeTestsMatching("org.maplibre.nativeffi.browser.HarnessCanaryTest")
    if (renderBackend == "webgpu" && !wasmCanary) {
      val canvasTest = "org.maplibre.nativeffi.render.CanvasSurfaceTest"
      if (wasmCanvasRun) includeTestsMatching(canvasTest) else excludeTestsMatching(canvasTest)
      // A --tests filter may select tests of only one of the two runs.
      isFailOnNoMatchingTests = false
    }
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
      rootProject.layout.projectDirectory.file(".mise/tasks/kotlin/check-android-api-floor").asFile,
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
