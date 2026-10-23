import com.android.build.api.variant.KotlinMultiplatformAndroidComponentsExtension
import com.vanniktech.maven.publish.MavenPublishBaseExtension
import java.io.File
import org.gradle.api.publish.PublishingExtension
import org.gradle.api.publish.maven.MavenPublication
import org.gradle.api.tasks.bundling.Zip
import org.gradle.jvm.tasks.Jar
import org.jetbrains.kotlin.gradle.dsl.KotlinMultiplatformExtension
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.tasks.CInteropProcess
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreRuntimeBackend
import org.maplibre.nativeffi.gradle.MaplibreRuntimeConvention
import org.maplibre.nativeffi.gradle.VerifyAndroidRuntimeBackend
import org.maplibre.nativeffi.gradle.canonicalizeKmpRootMetadata
import org.maplibre.nativeffi.gradle.catalogVersionInt
import org.maplibre.nativeffi.gradle.embedMaplibreLicenseBundle
import org.maplibre.nativeffi.gradle.kotlinNativeTargetPlatform
import org.maplibre.nativeffi.gradle.localBuild
import org.maplibre.nativeffi.gradle.localBuildPrefix
import org.maplibre.nativeffi.gradle.nativePreset
import org.maplibre.nativeffi.gradle.registerLocalBuildVerification
import org.maplibre.nativeffi.gradle.requiredEnvironmentVariable

data class NativeTargetConfiguration(
  val definitionFileName: String,
  val staticLibraries: List<String> = listOf("libmaplibre-native-c.a"),
)

fun String.capitalized(): String = replaceFirstChar(Char::uppercaseChar)

fun String.taskSuffix(): String =
  split('-').joinToString("") { it.replaceFirstChar(Char::uppercaseChar) }

fun nativeTargets(backend: MaplibreRuntimeBackend): Map<String, NativeTargetConfiguration> =
  when (backend) {
    MaplibreRuntimeBackend.OPENGL,
    MaplibreRuntimeBackend.VULKAN ->
      buildMap {
        AndroidTarget.entries.forEach { target ->
          put(
            target.kotlinNativeTarget,
            NativeTargetConfiguration(
              "android-${backend.id}.def",
              listOf("libmaplibre-native-c.a", "libmln_ffi_platform.a"),
            ),
          )
        }
        put(
          "linuxArm64",
          NativeTargetConfiguration(
            "linux-${backend.id}.def",
            listOf("libmaplibre-native-c.a", "libmln_ffi_platform.a"),
          ),
        )
        put(
          "linuxX64",
          NativeTargetConfiguration(
            "linux-${backend.id}.def",
            listOf("libmaplibre-native-c.a", "libmln_ffi_platform.a"),
          ),
        )
        put("macosArm64", NativeTargetConfiguration("macos-${backend.id}.def"))
      }

    MaplibreRuntimeBackend.METAL ->
      mapOf(
        "iosArm64" to NativeTargetConfiguration("ios-metal.def"),
        "iosSimulatorArm64" to NativeTargetConfiguration("ios-metal.def"),
        "tvosArm64" to NativeTargetConfiguration("ios-metal.def"),
        "tvosSimulatorArm64" to NativeTargetConfiguration("ios-metal.def"),
        "macosArm64" to NativeTargetConfiguration("macos-metal.def"),
      )
  }

/** The install prefix in the local build that serves [targetPlatform] with [backend]. */
fun runtimeInstall(targetPlatform: String, backend: String): File? = localBuild?.let {
  localBuildPrefix(it, nativePreset(targetPlatform, backend))
}

fun configureJvmRuntimeArtifacts(backend: String, classifiers: Collection<String>) {
  val runtimeJars = classifiers.map { classifier ->
    val targetPlatform = HostPlatform.JVM_TARGET_PLATFORMS.getValue(classifier)
    val taskSuffix = classifier.taskSuffix()
    val verifyLocalBuild =
      registerLocalBuildVerification("verifyJvm${taskSuffix}LocalBuild", targetPlatform, backend)
    val install = runtimeInstall(targetPlatform, backend)

    tasks.register<Jar>("jvm${taskSuffix}RuntimeJar") {
      group = "build"
      description = "Packages the $backend native runtime for $classifier."
      dependsOn(verifyLocalBuild)
      archiveClassifier.set(classifier)
      destinationDirectory.set(layout.buildDirectory.dir("libs"))

      if (install != null) {
        val resourcePath = "META-INF/maplibre-native-ffi/$classifier"
        // A build tree's install also holds the test graphics fixtures, which the
        // runtime leaves out.
        from(install.resolve("lib")) {
          include("*.so", "*.so.*", "*.dylib")
          exclude("*mln_test_graphics*")
          into(resourcePath)
        }
        from(install.resolve("bin")) {
          include("*.dll")
          exclude("*mln_test_graphics*")
          into(resourcePath)
        }
        from(install.resolve("share/maplibre-native-c/licenses")) {
          into("META-INF/licenses/maplibre-native-c")
        }
      }
      from(rootProject.file("LICENSE")) { into("META-INF") }
    }
  }

  plugins.withId("maven-publish") {
    extensions.configure<PublishingExtension> {
      publications
        .withType<MavenPublication>()
        .matching { it.name == "jvm" }
        .configureEach { runtimeJars.forEach(::artifact) }
    }
  }
}

