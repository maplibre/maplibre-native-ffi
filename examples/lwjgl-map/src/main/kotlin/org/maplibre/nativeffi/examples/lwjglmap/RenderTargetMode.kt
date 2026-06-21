package org.maplibre.nativeffi.examples.lwjglmap

internal enum class RenderTargetMode(private val cliNameValue: String) {
  OWNED_TEXTURE("owned-texture"),
  BORROWED_TEXTURE("borrowed-texture"),
  NATIVE_SURFACE("native-surface");

  fun cliName(): String = cliNameValue

  fun status(): String =
    when (this) {
      OWNED_TEXTURE -> "samples MapLibre-owned texture frames into the host swapchain"
      BORROWED_TEXTURE ->
        "renders into a host-owned texture, then samples it into the host swapchain"
      NATIVE_SURFACE -> "renders directly to the host window surface"
    }

  companion object {
    @JvmStatic
    fun parse(value: String): RenderTargetMode =
      entries.firstOrNull { it.cliNameValue == value }
        ?: throw IllegalArgumentException("unknown render target '$value'")
  }
}
