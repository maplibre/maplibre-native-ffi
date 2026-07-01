import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

fun Project.maplibreNativeCInstallDir(): File {
  val installDir =
    providers.environmentVariable("MLN_FFI_NATIVE_INSTALL_DIR").orNull
      ?: throw GradleException(
        "MLN_FFI_NATIVE_INSTALL_DIR is required; run native binding builds through mise."
      )
  return file(installDir)
}

fun Project.maplibreNativeCHostLibraryDirs(): List<File> {
  val libraryDirs =
    providers.environmentVariable("MLN_FFI_HOST_LIBRARY_DIRS").orNull
      ?: throw GradleException(
        "MLN_FFI_HOST_LIBRARY_DIRS is required; run native binding builds through mise."
      )
  return libraryDirs.split(File.pathSeparator).filter { it.isNotBlank() }.map { file(it) }
}

val maplibreNativeCInstallDir = maplibreNativeCInstallDir()
val maplibreNativeCHostLibraryDirs = maplibreNativeCHostLibraryDirs()

if (!maplibreNativeCInstallDir.isDirectory) {
  throw GradleException("Missing native install directory: $maplibreNativeCInstallDir")
}

extensions.add(
  "maplibreNativeC",
  MaplibreNativeCArtifact(maplibreNativeCInstallDir, maplibreNativeCHostLibraryDirs),
)
