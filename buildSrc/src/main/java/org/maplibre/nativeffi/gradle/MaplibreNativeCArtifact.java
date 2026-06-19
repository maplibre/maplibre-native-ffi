package org.maplibre.nativeffi.gradle;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;

public final class MaplibreNativeCArtifact {
  private final File propertiesFile;
  private final Properties properties;

  public MaplibreNativeCArtifact(File propertiesFile) {
    this.propertiesFile = propertiesFile;
    this.properties = loadProperties(propertiesFile);
  }

  public File getPropertiesFile() {
    return propertiesFile;
  }

  public File getLibraryPath() {
    return new File(property("maplibreNativeC.libraryPath"));
  }

  public List<File> getIncludeDirs() {
    return pathList("maplibreNativeC.includeDirs").stream().map(File::new).toList();
  }

  public List<File> getLinkDirs() {
    return pathList("maplibreNativeC.linkDirs").stream().map(File::new).toList();
  }

  public List<File> getRuntimeLibraryDirs() {
    return pathList("maplibreNativeC.runtimeLibraryDirs").stream().map(File::new).toList();
  }

  public List<String> getLinkLibraries() {
    return pathList("maplibreNativeC.linkLibraries");
  }

  public List<String> getFrameworks() {
    return pathList("maplibreNativeC.frameworks");
  }

  private static Properties loadProperties(File propertiesFile) {
    var properties = new Properties();
    try (var input = new FileInputStream(propertiesFile)) {
      properties.load(input);
    } catch (IOException error) {
      throw new UncheckedIOException("Failed to read " + propertiesFile, error);
    }
    return properties;
  }

  private String property(String name) {
    return properties.getProperty(name, "");
  }

  private List<String> pathList(String name) {
    return Arrays.stream(property(name).split(File.pathSeparator, -1))
        .filter(value -> !value.isBlank())
        .toList();
  }
}
