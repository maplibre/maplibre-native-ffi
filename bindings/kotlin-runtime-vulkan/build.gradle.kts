import org.gradle.api.publish.PublishingExtension
import org.gradle.api.publish.maven.MavenPublication
import org.gradle.jvm.tasks.Jar
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.dsl.KotlinMultiplatformExtension
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact
import org.maplibre.nativeffi.gradle.VerifyAndroidRuntimeBackend

plugins {
  alias(libs.plugins.kotlin.multiplatform)
  alias(libs.plugins.android.kotlin.multiplatform.library)
  alias(libs.plugins.maven.publish)
}

extensions.extraProperties["maplibreRuntimeBackend"] = "vulkan"

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

val hostPlatform = HostPlatform.current()
val androidTargets =
  AndroidTarget.parseAbis(
    providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS)
  )
val packagedAndroidNativeLibs =
  project(":bindings:kotlin").layout.buildDirectory.dir("generated/jniLibs/androidMain")

kotlin {
  jvm { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

  linuxX64()
  linuxArm64()

  android {
    namespace = "org.maplibre.nativeffi.runtime.vulkan"
    compileSdk = libs.versions.android.compileSdk.get().toInt()
    minSdk = libs.versions.android.minSdk.get().toInt()
  }

  sourceSets {
    commonMain {
      kotlin.srcDir(rootProject.file("bindings/kotlin-runtime-common/src/commonMain/kotlin"))
      dependencies { api(project(":bindings:kotlin")) }
    }
  }
}

androidComponents {
  onVariants { variant ->
    androidTargets.forEach { target ->
      variant.sources.jniLibs?.addStaticSourceDirectory(
        packagedAndroidNativeLibs.get().dir("vulkan/${target.cargoTarget}").asFile.absolutePath
      )
    }
  }
}

val verifyAndroidRuntimeBackend =
  tasks.register<VerifyAndroidRuntimeBackend>("verifyAndroidRuntimeBackend") {
    selectedBackend.set(
      providers.gradleProperty("maplibre.android.backend").orElse(AndroidTarget.DEFAULT_BACKEND)
    )
    expectedBackend.set("vulkan")
  }

tasks.configureEach {
  if (name == "preAndroidMainBuild" || name == "mergeAndroidMainJniLibFolders") {
    dependsOn(verifyAndroidRuntimeBackend, ":bindings:kotlin:packageAndroidNativeLibraries")
  }
}

mavenPublishing {
  coordinates(
    groupId = providers.gradleProperty("maplibre.maven.group").get(),
    artifactId = "maplibre-native-ffi-runtime-vulkan",
    version = providers.gradleProperty("maplibre.maven.version").get(),
  )
  publishToMavenCentral()
  pom {
    name.set("MapLibre Native FFI Vulkan runtime")
    description.set("Native Vulkan runtime for the MapLibre Native FFI Kotlin binding.")
  }
}

val runtimeBackend = requireNotNull(extensions.extraProperties["maplibreRuntimeBackend"] as? String)
val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val runtimeInteropDirectory =
  rootProject.layout.projectDirectory.dir("bindings/kotlin-runtime-common")

extensions.configure<KotlinMultiplatformExtension> {
  targets.withType<KotlinNativeTarget>().configureEach {
    val nativeTargetName = name
    val runtimeInstallDir =
      providers
        .gradleProperty("maplibre.runtime.$runtimeBackend.$nativeTargetName.installDir")
        .map(rootProject::file)
        .getOrElse(maplibreNativeC.installDir)
    val runtimeLibraryDirectory = runtimeInstallDir.resolve("lib")

    compilations.getByName("main") {
      cinterops.create("maplibreNativeRuntime") {
        defFile(runtimeInteropDirectory.file("linux-vulkan.def"))
        includeDirs.headerFilterOnly(runtimeInteropDirectory.asFile)
        compilerOpts("-I${runtimeInteropDirectory.asFile}")
        extraOpts(
          "-libraryPath",
          runtimeLibraryDirectory.absolutePath,
          "-staticLibrary",
          "libmaplibre-native-c.a",
        )
      }
    }
  }
}

val supportedJvmClassifiers =
  when (runtimeBackend) {
    "metal" -> listOf("natives-macos-arm64")
    else ->
      listOf(
        "natives-linux-x64",
        "natives-linux-arm64",
        "natives-macos-arm64",
        "natives-windows-x64",
        "natives-windows-arm64",
      )
  }
val configuredJvmRuntimeInstalls = buildMap {
  supportedJvmClassifiers.forEach { classifier ->
    providers
      .gradleProperty("maplibre.runtime.$runtimeBackend.jvm.$classifier.installDir")
      .orNull
      ?.let { put(classifier, rootProject.file(it)) }
  }
  if (isEmpty()) {
    put(hostPlatform.maplibreNativeClassifier, maplibreNativeC.installDir)
  }
}
val nativeRuntimeJars = configuredJvmRuntimeInstalls.map { (classifier, installDir) ->
  val taskSuffix =
    classifier.split('-').joinToString("") { part -> part.replaceFirstChar(Char::uppercaseChar) }
  tasks.register<Jar>("jvm${taskSuffix}RuntimeJar") {
    group = "build"
    description = "Packages the $runtimeBackend native runtime for $classifier."
    archiveClassifier.set(classifier)
    destinationDirectory.set(layout.buildDirectory.dir("libs"))

    val resourcePath = "META-INF/maplibre-native-ffi/$classifier"
    from(installDir.resolve("lib")) {
      include("*.so", "*.so.*", "*.dylib")
      into(resourcePath)
    }
    from(installDir.resolve("bin")) {
      include("*.dll")
      into(resourcePath)
    }
    from(rootProject.file("LICENSE")) { into("META-INF") }
  }
}

plugins.withId("maven-publish") {
  extensions.configure<PublishingExtension> {
    publications
      .withType<MavenPublication>()
      .matching { it.name == "jvm" }
      .configureEach { nativeRuntimeJars.forEach(::artifact) }
  }
}
