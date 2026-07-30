import org.gradle.api.DefaultTask
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.TaskAction
import org.gradle.work.DisableCachingByDefault
import org.jetbrains.kotlin.gradle.ExperimentalWasmDsl
import org.jetbrains.kotlin.gradle.targets.wasm.nodejs.WasmNodeJsRootExtension
import org.jetbrains.kotlin.gradle.targets.wasm.nodejs.WasmNodeJsRootPlugin
import org.jetbrains.kotlin.gradle.targets.wasm.npm.WasmNpmExtension

plugins {
  id("org.jetbrains.kotlin.multiplatform")
  alias(libs.plugins.compose.multiplatform)
  alias(libs.plugins.compose.compiler)
}

// This repository uses pnpm for its JavaScript workspace. Kotlin/Wasm's Yarn 1
// runner interprets the root packageManager entry as a Yarn version, so use the
// plugin's npm runner for this standalone Webpack bundle.
rootProject.plugins.withType<WasmNodeJsRootPlugin> {
  val node = rootProject.extensions.getByType<WasmNodeJsRootExtension>()
  node.packageManagerExtension.set(rootProject.extensions.getByType<WasmNpmExtension>())
}

repositories {
  google()
  mavenCentral()
  maven("https://maven.pkg.jetbrains.space/public/p/compose/dev")
}

// Capture the standard Skiko WebGL objects in its generated module glue. This
// is the browser analogue of the reflection used by the desktop prototype and
// keeps Compose and Skiko on their normal, mutually compatible versions.
@DisableCachingByDefault(because = "Patches generated Skiko output in place")
abstract class PatchSkikoRuntime : DefaultTask() {
  @get:OutputFile abstract val moduleFile: RegularFileProperty

  @TaskAction
  fun patch() {
    val module = moduleFile.get().asFile
    var source = module.readText()
    if (!source.contains("__composeSkiaDirectContext")) {
      val contextSymbol = "org_jetbrains_skia_DirectContext__1nMakeGL"
      val original =
        "export let $contextSymbol = (...a) => ($contextSymbol = " +
          "loadedWasm._[\"$contextSymbol\"])(...a)"
      val replacement =
        "export let $contextSymbol = (...a) => { const result = " +
          "($contextSymbol = loadedWasm._[\"$contextSymbol\"])(...a); " +
          "globalThis.__composeSkiaDirectContext = result; return result }"
      check(source.contains(original)) { "Skiko DirectContext wrapper changed" }
      source = source.replace(original, replacement)
      source += "\nglobalThis.__composeSkikoGL = GL\n"
      module.writeText(source)
    }
  }
}

val patchSkikoRuntime =
  tasks.register<PatchSkikoRuntime>("patchSkikoRuntime") {
    moduleFile.set(layout.buildDirectory.file("compose/skiko-runtime-processed-wasmjs/skiko.mjs"))
    dependsOn("processSkikoRuntimeForKWasm")
  }

tasks
  .matching { it.name == "wasmJsDevelopmentExecutableCompileSync" }
  .configureEach { dependsOn(patchSkikoRuntime) }

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
  rootProject.layout.projectDirectory.dir(
    "build/browser-wasm32-webgl/examples/compose-web-map-native"
  )
val copyNativeModule by
  tasks.registering(Sync::class) {
    from(nativeOutput) { include("maplibre-compose.js", "maplibre-compose.wasm") }
    into(layout.buildDirectory.dir("generated/nativeResources"))
  }

kotlin.sourceSets.named("wasmJsMain") {
  resources.srcDir(copyNativeModule.map { it.destinationDir })
}
