package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/**
 * Scaffold for the browser runtime handle.
 *
 * Every member throws. The actual exists so the `wasmJs` source set compiles while the browser
 * binding is filled in one file at a time; nothing here is finished work.
 */
public actual class RuntimeHandle private constructor() : AutoCloseable {
  public actual val isClosed: Boolean
    get() = throw NotImplementedError("wasmJs RuntimeHandle.isClosed is not implemented yet")

  public actual fun pump(timeoutMillis: Long) {
    throw NotImplementedError("wasmJs RuntimeHandle.pump is not implemented yet")
  }

  public actual fun acquireWakeSource(): WakeSource =
    throw NotImplementedError("wasmJs RuntimeHandle.acquireWakeSource is not implemented yet")

  public actual fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startAmbientCacheOperation is not implemented yet"
    )

  public actual fun startSetMaximumAmbientCacheSize(size: Long): OfflineOperationHandle<Unit> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startSetMaximumAmbientCacheSize is not implemented yet"
    )

  public actual fun startCreateOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startCreateOfflineRegion is not implemented yet"
    )

  public actual fun startOfflineRegion(id: Long): OfflineOperationHandle<OfflineRegionInfo?> =
    throw NotImplementedError("wasmJs RuntimeHandle.startOfflineRegion is not implemented yet")

  public actual fun startOfflineRegions(): OfflineOperationHandle<List<OfflineRegionInfo>> =
    throw NotImplementedError("wasmJs RuntimeHandle.startOfflineRegions is not implemented yet")

  public actual fun startMergeOfflineRegionsDatabase(
    path: String
  ): OfflineOperationHandle<List<OfflineRegionInfo>> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startMergeOfflineRegionsDatabase is not implemented yet"
    )

  public actual fun startUpdateOfflineRegionMetadata(
    id: Long,
    metadata: ByteArray,
  ): OfflineOperationHandle<OfflineRegionInfo> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startUpdateOfflineRegionMetadata is not implemented yet"
    )

  public actual fun startOfflineRegionStatus(
    id: Long
  ): OfflineOperationHandle<OfflineRegionStatus> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startOfflineRegionStatus is not implemented yet"
    )

  public actual fun startSetOfflineRegionObserved(
    id: Long,
    observed: Boolean,
  ): OfflineOperationHandle<Unit> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startSetOfflineRegionObserved is not implemented yet"
    )

  public actual fun startSetOfflineRegionDownloadState(
    id: Long,
    downloadState: OfflineRegionDownloadState,
  ): OfflineOperationHandle<Unit> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startSetOfflineRegionDownloadState is not implemented yet"
    )

  public actual fun startInvalidateOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startInvalidateOfflineRegion is not implemented yet"
    )

  public actual fun startDeleteOfflineRegion(id: Long): OfflineOperationHandle<Unit> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.startDeleteOfflineRegion is not implemented yet"
    )

  public actual fun takeCreateOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.takeCreateOfflineRegionResult is not implemented yet"
    )

  public actual fun takeOfflineRegionResult(
    operation: OfflineOperationHandle<OfflineRegionInfo?>
  ): OfflineRegionInfo? =
    throw NotImplementedError("wasmJs RuntimeHandle.takeOfflineRegionResult is not implemented yet")

  public actual fun takeOfflineRegionsResult(
    operation: OfflineOperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.takeOfflineRegionsResult is not implemented yet"
    )

  public actual fun takeMergeOfflineRegionsDatabaseResult(
    operation: OfflineOperationHandle<List<OfflineRegionInfo>>
  ): List<OfflineRegionInfo> =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.takeMergeOfflineRegionsDatabaseResult is not implemented yet"
    )

  public actual fun takeUpdateOfflineRegionMetadataResult(
    operation: OfflineOperationHandle<OfflineRegionInfo>
  ): OfflineRegionInfo =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.takeUpdateOfflineRegionMetadataResult is not implemented yet"
    )

  public actual fun takeOfflineRegionStatusResult(
    operation: OfflineOperationHandle<OfflineRegionStatus>
  ): OfflineRegionStatus =
    throw NotImplementedError(
      "wasmJs RuntimeHandle.takeOfflineRegionStatusResult is not implemented yet"
    )

  public actual fun setResourceProvider(callback: ResourceProviderCallback) {
    throw NotImplementedError("wasmJs RuntimeHandle.setResourceProvider is not implemented yet")
  }

  public actual fun clearResourceProvider() {
    throw NotImplementedError("wasmJs RuntimeHandle.clearResourceProvider is not implemented yet")
  }

  public actual fun setResourceTransform(callback: ResourceTransformCallback) {
    throw NotImplementedError("wasmJs RuntimeHandle.setResourceTransform is not implemented yet")
  }

  public actual fun clearResourceTransform() {
    throw NotImplementedError("wasmJs RuntimeHandle.clearResourceTransform is not implemented yet")
  }

  public actual fun setHttpHeaderTransform(callback: HttpHeaderTransformCallback) {
    throw NotImplementedError("wasmJs RuntimeHandle.setHttpHeaderTransform is not implemented yet")
  }

  public actual fun clearHttpHeaderTransform() {
    throw NotImplementedError(
      "wasmJs RuntimeHandle.clearHttpHeaderTransform is not implemented yet"
    )
  }

  public actual fun pollEvent(): RuntimeEvent? =
    throw NotImplementedError("wasmJs RuntimeHandle.pollEvent is not implemented yet")

  public actual override fun close() {
    throw NotImplementedError("wasmJs RuntimeHandle.close is not implemented yet")
  }

  public actual companion object {
    public actual fun create(options: RuntimeOptions): RuntimeHandle =
      throw NotImplementedError("wasmJs RuntimeHandle.create is not implemented yet")
  }
}
