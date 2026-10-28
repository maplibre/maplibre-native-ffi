package org.maplibre.nativeffi.gradle

import groovy.json.JsonSlurper
import java.io.File
import java.io.Serializable
import org.gradle.api.Project
import org.gradle.api.provider.Property
import org.gradle.api.provider.ValueSource
import org.gradle.api.provider.ValueSourceParameters
import org.gradle.api.tasks.TaskProvider

/**
 * The Gradle property that names the local build every native input comes from: an install prefix,
 * or a directory holding installs one level down, such as a CMake build root.
 */
const val LOCAL_BUILD_PROPERTY = "maplibre.localBuild"

internal const val NATIVE_DESCRIPTOR_PATH = "share/maplibre-native-c/artifact.json"

/** The local build that [LOCAL_BUILD_PROPERTY] names, or null when it is unset. */
private val Project.localBuild: File?
  get() = providers.gradleProperty(LOCAL_BUILD_PROPERTY).map(rootProject::file).orNull

/**
 * The install in the local build that serves one target, or, in [problem], why none does. A task
 * that needs the install depends on the [registerLocalBuildVerification] task, which reports the
 * problem, so that a local build missing one target fails only the tasks that need it.
 */
data class NativeInstall(val prefix: File?, internal val problem: String?) : Serializable

/**
 * Finds the install in the local build that serves [targetPlatform] with [backend], or without one,
 * with the platform's default backend. An install prefix serves its own backend unless [backend] is
 * given.
 */
fun Project.nativeInstall(targetPlatform: String, backend: String? = null): NativeInstall {
  val root =
    localBuild
      ?: return NativeInstall(
        null,
        "Set -P$LOCAL_BUILD_PROPERTY to an install prefix, or to a directory holding installs " +
          "one level down, such as a CMake build root",
      )
  // The configuration cache reruns the lookup to validate itself, so it tracks the answer rather
  // than every file the lookup reads. What the chosen descriptor says beyond the answer, such as
  // its link libraries, is tracked on its own.
  val install =
    providers
      .of(NativeInstallSource::class.java) {
        parameters.root.set(root.absolutePath)
        parameters.targetPlatform.set(targetPlatform)
        parameters.backend.set(backend)
      }
      .get()
  install.prefix?.let {
    providers
      .fileContents(layout.projectDirectory.file(it.resolve(NATIVE_DESCRIPTOR_PATH).path))
      .asBytes
      .get()
  }
  return install
}

internal abstract class NativeInstallSource :
  ValueSource<NativeInstall, NativeInstallSource.Parameters> {
  interface Parameters : ValueSourceParameters {
    val root: Property<String>
    val targetPlatform: Property<String>
    val backend: Property<String>
  }

  override fun obtain(): NativeInstall =
    findInstall(
      File(parameters.root.get()),
      parameters.targetPlatform.get(),
      parameters.backend.orNull,
    )
}

private fun findInstall(root: File, targetPlatform: String, backend: String?): NativeInstall {
  val request =
    if (backend == null) "$targetPlatform with its default backend" else "$targetPlatform $backend"
  if (root.resolve(NATIVE_DESCRIPTOR_PATH).isFile) {
    // A prefix serves one platform, so a build for several targets names a directory of installs
    // instead. Only a selected backend can conflict; without one, the prefix's own backend serves.
    val descriptor = readDescriptor(root)
    return if (
      descriptor.platform["id"] == targetPlatform &&
        (backend == null || descriptor["backend"] == backend)
    ) {
      NativeInstall(root, null)
    } else {
      NativeInstall(
        null,
        "$LOCAL_BUILD_PROPERTY names an install prefix built for " +
          "${descriptor.platform["id"]} ${descriptor["backend"]}, which cannot serve $request",
      )
    }
  }

  // Without a selected backend, only the target's default backend serves, so that building another
  // backend under the same directory never changes which install an unchanged request gets.
  val matches =
    root
      .listFiles()
      ?.filter(File::isDirectory)
      .orEmpty()
      .flatMap { listOf(it.resolve("install"), it) }
      .filter { prefix ->
        prefix.resolve(NATIVE_DESCRIPTOR_PATH).isFile &&
          readDescriptor(prefix).let {
            it.platform["id"] == targetPlatform &&
              it["backend"] == (backend ?: it.platform["defaultBackend"])
          }
      }
      .sorted()
  val named = "$LOCAL_BUILD_PROPERTY names $root, which holds"
  return when (matches.size) {
    1 -> NativeInstall(matches.single(), null)
    0 ->
      NativeInstall(
        null,
        "$named no install for $request. Build it, or select another backend.",
      )
    else ->
      NativeInstall(
        null,
        "$named several installs for $request:${matches.joinToString("") { "\n  $it" }}\n" +
          "Remove all but one, or name one of them as the install prefix.",
      )
  }
}

private fun readDescriptor(prefix: File): Map<*, *> =
  JsonSlurper().parseText(prefix.resolve(NATIVE_DESCRIPTOR_PATH).readText()) as? Map<*, *>
    ?: emptyMap<Any, Any>()

private val Map<*, *>.platform: Map<*, *>
  get() = this["platform"] as? Map<*, *> ?: emptyMap<Any, Any>()

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

/** Registers a task that fails with [install]'s problem, if it has one. */
fun Project.registerLocalBuildVerification(
  taskName: String,
  install: NativeInstall,
): TaskProvider<VerifyLocalBuild> =
  tasks.register(taskName, VerifyLocalBuild::class.java) { problem.set(install.problem) }
