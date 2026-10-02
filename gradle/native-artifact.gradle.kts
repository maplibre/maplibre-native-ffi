import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

val maplibreNativeCInstallDir =
  providers
    .gradleProperty("maplibreNativeCInstallDir")
    .map(rootProject::file)
    .orElse(rootProject.layout.buildDirectory.dir("host-native-unconfigured").map { it.asFile })
    .get()
// CI exposes host runtime dependencies, such as a Vulkan loader that ships only
// inside the SDK, through the same variable the Python loader reads.
val maplibreNativeCHostLibraryDirs =
  providers
    .gradleProperty("maplibreNativeCHostLibraryDirs")
    .orElse(providers.environmentVariable("MAPLIBRE_NATIVE_C_RUNTIME_LIBRARY_DIRS"))
    .map { value ->
      value.split(File.pathSeparator).filter(String::isNotBlank).map(rootProject::file)
    }
    .getOrElse(emptyList())

extensions.add(
  "maplibreNativeC",
  MaplibreNativeCArtifact(maplibreNativeCInstallDir, maplibreNativeCHostLibraryDirs),
)
