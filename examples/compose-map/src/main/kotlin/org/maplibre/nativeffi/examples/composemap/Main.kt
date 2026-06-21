package org.maplibre.nativeffi.examples.composemap

import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.examples.composemap.app.ComposeMapApp

internal object Main {
  @JvmStatic
  fun main(args: Array<String>) {
    Maplibre.setLogCallback { record ->
      System.err.printf(
        "MapLibre %s %s %d: %s%n",
        record.severity,
        record.event,
        record.code,
        record.message,
      )
      true
    }
    System.getProperty("org.maplibre.nativeffi.library.path")?.let {
      println("MapLibre native library: $it")
    }

    try {
      application {
        Window(onCloseRequest = ::exitApplication, title = "MapLibre Compose Map") {
          ComposeMapApp()
        }
      }
    } finally {
      Maplibre.clearLogCallback()
    }
  }
}
