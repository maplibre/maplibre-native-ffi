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
data class NativeInstall(val prefix: File?, internal val problem: String?)

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
  // The configuration cache tracks the lookup rather than every file it reads; see [Lookup.equals]
  // for when a cached lookup still serves. What the chosen descriptor says beyond the answer, such
  // as its link libraries, is tracked on its own.
  val lookup =
    providers
      .of(NativeInstallSource::class.java) {
        parameters.root.set(root.absolutePath)
        parameters.targetPlatform.set(targetPlatform)
        parameters.backend.set(backend)
      }
      .get()
  lookup.prefix?.let {
    providers
      .fileContents(layout.projectDirectory.file(it.resolve(NATIVE_DESCRIPTOR_PATH).path))
      .asBytes
      .get()
  }
  return NativeInstall(lookup.prefix, lookup.problem)
}

/** A lookup's answer, and every install in the local build that matches its request. */
internal class Lookup(val prefix: File?, val problem: String?, val matches: List<File>) :
  Serializable {
  /**
   * Whether this lookup, stored by the configuration cache, still serves when a fresh lookup finds
   * [other]. Gradle reuses the configuration only if the stored lookup equals the fresh one. A
   * chosen install keeps serving while it still matches, so that a second matching build is
   * reported only when configuration runs for another reason. A failed lookup keeps failing the
   * same way until an install appears or the failure changes kind, whatever unrelated installs come
   * and go.
   */
  override fun equals(other: Any?): Boolean =
    other is Lookup &&
      if (prefix != null) {
        prefix in other.matches
      } else {
        other.prefix == null && matches.isEmpty() == other.matches.isEmpty()
      }

  override fun hashCode(): Int = 0
}

internal abstract class NativeInstallSource : ValueSource<Lookup, NativeInstallSource.Parameters> {
  interface Parameters : ValueSourceParameters {
    val root: Property<String>
    val targetPlatform: Property<String>
    val backend: Property<String>
  }

  override fun obtain(): Lookup =
    findInstall(
      File(parameters.root.get()),
      parameters.targetPlatform.get(),
      parameters.backend.orNull,
    )
}

private fun findInstall(root: File, targetPlatform: String, backend: String?): Lookup {
  val request =
    if (backend == null) "$targetPlatform with its default backend" else "$targetPlatform $backend"
  if (root.resolve(NATIVE_DESCRIPTOR_PATH).isFile) {
    // A prefix serves one platform, so a build for several targets names a directory of installs
    // instead. Only a selected backend can conflict; without one, the prefix's own backend serves.
    val descriptor = readDescriptor(root)
    return if (
      descriptor["targetPlatform"] == targetPlatform &&
        (backend == null || descriptor["renderBackend"] == backend)
    ) {
      Lookup(root, null, listOf(root))
    } else {
      Lookup(
        null,
        "$LOCAL_BUILD_PROPERTY names an install prefix built for " +
          "${descriptor["targetPlatform"]} ${descriptor["renderBackend"]}, which cannot serve $request",
        emptyList(),
      )
    }
  }

  // Without a selected backend, only the target's default backend serves, so that building another
  // backend under the same directory never changes which install an unchanged request gets.
  val matches = mutableListOf<File>()
  val others = mutableListOf<String>()
  val stale = mutableListOf<File>()
  val children = root.listFiles()?.filter(File::isDirectory)?.sorted().orEmpty()
  for (child in children) {
    for (prefix in listOf(child.resolve("install"), child)) {
      if (!prefix.resolve(NATIVE_DESCRIPTOR_PATH).isFile) continue
      val descriptor = readDescriptor(prefix)
      val platform = descriptor["targetPlatform"]
      val builtBackend = descriptor["renderBackend"]
      val defaultBackend = descriptor["defaultBackend"]
      when {
        // Installs built before descriptors named their default backend.
        platform !is String || builtBackend !is String || defaultBackend !is String ->
          stale += prefix
        platform == targetPlatform && builtBackend == (backend ?: defaultBackend) ->
          matches += prefix
        else -> others += "$prefix ($platform $builtBackend)"
      }
    }
  }
  matches.singleOrNull()?.let {
    return Lookup(it, null, matches)
  }

  fun list(lines: List<Any>) = lines.joinToString("") { "\n  $it" }
  val named = "$LOCAL_BUILD_PROPERTY names $root, which holds"
  val problem =
    if (matches.isNotEmpty()) {
      "$named several installs for $request:${list(matches)}\n" +
        "Remove all but one, or name one of them as the install prefix."
    } else {
      buildString {
        append("$named no install for $request. Build one.")
        if (others.isNotEmpty()) append("\nIt holds:${list(others)}")
        if (stale.isNotEmpty()) {
          append(
            "\nThese installs predate the descriptor fields that the lookup reads; rebuild them:" +
              list(stale),
          )
        }
      }
    }
  return Lookup(null, problem, matches)
}

private fun readDescriptor(prefix: File): Map<*, *> =
  JsonSlurper().parseText(prefix.resolve(NATIVE_DESCRIPTOR_PATH).readText()) as? Map<*, *>
    ?: emptyMap<Any, Any>()

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
