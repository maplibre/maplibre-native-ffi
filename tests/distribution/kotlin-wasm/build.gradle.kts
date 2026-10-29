import java.time.Duration
import org.jetbrains.kotlin.gradle.ExperimentalWasmDsl
import org.jetbrains.kotlin.gradle.targets.js.testing.KotlinJsTest

plugins { kotlin("multiplatform") version libs.versions.kotlin }

val repository = providers.gradleProperty("maplibre.repository").get()
val version = providers.gradleProperty("maplibre.version").get()
val backend = providers.gradleProperty("maplibre.backend").get()

// Verify each backend with its own npm workspace.
layout.buildDirectory.set(layout.projectDirectory.dir("build/$backend"))

repositories {
  exclusiveContent {
    forRepository { maven(repository) }
    filter { includeGroup("org.maplibre.nativeffi") }
  }
  mavenCentral()
}

kotlin {
  @OptIn(ExperimentalWasmDsl::class)
  wasmJs { browser { testTask { useKarma { useChromeHeadless() } } } }
  sourceSets.wasmJsTest.dependencies {
    implementation(kotlin("test"))
    implementation(libs.coroutines.test)
    implementation("org.maplibre.nativeffi:maplibre-native-ffi:$version")
    val runtime = if (backend == "webgl") "opengl" else "webgpu"
    implementation("org.maplibre.nativeffi:maplibre-native-ffi-runtime-$runtime:$version")
  }
}

tasks.withType<KotlinJsTest>().configureEach {
  timeout.set(Duration.ofMinutes(3))
  environment("CHROME_BIN", providers.gradleProperty("maplibre.chromium").get())
  environment("MLN_BROWSER_BACKEND", backend)
}
