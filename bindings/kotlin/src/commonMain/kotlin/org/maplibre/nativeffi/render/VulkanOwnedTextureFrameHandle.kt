package org.maplibre.nativeffi.render

/** Explicit handle for a Vulkan session-owned texture frame. */
public expect class VulkanOwnedTextureFrameHandle : AutoCloseable {
  public fun frame(): VulkanOwnedTextureFrame

  public val isClosed: Boolean

  override fun close()
}
