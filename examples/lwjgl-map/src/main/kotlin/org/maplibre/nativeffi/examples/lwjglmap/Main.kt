package org.maplibre.nativeffi.examples.lwjglmap

import kotlin.system.exitProcess
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.render.RenderBackend

internal object Main {
  @JvmStatic
  fun main(args: Array<String>) {
    val mode = parseArgs(args) ?: return
    val backends = Maplibre.supportedRenderBackends()
    println("native render backends: $backends")
    check(supportsUsableBackend(backends)) {
      "The loaded MapLibre native library does not support a backend usable by lwjgl-map"
    }
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
      Shell.run(mode, backends)
    } finally {
      Maplibre.clearLogCallback()
    }
  }

  private fun parseArgs(args: Array<String>): RenderTargetMode? {
    if (args.size == 1 && args[0] == "--help") {
      printUsage()
      return null
    }
    if (args.size != 1 || args[0].startsWith("-")) {
      printUsage()
      exitProcess(1)
    }
    return try {
      RenderTargetMode.parse(args[0])
    } catch (error: IllegalArgumentException) {
      System.err.println(error.message)
      printUsage()
      exitProcess(1)
    }
  }

  private fun printUsage() {
    System.err.println(
      """
      Usage: lwjgl-map <mode>

      Modes:
        owned-texture     session-owned texture render target
        borrowed-texture  caller-owned texture render target
        native-surface    native surface render target
      """
        .trimIndent()
    )
  }

  private fun supportsUsableBackend(backends: Set<RenderBackend>): Boolean =
    RenderBackend.METAL in backends ||
      RenderBackend.OPENGL in backends ||
      RenderBackend.VULKAN in backends
}