fun configureAndroidRuntimePublication(backend: MaplibreRuntimeBackend) {
  val selectedBackend =
    providers.gradleProperty("maplibre.android.backend").orElse(AndroidTarget.DEFAULT_BACKEND)
  val verifyBackend =
    tasks.register<VerifyAndroidRuntimeBackend>("verifyAndroidRuntimeBackend") {
      this.selectedBackend.set(selectedBackend)
      expectedBackend.set(backend.id)
    }

  // The binding project checks the local build of each ABI before it packages the libraries.
  tasks.configureEach {
    if (name == "preAndroidMainBuild" || name == "mergeAndroidMainJniLibFolders") {
      dependsOn(verifyBackend, ":bindings:kotlin:packageAndroidRuntimeLibraries")
    }
  }

  val androidTargets =
    AndroidTarget.parseAbis(
      providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS),
    )
  val referenceInstall = runtimeInstall(androidTargets.first().targetPlatform, backend.id) ?: return
  // Resolve the NOTICE from the SDK Gradle selected, which is the same SDK whose
  // NDK supplies the statically linked libc++. Reading ANDROID_HOME directly
  // would pair the library with a notice from a different NDK whenever
  // local.properties or ANDROID_SDK_ROOT selects another SDK.
  val androidNdkVersion = requiredEnvironmentVariable("MLN_FFI_ANDROID_NDK_VERSION")
  val androidNdkNotice =
    extensions
      .getByType<KotlinMultiplatformAndroidComponentsExtension>()
      .sdkComponents
      .sdkDirectory
      .map { it.file("ndk/$androidNdkVersion/NOTICE") }
  val licenseDirectory = referenceInstall.resolve("share/maplibre-native-c/licenses")
  val headerDirectory = referenceInstall.resolve("include")

  // Prefab doesn't read jni/, so the Prefab module carries its own copy of each
  // library, taken from the directories that the AAR's jni/ entries come from.
  // The module declares the STL none because Prefab rejects a shared library that
  // declares c++_static, and this library exports only mln_* symbols.
  val prefabModule = "modules/maplibre-native-c"
  val prefabAbis = androidTargets.map(AndroidTarget::ndkAbi)
  val prefabMinSdk = catalogVersionInt("android-minSdk")
  val prefabNdkMajorVersion = androidNdkVersion.substringBefore('.').toInt()
  val runtimeLibraries =
    project(":bindings:kotlin").layout.buildDirectory.dir("generated/jniLibs/runtime/${backend.id}")
  val generatePrefabPackage =
    tasks.register<Sync>("generateAndroidPrefabPackage") {
      dependsOn(":bindings:kotlin:packageAndroidRuntimeLibraries")
      inputs.property("minSdk", prefabMinSdk)
      inputs.property("ndkMajorVersion", prefabNdkMajorVersion)
      into(layout.buildDirectory.dir("generated/prefab/androidMain"))
      from(headerDirectory) { into("$prefabModule/include") }
      androidTargets.forEach { target ->
        from(runtimeLibraries.map { it.dir("${target.cargoTarget}/${target.ndkAbi}") }) {
          include("libmaplibre-native-c.so")
          into("$prefabModule/libs/android.${target.ndkAbi}")
        }
      }
      doLast {
        val module = destinationDir.resolve(prefabModule)
        destinationDir
          .resolve("prefab.json")
          .writeText(
            """{"schema_version": 2, "name": "maplibre-native-c", "dependencies": []}""" + "\n",
          )
        module
          .resolve("module.json")
          .writeText(
            """{"export_libraries": [], "library_name": "libmaplibre-native-c", "android": {}}""" +
              "\n",
          )
        prefabAbis.forEach { abi ->
          module
            .resolve("libs/android.$abi/abi.json")
            .writeText(
              """{"abi": "$abi", "api": $prefabMinSdk, "ndk": $prefabNdkMajorVersion, """ +
                """"stl": "none", "static": false}""" +
                "\n",
            )
        }
      }
    }

  tasks.withType<Zip>().configureEach {
    if (name == "bundleAndroidMainAar") {
      dependsOn(":bindings:kotlin:androidSdkPackages")
      // A missing directory would otherwise be skipped silently and ship an AAR
      // with no native notices or Prefab headers.
      doFirst {
        check(licenseDirectory.isDirectory) {
          "Native license notices are missing at $licenseDirectory; the install " +
            "this AAR is assembled from predates the license bundle."
        }
        check(headerDirectory.isDirectory) {
          "C headers are missing at $headerDirectory; the Prefab module needs them."
        }
      }
      from(licenseDirectory) { into("META-INF/licenses/maplibre-native-c") }
      from(androidNdkNotice) { into("META-INF/licenses/android-ndk") }
      from(generatePrefabPackage) { into("prefab") }
    }
  }
}

