package org.maplibre.nativeffi.examples.lwjglmap

import kotlin.system.exitProcess
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.RenderBackendFlag

internal object Main {
  @JvmStatic
  fun main(args: Array<String>) {
    val mode = parseArgs(args) ?: return
    val backends = GeneratedApi.supportedRenderBackendMask()
    println("native render backends: $backends")
    check(supportsUsableBackend(backends)) {
      "The loaded MapLibre native library does not support a backend usable by lwjgl-map"
    }
    GeneratedApi.logSetCallback { severity, event, code, message ->
      System.err.printf("MapLibre %s %s %d: %s%n", severity, event, code, message)
      1u
    }
    System.getProperty("org.maplibre.nativeffi.library.path")?.let {
      println("MapLibre native library: $it")
    }

    try {
      Shell.run(mode, backends)
    } finally {
      GeneratedApi.logClearCallback()
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

  private fun supportsUsableBackend(backends: RenderBackendFlag): Boolean =
    RenderBackendFlag.METAL in backends ||
      RenderBackendFlag.OPENGL in backends ||
      RenderBackendFlag.VULKAN in backends
}
