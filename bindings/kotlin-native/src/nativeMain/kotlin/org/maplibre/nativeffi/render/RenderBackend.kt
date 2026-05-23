package org.maplibre.nativeffi.render

/** Render backend support flag reported by the native library build. */
public enum class RenderBackend(public val nativeMask: UInt) {
  METAL(1U),
  VULKAN(1U shl 1);

  public companion object {
    public fun fromMask(mask: UInt): Set<RenderBackend> =
      entries.filterTo(mutableSetOf()) { (mask and it.nativeMask) != 0U }
  }
}
