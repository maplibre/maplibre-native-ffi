package org.maplibre.nativeffi.gradle

import java.nio.file.Files
import java.nio.file.StandardCopyOption
import java.util.zip.ZipFile
import org.gradle.api.DefaultTask
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.tasks.CacheableTask
import org.gradle.api.tasks.InputFile
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.PathSensitive
import org.gradle.api.tasks.PathSensitivity
import org.gradle.api.tasks.TaskAction

@CacheableTask
abstract class ExtractAarClassesJar : DefaultTask() {
  @get:InputFile @get:PathSensitive(PathSensitivity.NONE) abstract val aarFile: RegularFileProperty

  @get:OutputFile abstract val outputJar: RegularFileProperty

  @TaskAction
  fun extract() {
    val output = outputJar.get().asFile.toPath()
    Files.createDirectories(output.parent)
    ZipFile(aarFile.get().asFile).use { aar ->
      val classes =
        requireNotNull(aar.getEntry("classes.jar")) { "AAR does not contain classes.jar" }
      aar.getInputStream(classes).use { input ->
        Files.copy(input, output, StandardCopyOption.REPLACE_EXISTING)
      }
    }
  }
}
