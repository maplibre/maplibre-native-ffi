package org.maplibre.nativeffi.gradle

import org.gradle.api.GradleException
import org.gradle.api.Project
import org.gradle.api.artifacts.VersionCatalogsExtension
import org.gradle.api.artifacts.VersionConstraint

fun Project.catalogVersion(name: String): String {
  val version: VersionConstraint =
    extensions
      .getByType(VersionCatalogsExtension::class.java)
      .named("libs")
      .findVersion(name)
      .orElseThrow { IllegalStateException("Missing version catalog entry: $name") }
  return version.requiredVersion
}

fun Project.catalogVersionInt(name: String): Int = catalogVersion(name).toInt()

fun Project.requiredEnvironmentVariable(name: String): String =
  providers.environmentVariable(name).orNull
    ?: throw GradleException("$name is required; run Gradle through mise or set it explicitly")
