package org.maplibre.nativeffi.render

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Owned render session control handle. Driver methods remain graphics-thread-affine. */
public expect class RenderSessionHandle : AutoCloseable {
  public val isClosed: Boolean

  public fun map(): MapHandle

  public fun capabilities(): RenderSessionCapabilities

  public fun snapshot(): RenderSessionSnapshot

  public fun requestFrame(demand: FrameDemand = FrameDemand())

  public fun drainFrameResults(): List<RenderFrameResult>

  /** Services typed native work for a caller-graphics-thread driver. */
  public fun serviceDriverWork(maxWork: Int = 0): Int

  public fun acquireFrame(): AcquiredFrameHandle?

  public fun resize(extent: RenderTargetExtent): Deferred<Unit>

  public fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor): Deferred<Unit>

  public fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor): Deferred<Unit>

  public fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor): Deferred<Unit>

  public fun setMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): Deferred<Unit>

  public fun setVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): Deferred<Unit>

  public fun setOpenGLBorrowedTextureTarget(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): Deferred<Unit>

  public fun reduceMemoryUse(): Deferred<Unit>

  public fun clearData(): Deferred<Unit>

  public fun dumpDebugLogs(): Deferred<Unit>

  public fun barrier(): Deferred<Unit>

  public fun detach(): Deferred<Unit>

  public fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>>

  public fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): Deferred<List<QueriedFeature>>

  public fun queryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): Deferred<ByteArray>

  public fun readPremultipliedRgba8(): Deferred<TextureReadback>

  /** Irreversibly abandons a lost target without graphics calls. */
  public fun abandon(): RenderAbandonResult

  /** Destroys detached or abandoned CPU-side session state. */
  override fun close()
}

/** An acquired texture-ring slot. Accessors become invalid after release or abandonment. */
public expect class AcquiredFrameHandle {
  public val isReleased: Boolean

  public fun result(): RenderFrameResult

  public fun producerSync(): GpuSync

  public fun metalTexture(): MetalOwnedTextureFrame

  public fun vulkanTexture(): VulkanOwnedTextureFrame

  public fun openGLTexture(): OpenGLOwnedTextureFrame

  public fun release(consumerCompletion: GpuSync = GpuSync())
}
