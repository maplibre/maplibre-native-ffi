package org.maplibre.nativejni.internal.loader;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.test.NativeTestSupport;

final class NativeLibraryTest {
  @Test
  void exposesDocumentedLookupInputs() {
    assertFalse(NativeLibrary.LIBRARY_PATH_PROPERTY.isBlank());
    assertFalse(NativeLibrary.LIBRARY_PATH_ENV.isBlank());
    assertEquals("jniMaplibreNativeC", NativeLibrary.LIBRARY_NAME);
    assertTrue(
        File.separatorChar == '\\'
            || System.mapLibraryName(NativeLibrary.LIBRARY_NAME).contains("libjniMaplibreNativeC"));
  }

  @Test
  void loadedLibraryServesCAbiCalls() {
    assertTrue(Maplibre.cVersion() >= 0);
    NativeLibrary.ensureLoaded();
  }

  @Test
  void subprocessLoadsThroughSystemProperty() throws Exception {
    assertLoaderSmoke(List.of("-D" + NativeLibrary.LIBRARY_PATH_PROPERTY + "=" + libraryPath()));
  }

  @Test
  void subprocessLoadsExactPath() throws Exception {
    assertLoaderSmoke(List.of(), libraryPath());
  }

  @Test
  void subprocessRejectsBadExactPath() throws Exception {
    assertLoaderSmokeFails(List.of(), Path.of(libraryPath()).resolveSibling("missing-jni-bridge"));
  }

  @Test
  void subprocessRejectsMissingConfiguredPropertyPath() throws Exception {
    assertLoaderSmokeFails(
        List.of(
            "-D"
                + NativeLibrary.LIBRARY_PATH_PROPERTY
                + "="
                + Path.of(libraryPath()).resolveSibling("missing-jni-bridge")));
  }

  @Test
  void subprocessLoadsThroughEnvironmentPath() throws Exception {
    assertLoaderSmoke(List.of(), NativeLibrary.LIBRARY_PATH_ENV, libraryPath());
  }

  @Test
  void subprocessLoadsThroughJavaLibraryPath() throws Exception {
    assertLoaderSmoke(List.of("-Djava.library.path=" + Path.of(libraryPath()).getParent()));
  }

  private static String libraryPath() {
    var configuredPath = NativeTestSupport.configuredLibraryPath();
    if (configuredPath != null && !configuredPath.isBlank()) {
      return configuredPath;
    }
    return javaLibraryPathCandidate().toString();
  }

  private static Path javaLibraryPathCandidate() {
    var libraryFileName = System.mapLibraryName(NativeLibrary.LIBRARY_NAME);
    for (var directory : System.getProperty("java.library.path", "").split(File.pathSeparator)) {
      if (directory.isBlank()) {
        continue;
      }
      var candidate = Path.of(directory).resolve(libraryFileName);
      if (Files.isRegularFile(candidate)) {
        return candidate;
      }
    }
    throw new IllegalStateException("Missing JNI bridge in java.library.path: " + libraryFileName);
  }

  private static void assertLoaderSmoke(List<String> javaArguments) throws Exception {
    assertLoaderSmoke(javaArguments, null, null);
  }

  private static void assertLoaderSmoke(List<String> javaArguments, String exactPath)
      throws Exception {
    assertLoaderSmoke(javaArguments, null, null, exactPath, 0);
  }

  private static void assertLoaderSmoke(List<String> javaArguments, String envName, String envValue)
      throws Exception {
    assertLoaderSmoke(javaArguments, envName, envValue, null, 0);
  }

  private static void assertLoaderSmokeFails(List<String> javaArguments, Path exactPath)
      throws Exception {
    assertLoaderSmoke(javaArguments, null, null, exactPath.toString(), 1);
  }

  private static void assertLoaderSmokeFails(List<String> javaArguments) throws Exception {
    assertLoaderSmoke(javaArguments, null, null, null, 1);
  }

  private static void assertLoaderSmoke(
      List<String> javaArguments,
      String envName,
      String envValue,
      String exactPath,
      int expectedExit)
      throws Exception {
    var javaExecutable =
        Path.of(System.getProperty("java.home"), "bin", isWindows() ? "java.exe" : "java");
    var command = new java.util.ArrayList<String>();
    command.add(javaExecutable.toString());
    command.add("--enable-native-access=ALL-UNNAMED");
    command.add("-cp");
    command.add(System.getProperty("java.class.path"));
    command.addAll(javaArguments);
    command.add(LoaderSmoke.class.getName());
    if (exactPath != null) {
      command.add(exactPath);
    }
    var processBuilder = new ProcessBuilder(command).redirectErrorStream(true);
    if (envName != null) {
      processBuilder.environment().put(envName, envValue);
    }
    var process = processBuilder.start();
    var output = new String(process.getInputStream().readAllBytes(), StandardCharsets.UTF_8);
    assertEquals(expectedExit, process.waitFor(), output);
  }

  private static boolean isWindows() {
    return System.getProperty("os.name", "").toLowerCase().contains("win");
  }

  public static final class LoaderSmoke {
    private LoaderSmoke() {}

    public static void main(String[] args) {
      if (args.length == 0) {
        Maplibre.loadNativeLibrary();
      } else {
        NativeLibrary.load(Path.of(args[0]));
      }
      if (Maplibre.cVersion() < 0) {
        throw new IllegalStateException("invalid C ABI version");
      }
    }
  }
}
