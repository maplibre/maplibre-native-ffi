// A Gradle JVM consumer of the binding, a runtime, and the runtime's natives JAR
// for the host, with the Maven repository at maplibre.repository in place of
// Maven Central.
plugins {
  kotlin("jvm") version libs.versions.kotlin
  application
}

val repository = providers.gradleProperty("maplibre.repository").get()
val version = providers.gradleProperty("maplibre.version").get()
val backend = providers.gradleProperty("maplibre.backend").get()
val classifier = providers.gradleProperty("maplibre.classifier").get()

repositories {
  exclusiveContent {
    forRepository { maven(repository) }
    filter { includeGroup("org.maplibre.nativeffi") }
  }
  mavenCentral()
}

dependencies {
  implementation("org.maplibre.nativeffi:maplibre-native-ffi:$version")
  implementation("org.maplibre.nativeffi:maplibre-native-ffi-runtime-$backend:$version")
  runtimeOnly(
    "org.maplibre.nativeffi:maplibre-native-ffi-runtime-$backend-jvm:$version:$classifier",
  )
}

kotlin { jvmToolchain(libs.versions.java.toolchain.get().toInt()) }

application {
  mainClass = "MainKt"
  applicationDefaultJvmArgs = listOf("--enable-native-access=ALL-UNNAMED")
}
