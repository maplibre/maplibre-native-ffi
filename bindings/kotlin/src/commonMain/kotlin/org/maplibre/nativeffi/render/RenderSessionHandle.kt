package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.OperationHandle

/** Owned render session control handle. Driver methods remain graphics-thread-affine. */
public expect class RenderSessionHandle : AutoCloseable {
  public val isClosed: Boolean

  public fun map(): MapHandle

  public fun capabilities(): RenderSessionCapabilities

  public fun snapshot(): RenderSessionSnapshot

  public fun requestFrame(demand: FrameDemand = FrameDemand())

  public fun drainFrameResults(maxResults: Int = 0): List<RenderFrameResult>

  /** Services typed native work for a caller-graphics-thread driver. */
  public fun serviceDriverWork(maxWork: Int = 0): Int

  public fun acquireFrame(): AcquiredFrameHandle?

  public fun startResize(extent: RenderTargetExtent): OperationHandle<Unit>

  public fun startSetMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor): OperationHandle<Unit>

  public fun startSetVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor): OperationHandle<Unit>

  public fun startSetOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor): OperationHandle<Unit>

  public fun startSetMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): OperationHandle<Unit>

  public fun startSetVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): OperationHandle<Unit>

  public fun startSetOpenGLBorrowedTextureTarget(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): OperationHandle<Unit>

  public fun startReduceMemoryUse(): OperationHandle<Unit>

  public fun startClearData(): OperationHandle<Unit>

  public fun startDumpDebugLogs(): OperationHandle<Unit>

  public fun startBarrier(minimumUpdateGeneration: ULong): OperationHandle<Unit>

  public fun startDetach(): OperationHandle<Unit>

  public fun startSetFeatureState(
    selector: FeatureStateSelector,
    value: ByteArray,
  ): OperationHandle<Unit>

  public fun startGetFeatureState(selector: FeatureStateSelector): OperationHandle<ByteArray>

  public fun takeFeatureStateResult(operation: OperationHandle<ByteArray>): ByteArray

  public fun startRemoveFeatureState(selector: FeatureStateSelector): OperationHandle<Unit>

  public fun startQueryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): OperationHandle<ByteArray>

  public fun startQuerySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): OperationHandle<ByteArray>

  public fun startQueryFeatureExtension(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray?,
  ): OperationHandle<ByteArray>

  public fun takeQueryResult(operation: OperationHandle<ByteArray>): ByteArray

  public fun startReadPremultipliedRgba8(): OperationHandle<TextureReadback>

  public fun takeReadPremultipliedRgba8Result(
    operation: OperationHandle<TextureReadback>
  ): TextureReadback

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

  public fun release(consumerCompletion: GpuSync = GpuSync()): OperationHandle<Unit>
}
