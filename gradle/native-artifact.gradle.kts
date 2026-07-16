import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact

val maplibreNativeCInstallDir =
  providers
    .gradleProperty("maplibreNativeCInstallDir")
    .map(rootProject::file)
    .orElse(rootProject.layout.buildDirectory.dir("host-native-unconfigured").map { it.asFile })
    .get()
val maplibreNativeCHostLibraryDirs =
  providers
    .gradleProperty("maplibreNativeCHostLibraryDirs")
    .map { value ->
      value.split(File.pathSeparator).filter(String::isNotBlank).map(rootProject::file)
    }
    .getOrElse(emptyList())

extensions.add(
  "maplibreNativeC",
  MaplibreNativeCArtifact(maplibreNativeCInstallDir, maplibreNativeCHostLibraryDirs),
)
