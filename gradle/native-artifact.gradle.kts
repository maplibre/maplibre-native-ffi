import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

fun Project.maplibreNativeCInstallDir(): File {
  val installDir =
    providers.environmentVariable("MLN_FFI_NATIVE_INSTALL_DIR").orNull
      ?: throw GradleException(
        "MLN_FFI_NATIVE_INSTALL_DIR is required; run native binding builds through mise."
      )
  return file(installDir)
}

val maplibreNativeCInstallDir = maplibreNativeCInstallDir()

if (!maplibreNativeCInstallDir.isDirectory) {
  throw GradleException("Missing native install directory: $maplibreNativeCInstallDir")
}

extensions.add("maplibreNativeC", MaplibreNativeCArtifact(maplibreNativeCInstallDir))