val runtime = extensions.getByType<MaplibreRuntimeConvention>()
val backend = runtime.backend
val runtimeBackend = backend.id
val mavenGroup = providers.gradleProperty("maplibre.maven.group").get()
val mavenVersion = providers.gradleProperty("maplibre.maven.version").get()
val mavenArtifact = "maplibre-native-ffi-runtime-${backend.id}"

val runtimeInteropDirectory =
  rootProject.layout.projectDirectory.dir("bindings/kotlin/runtimes/common")
val configuredNativeTargets = nativeTargets(backend)
val jvmClassifiers =
  if (backend == MaplibreRuntimeBackend.METAL) {
    listOf("natives-macos-arm64")
  } else {
    HostPlatform.JVM_TARGET_PLATFORMS.keys
  }

extensions.configure<KotlinMultiplatformExtension> {
  sourceSets.getByName("commonMain") {
    kotlin.srcDir(rootProject.file("bindings/kotlin/runtimes/common/src/commonMain/kotlin"))
  }

  targets.withType<KotlinNativeTarget>().configureEach {
    val targetConfiguration =
      requireNotNull(configuredNativeTargets[name]) {
        "Unsupported ${backend.displayName} Kotlin/Native runtime target: $name"
      }
    val targetPlatform = kotlinNativeTargetPlatform(name)
    val verifyLocalBuild =
      registerLocalBuildVerification(
        "verify${name.capitalized()}LocalBuild",
        targetPlatform,
        runtimeBackend,
      )
    val runtimeInstallDir = runtimeInstall(targetPlatform, runtimeBackend)
    val staticLibraryOptions =
      targetConfiguration.staticLibraries.flatMap { listOf("-staticLibrary", it) }.toTypedArray()

    val runtimeInterop =
      compilations.getByName("main").cinterops.create("maplibreNativeRuntime") {
        defFile(runtimeInteropDirectory.file(targetConfiguration.definitionFileName))
        includeDirs.headerFilterOnly(runtimeInteropDirectory.asFile)
        compilerOpts("-I${runtimeInteropDirectory.asFile}")
        if (runtimeInstallDir != null) {
          extraOpts(
            "-libraryPath",
            runtimeInstallDir.resolve("lib").absolutePath,
            *staticLibraryOptions,
          )
        }
      }

    tasks.named<CInteropProcess>(runtimeInterop.interopProcessingTaskName) {
      dependsOn(verifyLocalBuild)
      if (runtimeInstallDir != null) {
        // Track archives that cinterop receives through -staticLibrary.
        inputs
          .files(targetConfiguration.staticLibraries.map { runtimeInstallDir.resolve("lib/$it") })
          .withPropertyName("maplibreNativeCArchives")
        embedMaplibreLicenseBundle(runtimeInstallDir.resolve("share/maplibre-native-c/licenses"))
      }
    }
  }
}

configureJvmRuntimeArtifacts(runtimeBackend, jvmClassifiers)

if (backend != MaplibreRuntimeBackend.METAL) {
  configureAndroidRuntimePublication(backend)
}

extensions.configure<MavenPublishBaseExtension> {
  coordinates(mavenGroup, mavenArtifact, mavenVersion)
  publishToMavenCentral()
  pom {
    name.set("MapLibre Native FFI ${backend.displayName} runtime")
    description.set(
      "Native ${backend.displayName} runtime for the MapLibre Native FFI Kotlin binding.",
    )
  }
}

canonicalizeKmpRootMetadata(
  group = mavenGroup,
  version = mavenVersion,
  targetModules =
    when (backend) {
      MaplibreRuntimeBackend.OPENGL,
      MaplibreRuntimeBackend.VULKAN ->
        buildMap {
          put("android", "$mavenArtifact-android")
          AndroidTarget.entries.forEach { target ->
            put(
              target.kotlinNativeTarget,
              "$mavenArtifact-${target.kotlinNativeTarget.lowercase()}",
            )
          }
          put("jvm", "$mavenArtifact-jvm")
          put("linuxArm64", "$mavenArtifact-linuxarm64")
          put("linuxX64", "$mavenArtifact-linuxx64")
          put("macosArm64", "$mavenArtifact-macosarm64")
        }

      MaplibreRuntimeBackend.METAL ->
        mapOf(
          "iosArm64" to "$mavenArtifact-iosarm64",
          "iosSimulatorArm64" to "$mavenArtifact-iossimulatorarm64",
          "tvosArm64" to "$mavenArtifact-tvosarm64",
          "tvosSimulatorArm64" to "$mavenArtifact-tvossimulatorarm64",
          "jvm" to "$mavenArtifact-jvm",
          "macosArm64" to "$mavenArtifact-macosarm64",
        )
    },
)
