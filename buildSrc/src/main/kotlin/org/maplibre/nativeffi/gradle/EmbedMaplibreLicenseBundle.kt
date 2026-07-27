package org.maplibre.nativeffi.gradle

import java.io.File
import java.io.Serializable
import java.net.URI
import java.nio.file.FileSystems
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import org.gradle.api.Action
import org.gradle.api.Task
import org.jetbrains.kotlin.gradle.tasks.CInteropProcess

fun CInteropProcess.embedMaplibreLicenseBundle(licenseDirectory: File) {
  inputs.dir(licenseDirectory).withPropertyName("maplibreNativeCLicenses")
  doLast(EmbedMaplibreLicenseBundle(licenseDirectory))
}

private class EmbedMaplibreLicenseBundle(private val licenseDirectory: File) :
  Action<Task>, Serializable {
  override fun execute(task: Task) {
    require(task is CInteropProcess) {
      "Expected CInteropProcess, found ${task::class.qualifiedName}"
    }
    val klib = task.outputFileProvider.get()
    require(klib.exists()) { "KLIB does not exist: ${klib.absolutePath}" }
    require(licenseDirectory.isDirectory) {
      "Native license bundle does not exist: ${licenseDirectory.absolutePath}"
    }

    if (klib.isDirectory) {
      copyLicenses(licenseDirectory, klib.resolve("resources/licenses/maplibre-native-c"))
    } else {
      FileSystems.newFileSystem(URI.create("jar:${klib.toURI()}"), mapOf("create" to "false"))
        .use { archive ->
          val destination = archive.getPath("/resources/licenses/maplibre-native-c")
          licenseDirectory.walkTopDown().filter(File::isFile).forEach { license ->
            val relativePath = licenseDirectory.toPath().relativize(license.toPath())
            val destinationFile =
              destination.resolve(relativePath.toString().replace(File.separatorChar, '/'))
            Files.createDirectories(destinationFile.parent)
            Files.copy(license.toPath(), destinationFile, StandardCopyOption.REPLACE_EXISTING)
          }
        }
    }
  }

  private fun copyLicenses(source: File, destination: File) {
    source.walkTopDown().filter(File::isFile).forEach { license ->
      val relativePath = source.toPath().relativize(license.toPath())
      val destinationFile = destination.toPath().resolve(relativePath)
      Files.createDirectories(destinationFile.parent)
      Files.copy(license.toPath(), destinationFile, StandardCopyOption.REPLACE_EXISTING)
    }
  }

  private companion object {
    private const val serialVersionUID = 1L
  }
}
