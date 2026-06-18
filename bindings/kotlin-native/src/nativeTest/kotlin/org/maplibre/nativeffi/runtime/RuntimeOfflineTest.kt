package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import platform.posix.usleep

class RuntimeOfflineTest {
  private val deferredEvents = ArrayDeque<RuntimeEvent>()

  @Test
  fun processGlobalNetworkStatusRoundTrips() {
    val original = Maplibre.networkStatus
    try {
      Maplibre.setNetworkStatus(NetworkStatus.OFFLINE)
      assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus)
      Maplibre.setNetworkStatus(NetworkStatus.ONLINE)
      assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus)
    } finally {
      Maplibre.setNetworkStatus(original)
    }
  }

  @Test
  fun offlineDownloadStateUnknownRawValueRejectsBeforeNativeCall() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      assertFailsWith<IllegalArgumentException> {
        runtime.startSetOfflineRegionDownloadState(1, OfflineRegionDownloadState(900))
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun ambientCacheOperationHandleDiscardsOnce() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      val operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.INVALIDATE)
      assertFalse(operation.isClosed)
      assertEquals(OfflineOperationKind.AMBIENT_CACHE, operation.kind)
      assertEquals(OfflineOperationResultKind.NONE, operation.resultKind)

      operation.close()
      assertTrue(operation.isClosed)
      operation.close()
      assertTrue(operation.isClosed)
    } finally {
      runtime.close()
    }
  }

  @Test
  fun offlineRegionApisCreateObserveAndCopyPublicEvents() {
    val runtime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
    try {
      val definition = tileDefinition()
      val createMetadata = byteArrayOf(1, 2, 3)
      val createOperation = runtime.startCreateOfflineRegion(definition, createMetadata)
      createMetadata[0] = 9
      waitForOperation(runtime, createOperation)
      val created = runtime.takeCreateOfflineRegionResult(createOperation)
      assertTrue(created.id > 0)
      assertEquals(definition, created.definition)
      assertContentEquals(byteArrayOf(1, 2, 3), created.metadata)
      val copiedMetadata = created.metadata
      copiedMetadata[0] = 9
      assertContentEquals(byteArrayOf(1, 2, 3), created.metadata)

      assertEquals(created, offlineRegion(runtime, created.id))
      assertTrue(offlineRegions(runtime).contains(created))

      val updateMetadata = byteArrayOf(4, 5)
      val updateOperation = runtime.startUpdateOfflineRegionMetadata(created.id, updateMetadata)
      updateMetadata[0] = 9
      waitForOperation(runtime, updateOperation)
      val updated = runtime.takeUpdateOfflineRegionMetadataResult(updateOperation)
      assertEquals(created.id, updated.id)
      assertContentEquals(byteArrayOf(4, 5), updated.metadata)

      val status = offlineRegionStatus(runtime, created.id)
      assertEquals(OfflineRegionDownloadState.INACTIVE, status.downloadState)

      completeVoidOperation(runtime, runtime.startSetOfflineRegionObserved(created.id, true))
      completeVoidOperation(
        runtime,
        runtime.startSetOfflineRegionDownloadState(created.id, OfflineRegionDownloadState.ACTIVE),
      )
      val observed = waitForObservedOfflineRegionEvent(runtime, created.id)
      assertEquals(RuntimeEventSourceType.RUNTIME, observed.sourceType)
      assertEquals(runtime, observed.runtimeSource)
      assertNull(observed.mapSource)
      assertObservedOfflineRegionStatusPayload(created.id, observed.payload)
      val copiedMessage = observed.message
      runtime.pollEvent()
      assertEquals(copiedMessage, observed.message)

      completeVoidOperation(runtime, runtime.startSetOfflineRegionObserved(created.id, false))
      completeVoidOperation(
        runtime,
        runtime.startSetOfflineRegionDownloadState(created.id, OfflineRegionDownloadState.INACTIVE),
      )
      completeVoidOperation(runtime, runtime.startInvalidateOfflineRegion(created.id))
      completeVoidOperation(runtime, runtime.startDeleteOfflineRegion(created.id))
      assertNull(offlineRegion(runtime, created.id))
    } finally {
      runtime.close()
    }
  }

  private fun waitForOperation(
    runtime: RuntimeHandle,
    operation: OfflineOperationHandle<*>,
  ): RuntimeEventPayload.OfflineOperationCompleted {
    repeat(10_000) {
      runtime.runOnce()
      while (true) {
        val event = pollEvent(runtime) ?: break
        val completed = event.payload as? RuntimeEventPayload.OfflineOperationCompleted
        if (completed == null) {
          deferredEvents.addLast(event)
          continue
        }
        if (completed.operationId != operation.id) {
          deferredEvents.addLast(event)
          continue
        }
        assertEquals(operation.kind, completed.operationKind)
        assertEquals(operation.kind.nativeValue, completed.operationKind.nativeValue)
        assertEquals(operation.resultKind, completed.resultKind)
        assertEquals(operation.resultKind.nativeValue, completed.resultKind.nativeValue)
        if (completed.resultStatus != MaplibreStatus.OK.nativeCode) {
          throw MaplibreException.forStatus(
            MaplibreStatus.fromNative(completed.resultStatus),
            completed.resultStatus,
            event.message,
          )
        }
        return completed
      }
      usleep(1_000U)
    }
    error("offline operation did not complete: ${operation.id}")
  }

  private fun completeVoidOperation(
    runtime: RuntimeHandle,
    operation: OfflineOperationHandle<Unit>,
  ) {
    waitForOperation(runtime, operation)
    operation.close()
  }

  private fun offlineRegion(runtime: RuntimeHandle, id: Long): OfflineRegionInfo? {
    val operation = runtime.startOfflineRegion(id)
    waitForOperation(runtime, operation)
    return runtime.takeOfflineRegionResult(operation)
  }

  private fun offlineRegions(runtime: RuntimeHandle): List<OfflineRegionInfo> {
    val operation = runtime.startOfflineRegions()
    waitForOperation(runtime, operation)
    return runtime.takeOfflineRegionsResult(operation)
  }

  private fun offlineRegionStatus(runtime: RuntimeHandle, id: Long): OfflineRegionStatus {
    val operation = runtime.startOfflineRegionStatus(id)
    waitForOperation(runtime, operation)
    return runtime.takeOfflineRegionStatusResult(operation)
  }

  private fun waitForObservedOfflineRegionEvent(
    runtime: RuntimeHandle,
    regionId: Long,
  ): RuntimeEvent {
    repeat(10_000) {
      runtime.runOnce()
      while (true) {
        val event = pollEvent(runtime) ?: break
        when (val payload = event.payload) {
          is RuntimeEventPayload.OfflineRegionStatusChanged ->
            if (payload.regionId == regionId) return event
          is RuntimeEventPayload.OfflineRegionResponseError ->
            if (payload.regionId == regionId) return event
          is RuntimeEventPayload.OfflineRegionTileCountLimit ->
            if (payload.regionId == regionId) return event
          else -> Unit
        }
      }
      usleep(1_000U)
    }
    error("offline region observation event did not arrive for region $regionId")
  }

  private fun pollEvent(runtime: RuntimeHandle): RuntimeEvent? =
    if (deferredEvents.isNotEmpty()) deferredEvents.removeFirst() else runtime.pollEvent()

  private fun assertObservedOfflineRegionStatusPayload(
    regionId: Long,
    payload: RuntimeEventPayload,
  ) {
    val statusChanged = assertIs<RuntimeEventPayload.OfflineRegionStatusChanged>(payload)
    assertEquals(regionId, statusChanged.regionId)
    assertTrue(statusChanged.status.downloadState.nativeValue >= 0)
    assertTrue(statusChanged.status.completedResourceCount >= 0)
    assertTrue(statusChanged.status.completedResourceSize >= 0)
    assertTrue(statusChanged.status.completedTileCount >= 0)
    assertTrue(statusChanged.status.completedTileSize >= 0)
    assertTrue(statusChanged.status.requiredTileCount >= 0)
  }

  private fun tileDefinition(): OfflineRegionDefinition.TilePyramid =
    OfflineRegionDefinition.TilePyramid(
      "custom://offline-style.json",
      LatLngBounds(LatLng(0.0, 0.0), LatLng(1.0, 1.0)),
      0.0,
      1.0,
      1.0f,
      true,
    )
}
