// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*

public expect abstract class GeneratedAcquiredFrameOperations internal constructor() {
  public fun dispose(): Unit

  public fun <T> withGetMetalTexture(block: (MetalOwnedTextureFrame) -> T): T

  public fun <T> withGetOpenglTexture(block: (OpenglOwnedTextureFrame) -> T): T

  public fun <T> withGetProducerSync(block: (GpuSync) -> T): T

  public fun getResult(): RenderFrameResult

  public fun <T> withGetVulkanTexture(block: (VulkanOwnedTextureFrame) -> T): T

  public fun <T> withGetWebgpuTexture(block: (WebgpuOwnedTextureFrame) -> T): T

  public fun release(consumerCompletion: GpuSync = GeneratedApi.gpuSyncDefault()): Unit
}
