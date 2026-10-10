package org.maplibre.nativeffi.examples.composemap

import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.unit.DpSize
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.WindowPosition
import androidx.compose.ui.window.application
import androidx.compose.ui.window.rememberWindowState
import kotlin.system.exitProcess
import kotlin.time.Duration.Companion.seconds
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.withTimeoutOrNull
import org.maplibre.nativeffi.examples.composemap.app.ComposeMapApp
import org.maplibre.nativeffi.examples.composemap.map.MapLibreNativeSurfaceAdapter
import org.maplibre.nativeffi.examples.composemap.map.MapLibreSurfaceRenderer
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LogHandler

internal object Main {
  @JvmStatic
  fun main(args: Array<String>) {
    require(args.isEmpty()) { "Usage: compose-map" }
    // A smoke run, which MLN_EXAMPLE_SMOKE=1 selects, renders an inline style, so it needs
    // neither the network nor a user, and exits once the first frame reaches the window.
    val smoke = System.getenv(SMOKE_VARIABLE) == "1"
    val rendered = CompletableDeferred<Unit>()
    val renderer =
      if (smoke) MapLibreSurfaceRenderer(SMOKE_STYLE) { rendered.complete(Unit) }
      else MapLibreSurfaceRenderer()
    GeneratedApi.logSetAsyncSeverityMask(org.maplibre.nativeffi.generated.LogSeverityMask(0u))
    GeneratedApi.logSetCallback(
      LogHandler { severity, event, code, message ->
        System.err.printf("MapLibre %s %s %d: %s%n", severity, event, code, message)
        1u
      }
    )
    System.getProperty("org.maplibre.nativeffi.library.path")?.let {
      println("MapLibre native library: $it")
    }
    println("native render backends: ${GeneratedApi.supportedRenderBackendMask()}")
    println("render target: compose-borrowed-texture")
    println(
      "render target status: renders into a host-owned texture, then samples it into the Compose/Skiko surface"
    )
    println("render driver: ${MapLibreNativeSurfaceAdapter.driverLabel}")
    printControls()

    try {
      application(exitProcessOnExit = false) {
        if (smoke) {
          // Compose draws only into a window, so the smoke run parks a borderless one that takes
          // no focus beyond the screen's edge, where it renders without appearing.
          Window(
            onCloseRequest = { exitApplication() },
            state =
              rememberWindowState(
                position = WindowPosition(SMOKE_OFFSCREEN, SMOKE_OFFSCREEN),
                size = DpSize(SMOKE_SIZE, SMOKE_SIZE),
              ),
            title = "MapLibre Compose Map smoke",
            undecorated = true,
            focusable = false,
          ) {
            ComposeMapApp(renderer)
          }
        } else {
          Window(onCloseRequest = { exitApplication() }, title = "MapLibre Compose Map") {
            ComposeMapApp(renderer)
          }
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

  private const val SMOKE_VARIABLE = "MLN_EXAMPLE_SMOKE"
  private val SMOKE_TIMEOUT = 60.seconds
  private val SMOKE_OFFSCREEN = (-4096).dp
  private val SMOKE_SIZE = 256.dp
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
