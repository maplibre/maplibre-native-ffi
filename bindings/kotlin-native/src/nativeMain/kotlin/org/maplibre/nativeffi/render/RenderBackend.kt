package org.maplibre.nativeffi.render

/** Render backend support flag reported by the native library build. */
public enum class RenderBackend(public val nativeMask: Int) {
  METAL(1),
  VULKAN(1 shl 1),
  OPENGL(1 shl 2);

  public companion object {
    internal fun fromMask(mask: UInt): Set<RenderBackend> = fromMask(mask.toInt())

    public fun fromMask(mask: Int): Set<RenderBackend> =
      entries.filterTo(mutableSetOf()) { (mask and it.nativeMask) != 0 }
  }
}
