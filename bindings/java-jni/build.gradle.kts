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

tasks.withType<Test>().configureEach {
  useJUnitPlatform()
  inputs.property("org.maplibre.nativejni.library.path", nativeJniLibraryPath.orElse(""))
  nativeJniLibraryPath.orNull?.let { systemProperty("org.maplibre.nativejni.library.path", it) }
}
