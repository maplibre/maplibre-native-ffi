import org.gradle.api.tasks.testing.Test
import org.jetbrains.kotlin.gradle.ExperimentalWasmDsl
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.targets.js.testing.KotlinJsTest
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
// Struct offsets the browser binding writes descriptors at. Checked in like the jextract output,
// and regenerated from the browser ABI manifest rather than hand-maintained.
val checkedInWasmLayoutSources = layout.projectDirectory.dir("src/wasmJsMain/generated")
// The prelinked Emscripten module the browser binding drives, which the browser build leaves beside
// its wasm and its ABI manifest. Named by a property the way the host native install is, because
// the browser target has no install step to read it from.
val browserModuleSourceDir =
  providers
    .gradleProperty("maplibre.browser.moduleDir")
    .map(rootProject::file)
    .orElse(rootProject.layout.buildDirectory.dir("browser-module-unconfigured").map { it.asFile })
val browserModuleConfigured = providers.gradleProperty("maplibre.browser.moduleDir").isPresent
// The three files the wasmJs test page serves. Collected into a directory of their own so the test
// harness serves the module and nothing else out of the browser build tree.
val packagedBrowserModule = layout.buildDirectory.dir("wasmJsBrowserModule")
val testBrowserPath =
  providers
    .environmentVariable("MLN_FFI_TEST_BROWSER")
    .orElse(providers.environmentVariable("CHROME_PATH"))
    .orElse(providers.environmentVariable("CHROME_BIN"))
val packagedAndroidBindingLibs = layout.buildDirectory.dir("generated/jniLibs/androidMain")
val generatedJavaCppSources =
  layout.buildDirectory.dir("generated/sources/javacpp/androidMain/java")
val mavenGroup = providers.gradleProperty("maplibre.maven.group").get()
val mavenVersion = providers.gradleProperty("maplibre.maven.version").get()
val mavenArtifact = "maplibre-native-ffi"

kotlin {
  iosArm64()
  iosSimulatorArm64()
  linuxX64()
  macosArm64()

  // The browser binding calls a prelinked Emscripten module through JavaScript rather than a
  // shared library, and keeps the common synchronous API by parking a Kotlin stack on a promise.
  // Both mechanisms are experimental in Kotlin 2.4, so the opt-ins are target-wide rather than
  // repeated on every declaration that reaches native.
  @OptIn(ExperimentalWasmDsl::class)
  wasmJs {
    browser {
      testTask {
        // Karma serves the page, so it also sets the cross-origin isolation headers the module's
        // pthreads need and serves the module beside the test bundle. Both live in karma.config.d,
        // which Kotlin appends to the generated Karma configuration. Naming the launcher here is
        // what puts karma-chrome-launcher on the harness; karma.config.d then selects a launcher of
        // its own that carries the flags a container needs.
        useKarma { useChromeHeadless() }
      }
    }
    compilerOptions {
      optIn.addAll(
        "kotlin.js.ExperimentalWasmJsInterop",
        "kotlin.wasm.ExperimentalWasmInterop",
        "kotlin.wasm.unsafe.UnsafeWasmMemoryApi",
      )
    }
  }

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
  }

  sourceSets {
    androidMain { dependencies { implementation(libs.javacpp) } }

    wasmJsMain { kotlin.srcDir(checkedInWasmLayoutSources) }

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
      "iosArm64" to "$mavenArtifact-iosarm64",
      "iosSimulatorArm64" to "$mavenArtifact-iossimulatorarm64",
      "jvm" to "$mavenArtifact-jvm",
      "linuxX64" to "$mavenArtifact-linuxx64",
      "macosArm64" to "$mavenArtifact-macosarm64",
      "wasmJs" to "$mavenArtifact-wasm-js",
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

// The module, its wasm, and its ABI manifest travel together: the loader fetches the manifest
// beside the module before it instantiates anything, and refuses a module that arrives without one.
val packageBrowserModule =
  tasks.register<Sync>("packageBrowserModule") {
    group = "build"
    description = "Collects the prelinked Emscripten module that the wasmJs browser tests load."
    from(browserModuleSourceDir) {
      include("maplibre_native_c.mjs", "maplibre_native_c.wasm", "maplibre_native_c-abi.json")
    }
    into(packagedBrowserModule)
  }

tasks.named<KotlinJsTest>("wasmJsBrowserTest") {
  dependsOn(packageBrowserModule)
  inputs.dir(packagedBrowserModule).withPropertyName("browserModule")
  // Read by karma.config.d, which serves this directory to the test page. Karma runs as its own
  // process, so a Gradle-side path reaches it through the environment rather than through a
  // property.
  environment["MLN_FFI_BROWSER_MODULE_DIR"] = packagedBrowserModule.get().asFile.path
  // The same variables the C API browser runner honours, so one host setting names one browser for
  // every browser suite in the repository.
  testBrowserPath.orNull?.let { environment["CHROME_BIN"] = it }
  val moduleConfigured = browserModuleConfigured
  doFirst {
    check(moduleConfigured) {
      "The wasmJs browser tests drive the prelinked Emscripten module. Build it with " +
        "`mise run build emscripten-wasm32-webgl` and name it with " +
        "-Pmaplibre.browser.moduleDir=build/emscripten-wasm32-webgl/browser."
    }
  }
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
    ":bindings:kotlin-runtime-$androidBackend:assembleAndroidMain",
  )
}
