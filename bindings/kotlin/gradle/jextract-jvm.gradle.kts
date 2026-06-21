import java.net.URI
import java.security.MessageDigest
import org.gradle.api.DefaultTask
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.Sync
import org.gradle.api.tasks.TaskAction
import org.gradle.api.tasks.compile.JavaCompile
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact
import org.maplibre.nativeffi.gradle.catalogVersionInt

abstract class DownloadJextractTask : DefaultTask() {
  @get:Input abstract val url: Property<String>
  @get:Input abstract val expectedSha256: Property<String>
  @get:OutputFile abstract val archive: RegularFileProperty

  @TaskAction
  fun download() {
    val archiveFile = archive.get().asFile
    archiveFile.parentFile.mkdirs()
    if (!archiveFile.isFile) {
      URI(url.get()).toURL().openStream().use { input ->
        archiveFile.outputStream().use { output -> input.copyTo(output) }
      }
    }
    val actualSha256 = sha256(archiveFile)
    check(actualSha256 == expectedSha256.get()) {
      "Invalid jextract archive checksum for $archiveFile: expected ${expectedSha256.get()}, got $actualSha256"
    }
  }

  private fun sha256(file: File): String {
    val digest = MessageDigest.getInstance("SHA-256")
    file.inputStream().use { input ->
      val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
      while (true) {
        val byteCount = input.read(buffer)
        if (byteCount < 0) break
        digest.update(buffer, 0, byteCount)
      }
    }
    return digest.digest().joinToString("") { "%02x".format(it) }
  }
}

val hostPlatform = HostPlatform.current()
val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val jextractDistribution = hostPlatform.jextractDistribution

val generatedJextractSources = layout.buildDirectory.dir("generated/sources/jextract/jvmMain/java")
val jextractArchive = layout.buildDirectory.file("jextract/openjdk-25-jextract.tar.gz")
val jextractInstallDir = layout.buildDirectory.dir("jextract/tool")
val jextractExecutable = jextractInstallDir.map { dir ->
  dir.file("jextract-25/bin/${hostPlatform.jextractExecutableFileName}").asFile
}

// The de.infolektuell.jextract Gradle plugin applies Gradle's Java plugin, which is incompatible
// with Kotlin Multiplatform projects.
val downloadJextract =
  tasks.register<DownloadJextractTask>("downloadJextract") {
    url = jextractDistribution.url
    expectedSha256 = jextractDistribution.sha256
    archive = jextractArchive
  }

val extractJextract =
  tasks.register<Sync>("extractJextract") {
    val installDir = jextractInstallDir.get().asFile
    dependsOn(downloadJextract)
    from(tarTree(jextractArchive))
    into(jextractInstallDir)
    doFirst { installDir.deleteRecursively() }
  }

val generateJvmJextractBindings =
  tasks.register<Exec>("generateJvmJextractBindings") {
    group = "build"
    description = "Generates JVM FFM declarations for the MapLibre Native C ABI with jextract."
    dependsOn(extractJextract)
    inputs.file("src/jextract/maplibre-native-c.includes")
    inputs.files(maplibreNativeC.includeDirs).withPropertyName("maplibreNativeCIncludeDirs")
    outputs.dir(generatedJextractSources)
    executable = jextractExecutable.get().absolutePath
    args(
      "--output",
      generatedJextractSources.get().asFile.absolutePath,
      "--target-package",
      "org.maplibre.nativeffi.internal.c",
      "--header-class-name",
      "MapLibreNativeC",
      "@${layout.projectDirectory.file("src/jextract/maplibre-native-c.includes").asFile.absolutePath}",
      *maplibreNativeC.includeDirs.flatMap { listOf("-I", it.absolutePath) }.toTypedArray(),
      rootProject.layout.projectDirectory.file("include/maplibre_native_c.h").asFile.absolutePath,
    )
  }

tasks.named<JavaCompile>("compileJvmMainJava") {
  dependsOn(generateJvmJextractBindings)
  source(generatedJextractSources)
  options.release = catalogVersionInt("java-release")
}
