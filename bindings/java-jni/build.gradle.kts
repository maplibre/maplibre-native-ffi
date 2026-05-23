import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.testing.Test

plugins { `java-library` }

repositories { mavenCentral() }

dependencies {
  testImplementation(platform("org.junit:junit-bom:6.0.3"))
  testImplementation("org.junit.jupiter:junit-jupiter")
  testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

tasks.withType<JavaCompile>().configureEach { options.release = 25 }

tasks.named<Javadoc>("javadoc") {
  isFailOnError = true
  options {
    encoding = "UTF-8"
    (this as StandardJavadocDocletOptions).apply {
      links("https://docs.oracle.com/en/java/javase/25/docs/api/")
      addStringOption("exclude", "org.maplibre.nativejni.internal:*")
    }
  }
}

val nativeJniLibraryPath = providers.systemProperty("org.maplibre.nativejni.library.path")
val nativeJniLibraryEnv = providers.environmentVariable("MAPLIBRE_NATIVE_JNI_LIBRARY_PATH")
val javaLibraryPath = providers.systemProperty("java.library.path")
val requireNativeTests = providers.systemProperty("org.maplibre.nativejni.tests.requireNative")

tasks.withType<Test>().configureEach {
  useJUnitPlatform()
  jvmArgs("--enable-native-access=ALL-UNNAMED")
  inputs.property("org.maplibre.nativejni.library.path", nativeJniLibraryPath.orElse(""))
  inputs.property("MAPLIBRE_NATIVE_JNI_LIBRARY_PATH", nativeJniLibraryEnv.orElse(""))
  inputs.property("java.library.path", javaLibraryPath.orElse(""))
  inputs.property("org.maplibre.nativejni.tests.requireNative", requireNativeTests.orElse(""))
  nativeJniLibraryPath.orNull?.let { systemProperty("org.maplibre.nativejni.library.path", it) }
  javaLibraryPath.orNull?.let { systemProperty("java.library.path", it) }
  requireNativeTests.orNull?.let {
    systemProperty("org.maplibre.nativejni.tests.requireNative", it)
  }
}
