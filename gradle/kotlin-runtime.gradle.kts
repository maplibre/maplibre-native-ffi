import com.android.build.api.variant.KotlinMultiplatformAndroidComponentsExtension
import com.vanniktech.maven.publish.MavenPublishBaseExtension
import java.io.File
import org.gradle.api.provider.Provider
import org.gradle.api.publish.PublishingExtension
import org.gradle.api.publish.maven.MavenPublication
import org.gradle.api.tasks.bundling.Zip
import org.gradle.jvm.tasks.Jar
import org.jetbrains.kotlin.gradle.dsl.KotlinMultiplatformExtension
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.tasks.CInteropProcess
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.CargoPackage
import org.maplibre.nativeffi.gradle.ExtractAarClassesJar
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact
import org.maplibre.nativeffi.gradle.MaplibreRuntimeBackend
import org.maplibre.nativeffi.gradle.MaplibreRuntimeConvention
import org.maplibre.nativeffi.gradle.MaplibreRuntimeTargetFamily
import org.maplibre.nativeffi.gradle.VerifyAndroidRuntimeBackend
import org.maplibre.nativeffi.gradle.VerifyMaplibreRuntimeInstall
import org.maplibre.nativeffi.gradle.canonicalizeKmpRootMetadata
import org.maplibre.nativeffi.gradle.embedMaplibreLicenseBundle
import org.maplibre.nativeffi.gradle.requiredEnvironmentVariable

data class NativeTargetConfiguration(
  val definitionFileName: String,
  val targetPlatform: String,
  val staticLibraries: List<String> = listOf("libmaplibre-native-c.a"),
)

data class JvmRuntimeInstall(
  val classifier: String,
  val propertyName: String,
  val installDirectory: File,
  val explicitlyConfigured: Boolean,
)

val jvmTargetPlatforms =
  linkedMapOf(
    "natives-linux-x64" to "linux-x64",
    "natives-linux-arm64" to "linux-arm64",
    "natives-macos-arm64" to "macos-arm64",
    "natives-windows-x64" to "windows-x64",
    "natives-windows-arm64" to "windows-arm64",
  )

fun String.capitalized(): String = replaceFirstChar(Char::uppercaseChar)

fun String.taskSuffix(): String =
  split('-').joinToString("") { it.replaceFirstChar(Char::uppercaseChar) }

fun nativeTargets(
  backend: MaplibreRuntimeBackend,
  targetFamily: MaplibreRuntimeTargetFamily,
): Map<String, NativeTargetConfiguration> =
  when (targetFamily) {
    MaplibreRuntimeTargetFamily.LINUX -> {
      require(backend != MaplibreRuntimeBackend.METAL) {
        "Metal does not support the Linux runtime target family"
      }
      mapOf(
        "linuxArm64" to
          NativeTargetConfiguration(
            "linux-${backend.id}.def",
            "linux-arm64",
            listOf("libmaplibre-native-c.a", "libmln_ffi_platform.a"),
          ),
        "linuxX64" to
          NativeTargetConfiguration(
            "linux-${backend.id}.def",
            "linux-x64",
            listOf("libmaplibre-native-c.a", "libmln_ffi_platform.a"),
          ),
      )
    }
    MaplibreRuntimeTargetFamily.APPLE -> {
      require(backend == MaplibreRuntimeBackend.METAL) {
        "Only Metal supports Apple Kotlin/Native targets"
      }
      mapOf(
        "iosArm64" to NativeTargetConfiguration("ios-metal.def", "ios-arm64"),
        "iosSimulatorArm64" to NativeTargetConfiguration("ios-metal.def", "ios-simulator-arm64"),
        "macosArm64" to NativeTargetConfiguration("macos-metal.def", "macos-arm64"),
      )
    }
  }

fun registerRuntimeInstallVerification(
  taskName: String,
  installDirectory: Provider<File>,
  installPropertyName: String,
  explicitlyConfigured: Boolean,
  requireExplicitInput: Boolean,
  backend: String,
  targetPlatform: String,
) =
  tasks.register<VerifyMaplibreRuntimeInstall>(taskName) {
    installDirectoryPath.set(installDirectory.map(File::getAbsolutePath))
    this.installPropertyName.set(installPropertyName)
    this.explicitlyConfigured.set(explicitlyConfigured)
    this.requireExplicitInput.set(requireExplicitInput)
    expectedBackend.set(backend)
    expectedTargetPlatform.set(targetPlatform)
  }

