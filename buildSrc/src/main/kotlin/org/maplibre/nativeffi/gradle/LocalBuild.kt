package org.maplibre.nativeffi.gradle

import java.io.File
import org.gradle.api.Project
import org.gradle.api.tasks.TaskProvider

/**
 * The Gradle property that names the local build every native input comes from: an install prefix,
 * or a build root that holds `<preset>/install` for each preset a task needs.
 */
const val LOCAL_BUILD_PROPERTY = "maplibre.localBuild"

internal const val NATIVE_DESCRIPTOR_PATH = "share/maplibre-native-c/artifact.json"

/** The local build that [LOCAL_BUILD_PROPERTY] names, or null when it is unset. */
val Project.localBuild: File?
  get() = providers.gradleProperty(LOCAL_BUILD_PROPERTY).map(rootProject::file).orNull

/** The install prefix in [localBuild] that serves [preset]. */
fun localBuildPrefix(localBuild: File, preset: String): File =
  if (localBuild.resolve(NATIVE_DESCRIPTOR_PATH).isFile) localBuild
  else localBuild.resolve("$preset/install")

/** The CMake preset that builds [targetPlatform] for [backend]. */
fun nativePreset(targetPlatform: String, backend: String): String {
  val suffix =
    when (backend) {
      "opengl" -> if (targetPlatform.startsWith("windows-")) "wgl" else "egl"
      else -> backend
    }
  return "$targetPlatform-$suffix"
}

/**
 * The backend a binding test takes from a build root: Metal on Apple, OpenGL on Android, and Vulkan
 * elsewhere.
 */
fun defaultBackend(targetPlatform: String): String =
  when {
    targetPlatform.startsWith("macos-") ||
      targetPlatform.startsWith("ios-") ||
      targetPlatform.startsWith("tvos-") -> "metal"
    targetPlatform.startsWith("android-") -> "opengl"
    else -> "vulkan"
  }

/** The native target platform of a Kotlin/Native target. */
fun kotlinNativeTargetPlatform(kotlinTarget: String): String =
  AndroidTarget.entries.firstOrNull { it.kotlinNativeTarget == kotlinTarget }?.targetPlatform
    ?: when (kotlinTarget) {
      "iosArm64" -> "ios-arm64"
      "iosSimulatorArm64" -> "ios-simulator-arm64"
      "tvosArm64" -> "tvos-arm64"
      "tvosSimulatorArm64" -> "tvos-simulator-arm64"
      "linuxArm64" -> "linux-gnu-arm64"
      "linuxX64" -> "linux-gnu-x64"
      "macosArm64" -> "macos-arm64"
      else -> error("No native target platform for Kotlin/Native target $kotlinTarget")
    }

/**
 * Registers a task that checks that the local build serves [targetPlatform]. Without a [backend],
 * the task accepts an install prefix of any backend, and takes the [defaultBackend]'s preset from a
 * build root.
 */
fun Project.registerLocalBuildVerification(
  taskName: String,
  targetPlatform: String,
  backend: String? = null,
): TaskProvider<VerifyLocalBuild> {
  val localBuildPath = localBuild?.absolutePath
  return tasks.register(taskName, VerifyLocalBuild::class.java) {
    this.localBuildPath.set(localBuildPath)
    preset.set(nativePreset(targetPlatform, backend ?: defaultBackend(targetPlatform)))
    expectedTargetPlatform.set(targetPlatform)
    expectedBackend.set(backend)
  }
}
