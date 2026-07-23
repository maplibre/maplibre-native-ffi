import com.vanniktech.maven.publish.MavenPublishBaseExtension
import java.io.File
import org.gradle.api.provider.Provider
import org.gradle.api.publish.PublishingExtension
import org.gradle.api.publish.maven.MavenPublication
import org.gradle.jvm.tasks.Jar
import org.jetbrains.kotlin.gradle.dsl.KotlinMultiplatformExtension
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact
import org.maplibre.nativeffi.gradle.MaplibreRuntimeBackend
import org.maplibre.nativeffi.gradle.MaplibreRuntimeConvention
import org.maplibre.nativeffi.gradle.MaplibreRuntimeTargetFamily
import org.maplibre.nativeffi.gradle.VerifyAndroidRuntimeBackend
import org.maplibre.nativeffi.gradle.VerifyMaplibreRuntimeInstall
import org.maplibre.nativeffi.gradle.canonicalizeKmpRootMetadata

data class NativeTargetConfiguration(val definitionFileName: String, val zigTarget: String)

data class JvmRuntimeInstall(
  val classifier: String,
  val propertyName: String,
  val installDirectory: File,
  val explicitlyConfigured: Boolean,
)

val jvmZigTargets =
  linkedMapOf(
    "natives-linux-x64" to "x86_64-linux-gnu",
    "natives-linux-arm64" to "aarch64-linux-gnu",
    "natives-macos-arm64" to "aarch64-macos",
    "natives-windows-x64" to "x86_64-windows-msvc",
    "natives-windows-arm64" to "aarch64-windows-msvc",
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
        "Metal does not support Linux Kotlin/Native targets"
      }
      mapOf(
        "linuxX64" to NativeTargetConfiguration("linux-${backend.id}.def", "x86_64-linux-gnu"),
        "linuxArm64" to NativeTargetConfiguration("linux-${backend.id}.def", "aarch64-linux-gnu"),
      )
    }
    MaplibreRuntimeTargetFamily.APPLE -> {
      require(backend == MaplibreRuntimeBackend.METAL) {
        "Only Metal supports Apple Kotlin/Native targets"
      }
      mapOf(
        "iosArm64" to NativeTargetConfiguration("ios-metal.def", "aarch64-ios"),
        "iosSimulatorArm64" to NativeTargetConfiguration("ios-metal.def", "aarch64-ios-simulator"),
        "macosArm64" to NativeTargetConfiguration("macos-metal.def", "aarch64-macos"),
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
  zigTarget: String,
) =
  tasks.register<VerifyMaplibreRuntimeInstall>(taskName) {
    installDirectoryPath.set(installDirectory.map(File::getAbsolutePath))
    this.installPropertyName.set(installPropertyName)
    this.explicitlyConfigured.set(explicitlyConfigured)
    this.requireExplicitInput.set(requireExplicitInput)
    expectedBackend.set(backend)
    expectedZigTarget.set(zigTarget)
  }

fun configureJvmRuntimeArtifacts(
  backend: String,
  classifierZigTargets: Map<String, String>,
  maplibreNativeC: MaplibreNativeCArtifact,
) {
  val hostClassifier = HostPlatform.current().maplibreNativeClassifier
  val configuredInstalls = classifierZigTargets.mapNotNull { (classifier, _) ->
    val propertyName = "maplibre.runtime.$backend.jvm.$classifier.installDir"
    providers.gradleProperty(propertyName).orNull?.let {
      JvmRuntimeInstall(classifier, propertyName, rootProject.file(it), true)
    }
  }
  val runtimeInstalls = configuredInstalls.ifEmpty {
    if (hostClassifier in classifierZigTargets) {
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
        zigTarget = classifierZigTargets.getValue(runtimeInstall.classifier),
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
      from(rootProject.file("LICENSE")) { into("META-INF") }
    }
  }

  val verifyPublicationInputs = tasks.register("verifyJvmRuntimePublicationInputs")
  classifierZigTargets.forEach { (classifier, zigTarget) ->
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
        zigTarget = zigTarget,
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

fun configureAndroidRuntimePublication(backend: MaplibreRuntimeBackend) {
  val selectedBackend =
    providers.gradleProperty("maplibre.android.backend").orElse(AndroidTarget.DEFAULT_BACKEND)
  val verifyBackend =
    tasks.register<VerifyAndroidRuntimeBackend>("verifyAndroidRuntimeBackend") {
      this.selectedBackend.set(selectedBackend)
      expectedBackend.set(backend.id)
    }

  tasks.configureEach {
    if (name == "preAndroidMainBuild" || name == "mergeAndroidMainJniLibFolders") {
      dependsOn(verifyBackend, ":bindings:kotlin:packageAndroidNativeLibraries")
    }
  }

  val prebuiltInstallRoot = providers.gradleProperty("maplibre.android.prebuiltInstallRoot")
  val verifyPublicationInputs =
    tasks.register("verifyAndroidRuntimePublicationInputs") { dependsOn(verifyBackend) }
  val androidTargets =
    AndroidTarget.parseAbis(
      providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS)
    )
  androidTargets.forEach { target ->
    val preset = target.cmakePreset(backend.id)
    val propertyName = "maplibre.android.prebuiltInstallRoot"
    val selectedInstall =
      prebuiltInstallRoot
        .map { rootProject.file(it).resolve(preset) }
        .getOrElse(rootProject.file("build/$preset/install"))
    val verifyInput =
      registerRuntimeInstallVerification(
        taskName = "verifyAndroid${target.taskSuffix}RuntimePublicationInput",
        installDirectory = providers.provider { selectedInstall },
        installPropertyName = propertyName,
        explicitlyConfigured = prebuiltInstallRoot.isPresent,
        requireExplicitInput = true,
        backend = backend.id,
        zigTarget = target.cargoTarget,
      )
    verifyPublicationInputs.configure { dependsOn(verifyInput) }
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
  rootProject.layout.projectDirectory.dir("bindings/kotlin-runtime-common")
val configuredNativeTargets = nativeTargets(backend, targetFamily)
val configuredJvmZigTargets =
  if (targetFamily == MaplibreRuntimeTargetFamily.APPLE) {
    jvmZigTargets.filterKeys { it == "natives-macos-arm64" }
  } else {
    jvmZigTargets
  }

extensions.configure<KotlinMultiplatformExtension> {
  sourceSets.getByName("commonMain") {
    kotlin.srcDir(rootProject.file("bindings/kotlin-runtime-common/src/commonMain/kotlin"))
    dependencies { api(project(":bindings:kotlin")) }
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

    compilations.getByName("main") {
      cinterops.create("maplibreNativeRuntime") {
        defFile(runtimeInteropDirectory.file(targetConfiguration.definitionFileName))
        includeDirs.headerFilterOnly(runtimeInteropDirectory.asFile)
        compilerOpts("-I${runtimeInteropDirectory.asFile}")
        extraOpts(
          "-libraryPath",
          runtimeInstallDir.resolve("lib").absolutePath,
          "-staticLibrary",
          "libmaplibre-native-c.a",
        )
      }
    }

    val verifyPublicationInput =
      registerRuntimeInstallVerification(
        taskName = "verify${nativeTargetName.capitalized()}RuntimePublicationInput",
        installDirectory = providers.provider { runtimeInstallDir },
        installPropertyName = installPropertyName,
        explicitlyConfigured = configuredInstall.isPresent,
        requireExplicitInput = true,
        backend = runtimeBackend,
        zigTarget = targetConfiguration.zigTarget,
      )
    tasks.configureEach {
      if (name.startsWith("publish${nativeTargetName.capitalized()}PublicationTo")) {
        dependsOn(verifyPublicationInput)
      }
    }
  }
}

configureJvmRuntimeArtifacts(runtimeBackend, configuredJvmZigTargets, maplibreNativeC)

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
