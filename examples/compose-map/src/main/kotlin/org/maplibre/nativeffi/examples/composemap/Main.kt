package org.maplibre.nativeffi.examples.composemap

import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application
import kotlin.system.exitProcess
import kotlin.time.Duration.Companion.seconds
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.withTimeoutOrNull
import org.maplibre.nativeffi.examples.composemap.app.ComposeMapApp
import org.maplibre.nativeffi.examples.composemap.map.MapLibreSurfaceRenderer
import org.maplibre.nativeffi.generated.GeneratedApi

internal object Main {
  @JvmStatic
  fun main(args: Array<String>) {
    // A smoke run renders an inline style, so it needs neither the network nor a user, and
    // exits once the first frame reaches the window.
    val smoke = args.contentEquals(arrayOf(SMOKE_FLAG))
    require(smoke || args.isEmpty()) { "Usage: compose-map [$SMOKE_FLAG]" }
    val rendered = CompletableDeferred<Unit>()
    val renderer =
      if (smoke) MapLibreSurfaceRenderer(SMOKE_STYLE) { rendered.complete(Unit) }
      else MapLibreSurfaceRenderer()
    GeneratedApi.logSetAsyncSeverityMask(org.maplibre.nativeffi.generated.LogSeverityMask(0u))
    GeneratedApi.logSetCallback { severity, event, code, message ->
      System.err.printf("MapLibre %s %s %d: %s%n", severity, event, code, message)
      1u
    }
    System.getProperty("org.maplibre.nativeffi.library.path")?.let {
      println("MapLibre native library: $it")
    }
    println("native render backends: ${GeneratedApi.supportedRenderBackendMask()}")
    println("render target: compose-borrowed-texture")
    println(
      "render target status: renders into a host-owned texture, then samples it into the Compose/Skiko surface"
    )
    printControls()

    try {
      application(exitProcessOnExit = false) {
        Window(onCloseRequest = { exitApplication() }, title = "MapLibre Compose Map") {
          ComposeMapApp(renderer)
        }
        if (smoke) {
          LaunchedEffect(rendered) {
            smokeRendered = withTimeoutOrNull(SMOKE_TIMEOUT) { rendered.await() } != null
            println(
              if (smokeRendered) "smoke: rendered a frame" else "smoke: no frame reached the window"
            )
            exitApplication()
          }
        }
      }
    } finally {
      GeneratedApi.logClearCallback()
    }
    if (smoke && !smokeRendered) exitProcess(1)
  }

  @Volatile private var smokeRendered = false

  private const val SMOKE_FLAG = "--smoke"
  private val SMOKE_TIMEOUT = 60.seconds
  private const val SMOKE_STYLE =
    """{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#2a6f97"}}]}"""

  private fun printControls() {
    println(
      """
      Controls:
        left drag: pan
        right drag or Ctrl+left drag: rotate with X, pitch with Y
        scroll: zoom at cursor
        arrows or WASD: pan
        + / -: zoom at center
        Q / E: rotate
        ] / [: pitch
        0: reset pitch and bearing
      """
        .trimIndent()
    )
  }
}
