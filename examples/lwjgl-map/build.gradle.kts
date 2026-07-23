import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.compile.JavaCompile
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

plugins {
  application
  id("org.jetbrains.kotlin.jvm")
}

apply(from = rootProject.file("gradle/native-artifact.gradle.kts"))

repositories {
  providers.gradleProperty("maplibre.maven.localRepository").orNull?.let {
    maven { url = rootProject.uri(it) }
  }
  mavenCentral()
}

val hostPlatform = HostPlatform.current()
val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val lwjglNative = hostPlatform.lwjglNativeClassifier
val usePublishedKotlin =
  providers.gradleProperty("maplibre.usePublishedKotlin").map(String::toBoolean).getOrElse(false)
val maplibrePublicationVersion = providers.gradleProperty("maplibre.maven.version").get()
val maplibreRuntimeBackend =
  providers.gradleProperty("maplibre.runtime.backend").getOrElse("vulkan")
val lwjglMapJvmArgs = buildList {
  add("--enable-native-access=ALL-UNNAMED")
  if (hostPlatform.isMac) {
    add("-XstartOnFirstThread")
  }
}

kotlin { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

application {
  mainClass = "org.maplibre.nativeffi.examples.lwjglmap.Main"
  applicationDefaultJvmArgs = lwjglMapJvmArgs
}

dependencies {
  if (usePublishedKotlin) {
    implementation("org.maplibre.nativeffi:maplibre-native-ffi-jvm:$maplibrePublicationVersion")
    runtimeOnly(
      "org.maplibre.nativeffi:maplibre-native-ffi-runtime-$maplibreRuntimeBackend-jvm:" +
        "$maplibrePublicationVersion:${hostPlatform.maplibreNativeClassifier}"
    )
  } else {
    implementation(project(":bindings:kotlin"))
  }
  implementation(platform(libs.lwjgl.bom))
  implementation(libs.lwjgl)
  implementation(libs.lwjgl.egl)
  implementation(libs.lwjgl.glfw)
  implementation(libs.lwjgl.opengl)
  implementation(libs.lwjgl.opengles)
  implementation(libs.lwjgl.vulkan)
  implementation(libs.lwjgl.shaderc)
  runtimeOnly(variantOf(libs.lwjgl) { classifier(lwjglNative) })
  runtimeOnly(variantOf(libs.lwjgl.glfw) { classifier(lwjglNative) })
  runtimeOnly(variantOf(libs.lwjgl.opengl) { classifier(lwjglNative) })
  runtimeOnly(variantOf(libs.lwjgl.opengles) { classifier(lwjglNative) })
  runtimeOnly(variantOf(libs.lwjgl.shaderc) { classifier(lwjglNative) })
}

tasks.withType<JavaCompile>().configureEach {
  options.release = libs.versions.java.release.get().toInt()
}

val nativeLibraryPathProperty = "org.maplibre.nativeffi.library.path"
val nativeLibraryDirsProperty = "org.maplibre.nativeffi.library.dirs"
val nativeLibraryPath = maplibreNativeC.libraryPath
val nativeHostLibraryPath =
  maplibreNativeC.hostLibraryDirs.joinToString(File.pathSeparator) { it.absolutePath }
val nativeLoaderLibraryPath =
  maplibreNativeC.loaderLibraryDirs.joinToString(File.pathSeparator) { it.absolutePath }

tasks.withType<JavaExec>().configureEach {
  jvmArgs(lwjglMapJvmArgs)
  if (!usePublishedKotlin) {
    systemProperty("org.lwjgl.librarypath", nativeHostLibraryPath)
    systemProperty(nativeLibraryPathProperty, nativeLibraryPath.absolutePath)
    systemProperty(nativeLibraryDirsProperty, nativeLoaderLibraryPath)
    inputs.file(nativeLibraryPath).withPropertyName("maplibreNativeCLibrary")
    inputs
      .files(maplibreNativeC.loaderLibraryDirs)
      .withPropertyName("maplibreNativeCLoaderLibraryDirs")
    inputs.dir(maplibreNativeC.installDir).withPropertyName("maplibreNativeCInstallDir")
  }
}
