import org.gradle.api.tasks.compile.JavaCompile

plugins {
  `java-library`
}

tasks.withType<JavaCompile>().configureEach {
  options.release = 25
}
