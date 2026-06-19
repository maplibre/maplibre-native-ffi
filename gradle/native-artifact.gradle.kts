import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

fun Project.maplibreNativeCPropertiesFile(): File {
  val buildDir =
    providers.environmentVariable("MLN_FFI_BUILD_DIR").orNull
      ?: throw GradleException(
        "MLN_FFI_BUILD_DIR is required; run native binding builds through mise."
      )
  return file("$buildDir/maplibre-native-c.gradle.properties")
}

val maplibreNativeCPropertiesFile = maplibreNativeCPropertiesFile()

if (!maplibreNativeCPropertiesFile.isFile) {
  throw GradleException("Missing native artifact properties: $maplibreNativeCPropertiesFile")
}

extensions.add("maplibreNativeC", MaplibreNativeCArtifact(maplibreNativeCPropertiesFile))
