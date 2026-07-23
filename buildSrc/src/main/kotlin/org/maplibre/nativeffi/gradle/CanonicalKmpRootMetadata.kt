package org.maplibre.nativeffi.gradle

import groovy.json.JsonOutput
import groovy.json.JsonSlurper
import java.io.Serializable
import org.gradle.api.Action
import org.gradle.api.Project
import org.gradle.api.Task
import org.gradle.api.publish.tasks.GenerateModuleMetadata

/**
 * Replaces fallback coordinates for targets disabled on the current host.
 *
 * Kotlin assigns those variants the Gradle project name and `unspecified` because it creates no
 * target publication delegate on that host. Split-host cinterop publishing still needs one
 * canonical root that references every target publication.
 */
fun Project.canonicalizeKmpRootMetadata(
  group: String,
  rootModule: String,
  version: String,
  targetModules: Map<String, String>,
) {
  tasks
    .named(
      "generateMetadataFileForKotlinMultiplatformPublication",
      GenerateModuleMetadata::class.java,
    )
    .configure {
      inputs.property("canonicalRootGroup", group)
      inputs.property("canonicalRootModule", rootModule)
      inputs.property("canonicalRootVersion", version)
      inputs.properties(targetModules.mapKeys { (target, _) -> "canonicalTargetModule.$target" })
      doLast(CanonicalizeKmpRootMetadata(group, rootModule, version, targetModules))
    }
}

private class CanonicalizeKmpRootMetadata(
  private val group: String,
  private val rootModule: String,
  private val version: String,
  targetModules: Map<String, String>,
) : Action<Task>, Serializable {
  private val targetModules = targetModules.toMap()

  override fun execute(task: Task) {
    require(task is GenerateModuleMetadata) {
      "Expected GenerateModuleMetadata, found ${task::class.qualifiedName}"
    }
    val metadataFile = task.outputFile.get().asFile
    @Suppress("UNCHECKED_CAST")
    val metadata = JsonSlurper().parse(metadataFile) as MutableMap<String, Any?>
    @Suppress("UNCHECKED_CAST") val component = metadata["component"] as MutableMap<String, Any?>
    require(component["group"] == group) {
      "Expected $group, found ${component["group"]} in ${metadataFile.absolutePath}"
    }
    require(component["module"] == rootModule) {
      "Expected $rootModule, found ${component["module"]} in ${metadataFile.absolutePath}"
    }
    require(component["version"] == version) {
      "Expected $version, found ${component["version"]} in ${metadataFile.absolutePath}"
    }

    @Suppress("UNCHECKED_CAST")
    val variants = metadata["variants"] as List<MutableMap<String, Any?>>
    val referencedTargets = mutableSetOf<String>()
    variants.forEach { variant ->
      @Suppress("UNCHECKED_CAST")
      val availableAt = variant["available-at"] as? MutableMap<String, Any?> ?: return@forEach
      val variantName = variant["name"] as String
      val target =
        targetModules.keys.singleOrNull(variantName::startsWith)
          ?: error("No target module mapping for root variant '$variantName'")
      val targetModule = targetModules.getValue(target)
      referencedTargets += target
      availableAt["group"] = group
      availableAt["module"] = targetModule
      availableAt["version"] = version
      availableAt["url"] = "../../$targetModule/$version/$targetModule-$version.module"
    }
    require(referencedTargets == targetModules.keys) {
      "Root metadata referenced $referencedTargets; expected ${targetModules.keys}"
    }

    metadataFile.writeText(JsonOutput.prettyPrint(JsonOutput.toJson(metadata)) + "\n")
  }

  private companion object {
    private const val serialVersionUID = 1L
  }
}
