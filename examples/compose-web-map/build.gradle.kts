import org.jetbrains.kotlin.gradle.ExperimentalWasmDsl

plugins {
  id("org.jetbrains.kotlin.multiplatform")
  alias(libs.plugins.compose.multiplatform)
  alias(libs.plugins.compose.compiler)
}

repositories {
  google()
  mavenCentral()
}

kotlin {
  @OptIn(ExperimentalWasmDsl::class)
  wasmJs {
    outputModuleName = "composeWebMap"
    browser { commonWebpackConfig { outputFileName = "composeWebMap.js" } }
    binaries.executable()
  }

  sourceSets {
    wasmJsMain.dependencies {
      implementation(compose.runtime)
      implementation(compose.ui)
      implementation(compose.foundation)
      implementation(compose.material)
    }
  }
}

val nativeOutput =
  rootProject.layout.projectDirectory.dir("build/browser-wasm32-webgpu/examples/browser-map-native")
val copyNativeModule by
  tasks.registering(Sync::class) {
    from(nativeOutput) { include("browser-map.js", "browser-map.wasm") }
    into(layout.buildDirectory.dir("generated/nativeResources"))
  }

kotlin.sourceSets.named("wasmJsMain") {
  resources.srcDir(copyNativeModule.map { it.destinationDir })
}