fun configureJvmRuntimeArtifacts(
  backend: String,
  classifierTargetPlatforms: Map<String, String>,
  maplibreNativeC: MaplibreNativeCArtifact,
) {
  val hostClassifier = HostPlatform.current().maplibreNativeClassifier
  val configuredInstalls = classifierTargetPlatforms.mapNotNull { (classifier, _) ->
    val propertyName = "maplibre.runtime.$backend.jvm.$classifier.installDir"
    providers.gradleProperty(propertyName).orNull?.let {
      JvmRuntimeInstall(classifier, propertyName, rootProject.file(it), true)
    }
  }
  val runtimeInstalls = configuredInstalls.ifEmpty {
    if (hostClassifier in classifierTargetPlatforms) {
      listOf(
        JvmRuntimeInstall(
          hostClassifier,
          "maplibre.runtime.$backend.jvm.$hostClassifier.installDir",
          maplibreNativeC.installDir,
          false,
        )
      )
    } else {
      emptyList()
    }
  }

  val runtimeJars = runtimeInstalls.map { runtimeInstall ->
    val taskSuffix = runtimeInstall.classifier.taskSuffix()
    val verifyRuntime =
      registerRuntimeInstallVerification(
        taskName = "verifyJvm${taskSuffix}RuntimeArtifact",
        installDirectory = providers.provider { runtimeInstall.installDirectory },
        installPropertyName = runtimeInstall.propertyName,
        explicitlyConfigured = runtimeInstall.explicitlyConfigured,
        requireExplicitInput = false,
        backend = backend,
        targetPlatform = classifierTargetPlatforms.getValue(runtimeInstall.classifier),
      )

    tasks.register<Jar>("jvm${taskSuffix}RuntimeJar") {
      group = "build"
      description = "Packages the $backend native runtime for ${runtimeInstall.classifier}."
      dependsOn(verifyRuntime)
      archiveClassifier.set(runtimeInstall.classifier)
      destinationDirectory.set(layout.buildDirectory.dir("libs"))

      val resourcePath = "META-INF/maplibre-native-ffi/${runtimeInstall.classifier}"
      from(runtimeInstall.installDirectory.resolve("lib")) {
        include("*.so", "*.so.*", "*.dylib")
        into(resourcePath)
      }
      from(runtimeInstall.installDirectory.resolve("bin")) {
        include("*.dll")
        into(resourcePath)
      }
      from(runtimeInstall.installDirectory.resolve("share/maplibre-native-c/licenses")) {
        into("META-INF/licenses/maplibre-native-c")
      }
      from(rootProject.file("LICENSE")) { into("META-INF") }
    }
  }

  val verifyPublicationInputs = tasks.register("verifyJvmRuntimePublicationInputs")
  classifierTargetPlatforms.forEach { (classifier, targetPlatform) ->
    val propertyName = "maplibre.runtime.$backend.jvm.$classifier.installDir"
    val configuredInstall = providers.gradleProperty(propertyName)
    val selectedInstall =
      configuredInstall.map(rootProject::file).getOrElse(maplibreNativeC.installDir)
    val verifyInput =
      registerRuntimeInstallVerification(
        taskName = "verifyJvm${classifier.taskSuffix()}RuntimePublicationInput",
        installDirectory = providers.provider { selectedInstall },
        installPropertyName = propertyName,
        explicitlyConfigured = configuredInstall.isPresent,
        requireExplicitInput = true,
        backend = backend,
        targetPlatform = targetPlatform,
      )
    verifyPublicationInputs.configure { dependsOn(verifyInput) }
  }

  tasks.configureEach {
    if (name.startsWith("publishJvmPublicationTo")) {
      dependsOn(verifyPublicationInputs)
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

// The Rustls platform verifier reaches the JVM trust policy that Android TLS
// needs. The native library calls it over JNI under a name the Rust JNI
// descriptors hard-code, and nothing in this module references it, so it travels
// with the library it serves rather than with any one language binding.
fun embedRustlsPlatformVerifier() {
  val verifierAar =
    project(":bindings:rustls-platform-verifier-android")
      .layout
      .buildDirectory
      .file("outputs/aar/rustls-platform-verifier-android-release.aar")
  val verifierJar =
    layout.buildDirectory.file(
      "generated/dependencies/rustlsPlatformVerifierAndroid/rustls-platform-verifier-android.jar"
    )
  val extractVerifierJar =
    tasks.register<ExtractAarClassesJar>("extractRustlsPlatformVerifierAndroidJar") {
      dependsOn(":bindings:rustls-platform-verifier-android:bundleReleaseAar")
      aarFile.set(verifierAar)
      outputJar.set(verifierJar)
    }

  extensions.configure<KotlinMultiplatformExtension> {
    sourceSets.getByName("androidMain") {
      dependencies { implementation(files(verifierJar).builtBy(extractVerifierJar)) }
    }
  }

  val verifierPackage = CargoPackage.directory(project, "rustls-platform-verifier")
  tasks.withType<Zip>().configureEach {
    if (name == "bundleAndroidMainAar") {
      val licenseDirectory = "META-INF/licenses/rustls-platform-verifier"
      from(verifierPackage.map { it.resolve("LICENSE-APACHE") }) { into(licenseDirectory) }
      from(verifierPackage.map { it.resolve("LICENSE-MIT") }) { into(licenseDirectory) }
      from(rootProject.file("patches/rustls-platform-verifier/NOTICE")) { into(licenseDirectory) }
    }
  }
}

fun configureAndroidRuntimePublication(backend: MaplibreRuntimeBackend) {
  embedRustlsPlatformVerifier()

  val selectedBackend =
    providers.gradleProperty("maplibre.android.backend").orElse(AndroidTarget.DEFAULT_BACKEND)
  val verifyBackend =
    tasks.register<VerifyAndroidRuntimeBackend>("verifyAndroidRuntimeBackend") {
      this.selectedBackend.set(selectedBackend)
      expectedBackend.set(backend.id)
    }

  tasks.configureEach {
    if (name == "preAndroidMainBuild" || name == "mergeAndroidMainJniLibFolders") {
      dependsOn(verifyBackend, ":bindings:kotlin:packageAndroidRuntimeLibraries")
    }
  }

  val prebuiltInstallRoot = providers.gradleProperty("maplibre.android.prebuiltInstallRoot")
  val prebuiltBuildRoot = providers.gradleProperty("maplibre.android.prebuiltBuildRoot")
  check(!(prebuiltInstallRoot.isPresent && prebuiltBuildRoot.isPresent)) {
    "Configure only one of maplibre.android.prebuiltInstallRoot and maplibre.android.prebuiltBuildRoot"
  }
  val verifyPublicationInputs =
    tasks.register("verifyAndroidRuntimePublicationInputs") { dependsOn(verifyBackend) }
  val androidTargets =
    AndroidTarget.parseAbis(
      providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS)
    )
  val selectedInstalls = mutableListOf<File>()
  androidTargets.forEach { target ->
    val preset = target.cmakePreset(backend.id)
    val propertyName =
      if (prebuiltBuildRoot.isPresent) {
        "maplibre.android.prebuiltBuildRoot"
      } else {
        "maplibre.android.prebuiltInstallRoot"
      }
    val selectedInstall =
      prebuiltBuildRoot
        .map { rootProject.file(it).resolve(preset).resolve("install") }
        .orElse(prebuiltInstallRoot.map { rootProject.file(it).resolve(preset) })
        .getOrElse(rootProject.file("build/$preset/install"))
    selectedInstalls.add(selectedInstall)
    val verifyInput =
      registerRuntimeInstallVerification(
        taskName = "verifyAndroid${target.taskSuffix}RuntimePublicationInput",
        installDirectory = providers.provider { selectedInstall },
        installPropertyName = propertyName,
        explicitlyConfigured = prebuiltInstallRoot.isPresent || prebuiltBuildRoot.isPresent,
        requireExplicitInput = true,
        backend = backend.id,
        targetPlatform = target.targetPlatform,
      )
    verifyPublicationInputs.configure { dependsOn(verifyInput) }
  }

  val licenseInstall = selectedInstalls.first()
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
  val licenseDirectory = licenseInstall.resolve("share/maplibre-native-c/licenses")
  tasks.withType<Zip>().configureEach {
    if (name == "bundleAndroidMainAar") {
      // A missing directory would otherwise be skipped silently and ship an AAR
      // with no native notices at all.
      doFirst {
        check(licenseDirectory.isDirectory) {
          "Native license notices are missing at $licenseDirectory; the install " +
            "this AAR is assembled from predates the license bundle."
        }
      }
      from(licenseDirectory) { into("META-INF/licenses/maplibre-native-c") }
      from(androidNdkNotice) { into("META-INF/licenses/android-ndk") }
    }
  }

  tasks.configureEach {
    if (name.startsWith("publishAndroidPublicationTo")) {
      dependsOn(verifyPublicationInputs)
    }
  }
}

val runtime = extensions.getByType<MaplibreRuntimeConvention>()
val backend = runtime.backend
val targetFamily = runtime.targetFamily
val runtimeBackend = backend.id
val mavenGroup = providers.gradleProperty("maplibre.maven.group").get()
val mavenVersion = providers.gradleProperty("maplibre.maven.version").get()
val mavenArtifact = "maplibre-native-ffi-runtime-${backend.id}"

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val runtimeInteropDirectory =
  rootProject.layout.projectDirectory.dir("bindings/kotlin/runtimes/common")
val configuredNativeTargets = nativeTargets(backend, targetFamily)
val configuredJvmTargetPlatforms =
  if (targetFamily == MaplibreRuntimeTargetFamily.APPLE) {
    jvmTargetPlatforms.filterKeys { it == "natives-macos-arm64" }
  } else {
    jvmTargetPlatforms
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
    val nativeTargetName = name
    val installPropertyName = "maplibre.runtime.$runtimeBackend.$nativeTargetName.installDir"
    val configuredInstall = providers.gradleProperty(installPropertyName)
    val runtimeInstallDir =
      configuredInstall.map(rootProject::file).getOrElse(maplibreNativeC.installDir)
    val runtimeArchives =
      targetConfiguration.staticLibraries.map { runtimeInstallDir.resolve("lib/$it") }
    val staticLibraryOptions =
      targetConfiguration.staticLibraries.flatMap { listOf("-staticLibrary", it) }.toTypedArray()

    val runtimeInterop =
      compilations.getByName("main").cinterops.create("maplibreNativeRuntime") {
        defFile(runtimeInteropDirectory.file(targetConfiguration.definitionFileName))
        includeDirs.headerFilterOnly(runtimeInteropDirectory.asFile)
        compilerOpts("-I${runtimeInteropDirectory.asFile}")
        extraOpts(
          "-libraryPath",
          runtimeInstallDir.resolve("lib").absolutePath,
          *staticLibraryOptions,
        )
      }

    val runtimeLicenseDirectory = runtimeInstallDir.resolve("share/maplibre-native-c/licenses")
    tasks.named<CInteropProcess>(runtimeInterop.interopProcessingTaskName) {
      // Track archives that cinterop receives through -staticLibrary.
      inputs.files(runtimeArchives).withPropertyName("maplibreNativeCArchives")
      embedMaplibreLicenseBundle(runtimeLicenseDirectory)
    }

    val verifyPublicationInput =
      registerRuntimeInstallVerification(
        taskName = "verify${nativeTargetName.capitalized()}RuntimePublicationInput",
        installDirectory = providers.provider { runtimeInstallDir },
        installPropertyName = installPropertyName,
        explicitlyConfigured = configuredInstall.isPresent,
        requireExplicitInput = true,
        backend = runtimeBackend,
        targetPlatform = targetConfiguration.targetPlatform,
      )
    tasks.configureEach {
      if (name.startsWith("publish${nativeTargetName.capitalized()}PublicationTo")) {
        dependsOn(verifyPublicationInput)
      }
    }
  }
}

configureJvmRuntimeArtifacts(runtimeBackend, configuredJvmTargetPlatforms, maplibreNativeC)

if (targetFamily == MaplibreRuntimeTargetFamily.LINUX) {
  configureAndroidRuntimePublication(backend)
}

extensions.configure<MavenPublishBaseExtension> {
  coordinates(mavenGroup, mavenArtifact, mavenVersion)
  publishToMavenCentral()
  pom {
    name.set("MapLibre Native FFI ${backend.displayName} runtime")
    description.set(
      "Native ${backend.displayName} runtime for the MapLibre Native FFI Kotlin binding."
    )
  }
}

canonicalizeKmpRootMetadata(
  group = mavenGroup,
  version = mavenVersion,
  targetModules =
    when (targetFamily) {
      MaplibreRuntimeTargetFamily.LINUX ->
        mapOf(
          "android" to "$mavenArtifact-android",
          "jvm" to "$mavenArtifact-jvm",
          "linuxArm64" to "$mavenArtifact-linuxarm64",
          "linuxX64" to "$mavenArtifact-linuxx64",
        )
      MaplibreRuntimeTargetFamily.APPLE ->
        mapOf(
          "iosArm64" to "$mavenArtifact-iosarm64",
          "iosSimulatorArm64" to "$mavenArtifact-iossimulatorarm64",
          "jvm" to "$mavenArtifact-jvm",
          "macosArm64" to "$mavenArtifact-macosarm64",
        )
    },
)
