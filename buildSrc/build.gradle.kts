import org.gradle.api.tasks.compile.JavaCompile
import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins { `kotlin-dsl` }

repositories {
  google()
  mavenCentral()
}

dependencies {
  implementation(gradleApi())
  implementation("com.android.tools.build:gradle:${libs.versions.agp.get()}")
  implementation("com.vanniktech:gradle-maven-publish-plugin:${libs.versions.maven.publish.get()}")
  implementation("org.jetbrains.kotlin:kotlin-gradle-plugin:${libs.versions.kotlin.get()}")
}

kotlin { compilerOptions { jvmTarget.set(JvmTarget.fromTarget(libs.versions.java.release.get())) } }

tasks.withType<JavaCompile>().configureEach {
  options.release = libs.versions.java.release.get().toInt()
}
