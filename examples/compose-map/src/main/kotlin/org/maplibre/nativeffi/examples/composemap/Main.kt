package org.maplibre.nativeffi.examples.composemap

import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application
import org.maplibre.nativeffi.examples.composemap.app.ComposeMapApp
import org.maplibre.nativeffi.examples.composemap.map.MapLibreSurfaceRenderer
import org.maplibre.nativeffi.generated.GeneratedApi

internal object Main {
  @JvmStatic
  fun main(args: Array<String>) {
    val renderer = MapLibreSurfaceRenderer()
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
      }
    } finally {
      GeneratedApi.logClearCallback()
    }
  }

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
