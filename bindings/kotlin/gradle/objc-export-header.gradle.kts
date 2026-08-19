import javax.inject.Inject
import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.InputDirectory
import org.gradle.api.tasks.InputFile
import org.gradle.api.tasks.PathSensitive
import org.gradle.api.tasks.PathSensitivity
import org.gradle.api.tasks.TaskAction
import org.gradle.process.ExecOperations
import org.jetbrains.kotlin.gradle.dsl.KotlinMultiplatformExtension
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.NativeBuildType
import org.jetbrains.kotlin.konan.target.KonanTarget
import org.maplibre.nativeffi.gradle.HostPlatform

// Kotlin tests compile against a klib. Apple CI compiles the generated
// Objective-C header with clang so a public name that collides with a C
// macro fails the same way an Xcode framework consumer would.

val hostIsMac = HostPlatform.current().isMac

val objcExportFrameworkBaseName = "MaplibreNativeFfiObjCCheck"
val objcExportBinaryName = "objcExportCheck"
val objcExportStub = layout.projectDirectory.file("src/objcExportCheck/import-header.m")
val objcExportScript = layout.projectDirectory.file("scripts/check-objc-export-header.sh")

fun appleSdkName(konanTarget: KonanTarget): String =
  when (konanTarget) {
    KonanTarget.MACOS_ARM64,
    KonanTarget.MACOS_X64 -> "macosx"
    KonanTarget.IOS_ARM64 -> "iphoneos"
    KonanTarget.IOS_SIMULATOR_ARM64,
    KonanTarget.IOS_X64 -> "iphonesimulator"
    KonanTarget.TVOS_ARM64 -> "appletvos"
    KonanTarget.TVOS_SIMULATOR_ARM64,
    KonanTarget.TVOS_X64 -> "appletvsimulator"
    else -> error("No Xcode SDK mapping for Kotlin/Native target ${konanTarget.name}")
  }

abstract class CheckObjcExportHeaderTask : DefaultTask() {
  @get:Input abstract val sdkName: Property<String>

  @get:InputDirectory
  @get:PathSensitive(PathSensitivity.RELATIVE)
  abstract val frameworkBundle: DirectoryProperty

  @get:InputFile @get:PathSensitive(PathSensitivity.RELATIVE) abstract val stub: RegularFileProperty

  @get:InputFile
  @get:PathSensitive(PathSensitivity.RELATIVE)
  abstract val script: RegularFileProperty

  @get:Inject abstract val execOperations: ExecOperations

  @TaskAction
  fun check() {
    execOperations.exec {
      commandLine(
        "bash",
        script.get().asFile.absolutePath,
        sdkName.get(),
        frameworkBundle.get().asFile.absolutePath,
        stub.get().asFile.absolutePath,
      )
    }
  }
}

extensions.configure<KotlinMultiplatformExtension> {
  targets.withType<KotlinNativeTarget>().configureEach {
    if (!hostIsMac || !konanTarget.family.isAppleFamily) return@configureEach

    binaries.framework(objcExportBinaryName, listOf(NativeBuildType.DEBUG)) {
      baseName = objcExportFrameworkBaseName
      isStatic = true
    }

    val framework = binaries.getFramework(objcExportBinaryName, NativeBuildType.DEBUG)
    val checkTaskName = "checkObjcExportHeader${name.replaceFirstChar { it.uppercase() }}"
    val check =
      tasks.register<CheckObjcExportHeaderTask>(checkTaskName) {
        group = "verification"
        description =
          "Compiles the Kotlin/Native-generated Objective-C header for $name with clang."
        dependsOn(framework.linkTaskProvider)
        sdkName.set(appleSdkName(konanTarget))
        frameworkBundle.set(layout.dir(framework.linkTaskProvider.map { framework.outputFile }))
        stub.set(objcExportStub)
        script.set(objcExportScript)
      }
    tasks.matching { it.name == "${name}Test" }.configureEach { dependsOn(check) }
  }
}
