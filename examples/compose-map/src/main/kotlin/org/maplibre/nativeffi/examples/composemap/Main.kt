package org.maplibre.nativeffi.examples.composemap

import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application
import java.awt.Desktop
import java.util.concurrent.atomic.AtomicBoolean
import javax.swing.SwingUtilities
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.examples.composemap.app.ComposeMapApp
import org.maplibre.nativeffi.examples.composemap.map.MapLibreSurfaceRenderer
import org.maplibre.nativeffi.examples.composemap.surface.SkikoHost

internal object Main {
  @JvmStatic
  fun main(args: Array<String>) {
    val renderer = MapLibreSurfaceRenderer()
    val cleanedUp = AtomicBoolean(false)
    val cleanup = { cleanupOnce(renderer, cleanedUp) }
    Maplibre.setAsyncLogSeverities(emptySet())
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
    println("native render backends: ${Maplibre.supportedRenderBackends().joinToString()}")
    println("render target: compose-borrowed-texture")
    println(
      "render target status: renders into a host-owned texture, then samples it into the Compose/Skiko surface"
    )
    printControls()
    installQuitHandler(cleanup)

    try {
      application(exitProcessOnExit = false) {
        Window(
          onCloseRequest = {
            cleanup()
            exitApplication()
          },
          title = "MapLibre Compose Map",
        ) {
          ComposeMapApp(renderer)
        }
      }
    } finally {
      cleanup()
    }
  }

  private fun installQuitHandler(cleanup: () -> Unit) {
    if (!Desktop.isDesktopSupported()) {
      return
    }
    val desktop = Desktop.getDesktop()
    if (!desktop.isSupported(Desktop.Action.APP_QUIT_HANDLER)) {
      return
    }
    desktop.setQuitHandler { _, response ->
      try {
        cleanup()
        response.performQuit()
      } catch (error: Throwable) {
        error.printStackTrace()
        response.cancelQuit()
      }
    }
  }

  private fun cleanupOnce(renderer: MapLibreSurfaceRenderer, cleanedUp: AtomicBoolean) {
    if (!cleanedUp.compareAndSet(false, true)) {
      return
    }
    try {
      if (SwingUtilities.isEventDispatchThread()) {
        cleanup(renderer)
      } else {
        SwingUtilities.invokeAndWait { cleanup(renderer) }
      }
    } catch (error: Throwable) {
      cleanedUp.set(false)
      throw error
    }
  }

  private fun cleanup(renderer: MapLibreSurfaceRenderer) {
    try {
      renderer.close()
    } finally {
      try {
        SkikoHost.close()
      } finally {
        Maplibre.clearLogCallback()
      }
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
