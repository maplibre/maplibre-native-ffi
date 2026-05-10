import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.testing.Test

plugins {
  `java-library`
  id("de.infolektuell.jextract") version "1.4.0"
}

repositories { mavenCentral() }

jextract.libraries {
  val maplibreNativeC by registering {
    header = rootProject.layout.projectDirectory.file("include/maplibre_native_c.h")
    includes.add(rootProject.layout.projectDirectory.dir("include"))
    headerClassName = "MapLibreNativeC"
    targetPackage = "org.maplibre.nativeffi.internal.c"
    whitelist.argFile = layout.projectDirectory.file("src/jextract/maplibre-native-c.includes")
  }

  sourceSets.named("main") { jextract.libraries.addLater(maplibreNativeC) }
}

dependencies {
  testImplementation(platform("org.junit:junit-bom:6.0.3"))
  testImplementation("org.junit.jupiter:junit-jupiter")
  testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

tasks.withType<JavaCompile>().configureEach { options.release = 25 }

val nativeLibraryPathProperty = "org.maplibre.nativeffi.library.path"
val nativeLibraryPathEnvironment = "MAPLIBRE_NATIVE_FFI_LIBRARY_PATH"
val explicitNativeLibraryPath = providers.systemProperty(nativeLibraryPathProperty)
val nativeBuildDirForTests =
  providers
    .environmentVariable("MLN_FFI_BUILD_DIR")
    .orElse(rootProject.layout.buildDirectory.dir("host").map { it.asFile.absolutePath })
    .get()
val nativeLibraryPathForTests =
  explicitNativeLibraryPath
    .orElse(providers.environmentVariable(nativeLibraryPathEnvironment))
    .orElse(
      providers.provider { "$nativeBuildDirForTests/${System.mapLibraryName("maplibre-native-c")}" }
    )
    .get()

tasks.withType<Test>().configureEach {
  useJUnitPlatform()
  jvmArgs("--enable-native-access=ALL-UNNAMED")
  explicitNativeLibraryPath.orNull?.let { systemProperty(nativeLibraryPathProperty, it) }
  inputs.property("mlnFfiBuildDir", nativeBuildDirForTests)
  inputs.file(nativeLibraryPathForTests).withPropertyName("maplibreNativeCLibrary").optional()
  environment("MLN_FFI_BUILD_DIR", nativeBuildDirForTests)
}
