import org.gradle.api.publish.PublishingExtension
import org.gradle.api.publish.maven.MavenPublication
import org.gradle.jvm.tasks.Jar
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.dsl.KotlinMultiplatformExtension
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins {
  alias(libs.plugins.kotlin.multiplatform)
  alias(libs.plugins.maven.publish)
}

extensions.extraProperties["maplibreRuntimeBackend"] = "metal"

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

val hostPlatform = HostPlatform.current()
val publishingSnapshot =
  providers.gradleProperty("maplibre.publish").map(String::toBoolean).getOrElse(false)

kotlin {
  jvm { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

  if (publishingSnapshot || hostPlatform.kotlinNativeTargetPresetName == "macosArm64") {
    macosArm64()
  }

  sourceSets {
    commonMain {
      kotlin.srcDir(rootProject.file("bindings/kotlin-runtime-common/src/commonMain/kotlin"))
      dependencies { api(project(":bindings:kotlin")) }
    }
  }
}

mavenPublishing {
  coordinates(
    groupId = providers.gradleProperty("maplibre.maven.group").get(),
    artifactId = "maplibre-native-ffi-runtime-metal",
    version = providers.gradleProperty("maplibre.maven.version").get(),
  )
  publishToMavenCentral()
  pom {
    name.set("MapLibre Native FFI Metal runtime")
    description.set("Native Metal runtime for the MapLibre Native FFI Kotlin binding.")
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
        defFile(runtimeInteropDirectory.file("macos-metal.def"))
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
