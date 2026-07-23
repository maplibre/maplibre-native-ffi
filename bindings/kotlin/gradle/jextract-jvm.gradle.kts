import java.net.URI
import java.security.MessageDigest
import javax.inject.Inject
import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.InputDirectory
import org.gradle.api.tasks.InputFile
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.PathSensitive
import org.gradle.api.tasks.PathSensitivity
import org.gradle.api.tasks.Sync
import org.gradle.api.tasks.TaskAction
import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.process.ExecOperations
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.catalogVersionInt

abstract class DownloadJextractTask : DefaultTask() {
  @get:Input abstract val url: Property<String>
  @get:Input abstract val expectedSha256: Property<String>
  @get:OutputFile abstract val archive: RegularFileProperty

  @TaskAction
  fun download() {
    val archiveFile = archive.get().asFile
    archiveFile.parentFile.mkdirs()
    if (!archiveFile.isFile || sha256(archiveFile) != expectedSha256.get()) {
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

abstract class GenerateJvmJextractBindingsTask : DefaultTask() {
  @get:InputFile
  @get:PathSensitive(PathSensitivity.RELATIVE)
  abstract val includes: RegularFileProperty

  @get:InputDirectory
  @get:PathSensitive(PathSensitivity.RELATIVE)
  abstract val cHeaders: DirectoryProperty

  @get:InputFile
  @get:PathSensitive(PathSensitivity.NONE)
  abstract val jextractExecutable: RegularFileProperty

  @get:Input abstract val jextractArchiveSha256: Property<String>
  @get:OutputDirectory abstract val outputDirectory: DirectoryProperty
  @get:Inject abstract val execOperations: ExecOperations

  @TaskAction
  fun generate() {
    val output = outputDirectory.get().asFile
    output.deleteRecursively()
    output.mkdirs()
    execOperations
      .exec {
        executable = jextractExecutable.get().asFile.absolutePath
        args(
          "--output",
          output.absolutePath,
          "--target-package",
          "org.maplibre.nativeffi.internal.c",
          "--header-class-name",
          "MapLibreNativeC",
          "@${includes.get().asFile.absolutePath}",
          "-I",
          cHeaders.get().asFile.absolutePath,
          cHeaders.get().file("maplibre_native_c.h").asFile.absolutePath,
        )
      }
      .assertNormalExitValue()

    val sharedBindings =
      output.resolve("org/maplibre/nativeffi/internal/c/MapLibreNativeC\$shared.java")
    // jextract lowers size_t, int64_t, and uint64_t through the build host's typedefs. Unix
    // output therefore uses C_LONG even though every such value in our 64-bit C ABI is 64 bits,
    // while C_LONG itself becomes 32 bits when the same JVM artifact runs on Windows.
    // The public API has no plain C long, so every generated C_LONG use is a fixed-width value.
    sharedBindings.writeText(
      sharedBindings
        .readText()
        .replace(
          Regex(
            """public static final ValueLayout\.Of(?:Int|Long) C_LONG\s*=\s*""" +
              """\(ValueLayout\.Of(?:Int|Long)\)\s*""" +
              """Linker\.nativeLinker\(\)\.canonicalLayouts\(\)\.get\("long"\);"""
          ),
          "public static final ValueLayout.OfLong C_LONG = JAVA_LONG;",
        )
    )
  }
}

val hostPlatform = HostPlatform.current()
val jextractDistribution = hostPlatform.jextractDistribution
val checkedInCHeaders = rootProject.layout.projectDirectory.dir("include")

val generatedJextractSources = layout.buildDirectory.dir("generated/sources/jextract/jvmMain/java")
val jextractArchive = layout.buildDirectory.file("jextract/openjdk-25-jextract.tar.gz")
val jextractInstallDir = layout.buildDirectory.dir("jextract/tool")
val jextractExecutableFile = jextractInstallDir.map { dir ->
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
  tasks.register<GenerateJvmJextractBindingsTask>("generateJvmJextractBindings") {
    group = "build"
    description = "Generates JVM FFM declarations for the MapLibre Native C ABI with jextract."
    dependsOn(extractJextract)
    includes = layout.projectDirectory.file("src/jextract/maplibre-native-c.includes")
    cHeaders = checkedInCHeaders
    jextractExecutable = layout.file(provider { jextractExecutableFile.get() })
    jextractArchiveSha256 = jextractDistribution.sha256
    outputDirectory = generatedJextractSources
  }

tasks.named<JavaCompile>("compileJvmMainJava") {
  dependsOn(generateJvmJextractBindings)
  source(generatedJextractSources)
  options.release = catalogVersionInt("java-release")
}
