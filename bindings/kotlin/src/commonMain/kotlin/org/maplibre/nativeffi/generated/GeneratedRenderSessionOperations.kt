// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.runtime.CommandCompletion

public expect abstract class GeneratedRenderSessionOperations internal constructor() {
  public fun metalBorrowedTextureSetTarget(
    descriptor: MetalBorrowedTextureDescriptor
  ): Deferred<Unit>

  public fun metalSurfaceSetTarget(descriptor: MetalSurfaceDescriptor): Deferred<Unit>

  public fun openglBorrowedTextureSetTarget(
    descriptor: OpenglBorrowedTextureDescriptor
  ): Deferred<Unit>

  public fun openglSurfaceSetTarget(descriptor: OpenglSurfaceDescriptor): Deferred<Unit>

  public fun abandon(): RenderAbandonResult

  public fun acquireFrame(): org.maplibre.nativeffi.render.AcquiredFrameHandle

  public fun barrier(): Deferred<Unit>

  public fun clearData(): Deferred<Unit>

  public fun destroy(): Unit

  public fun detach(): Deferred<Unit>

  public fun dispose(): Unit

  public fun drainFrameResults(): org.maplibre.nativeffi.generated.RenderFrameBatchHandle

  public fun dumpDebugLogs(): Deferred<Unit>

  public fun getCapabilities(): RenderSessionCapabilities

  public fun getSnapshot(): RenderSessionSnapshot

  public fun projectionCreate(): org.maplibre.nativeffi.map.MapProjectionHandle

  public fun queryFeatureExtensions(
    sourceId: String,
    feature: ByteArray,
    extension: String,
    extensionField: String,
    arguments: ByteArray? = null,
  ): Deferred<ByteArray>

  public fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions? = null,
  ): Deferred<List<QueriedFeature>>

  public fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions? = null,
  ): Deferred<List<QueriedFeature>>

  public fun reduceMemoryUse(): Deferred<Unit>

  public fun requestFrame(demand: FrameDemand): Unit

  public fun resize(extent: RenderTargetExtent): Deferred<CommandCompletion>

  public fun serviceDriverWork(maxWork: ULong): ULong

  public fun textureReadPremultipliedRgba8(): Deferred<TextureReadbackResult>

  public fun vulkanBorrowedTextureSetTarget(
    descriptor: VulkanBorrowedTextureDescriptor
  ): Deferred<Unit>

  public fun vulkanSurfaceSetTarget(descriptor: VulkanSurfaceDescriptor): Deferred<Unit>

  public fun webgpuBorrowedTextureSetTarget(
    descriptor: WebgpuBorrowedTextureDescriptor
  ): Deferred<Unit>

  public fun webgpuSurfaceSetTarget(descriptor: WebgpuSurfaceDescriptor): Deferred<Unit>
}
