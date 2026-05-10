import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.testing.Test

plugins {
  `java-library`
  id("de.infolektuell.jextract") version "1.4.0"
}

repositories {
  mavenCentral()
}

jextract.libraries {
  val maplibreNativeC by registering {
    header = rootProject.layout.projectDirectory.file("include/maplibre_native_c.h")
    includes.add(rootProject.layout.projectDirectory.dir("include"))
    headerClassName = "MapLibreNativeC"
    targetPackage = "org.maplibre.nativeffi.internal.c"
    whitelist.argFile = layout.projectDirectory.file("src/jextract/maplibre-native-c.includes")
  }

  sourceSets.named("main") {
    jextract.libraries.addLater(maplibreNativeC)
  }
}

dependencies {
  testImplementation(platform("org.junit:junit-bom:6.0.3"))
  testImplementation("org.junit.jupiter:junit-jupiter")
  testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

tasks.withType<JavaCompile>().configureEach {
  options.release = 25
}

tasks.withType<Test>().configureEach {
  useJUnitPlatform()
  jvmArgs("--enable-native-access=ALL-UNNAMED")
}
