package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertIs
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus

class RuntimeOfflineConformanceTest {
  private val drained = mutableListOf<RuntimeEvent>()

  @Test
  fun offlineRegionApisCreateObserveAndCopyPublicEvents(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
      try {
        val definition = tileDefinition()
        val createMetadata = byteArrayOf(1, 2, 3)
        val createOperation = runtime.startCreateOfflineRegion(definition, createMetadata)
        createMetadata[0] = 9
        waitForOperation(runtime, createOperation)
        val created = runtime.takeCreateOfflineRegionResult(createOperation)
        createOperation.close()
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
        updateOperation.close()
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
        // A drained value stays readable after the next drain ends the batch window.
        val copiedMessage = observed.message
        runtime.drainEvents()
        assertEquals(copiedMessage, observed.message)

        completeVoidOperation(runtime, runtime.startSetOfflineRegionObserved(created.id, false))
        completeVoidOperation(
          runtime,
          runtime.startSetOfflineRegionDownloadState(
            created.id,
            OfflineRegionDownloadState.INACTIVE,
          ),
        )
        completeVoidOperation(runtime, runtime.startInvalidateOfflineRegion(created.id))
        completeVoidOperation(runtime, runtime.startDeleteOfflineRegion(created.id))
        assertNull(offlineRegion(runtime, created.id))
      } finally {
        runtime.close()
      }
    }

  private suspend fun waitForOperation(runtime: RuntimeHandle, operation: OperationHandle<*>) {
    repeat(10_000) {
      if (operation.poll()) {
        return
      }
      runtime.barrier()
    }
    error("operation did not complete")
  }

  private suspend fun completeVoidOperation(
    runtime: RuntimeHandle,
    operation: OperationHandle<Unit>,
  ) {
    waitForOperation(runtime, operation)
    operation.discard()
    operation.close()
  }

  private suspend fun offlineRegion(runtime: RuntimeHandle, id: Long): OfflineRegionInfo? {
    val operation = runtime.startOfflineRegion(id)
    waitForOperation(runtime, operation)
    return try {
      runtime.takeOfflineRegionResult(operation)
    } finally {
      operation.close()
    }
  }

  private suspend fun offlineRegions(runtime: RuntimeHandle): List<OfflineRegionInfo> {
    val operation = runtime.startOfflineRegions()
    waitForOperation(runtime, operation)
    return try {
      runtime.takeOfflineRegionsResult(operation)
    } finally {
      operation.close()
    }
  }

  private suspend fun offlineRegionStatus(runtime: RuntimeHandle, id: Long): OfflineRegionStatus {
    val operation = runtime.startOfflineRegionStatus(id)
    waitForOperation(runtime, operation)
    return try {
      runtime.takeOfflineRegionStatusResult(operation)
    } finally {
      operation.close()
    }
  }

  private suspend fun waitForObservedOfflineRegionEvent(
    runtime: RuntimeHandle,
    regionId: Long,
  ): RuntimeEvent {
    repeat(10_000) {
      drain(runtime)
      for (event in drained) {
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
      runtime.barrier()
    }
    error("offline region observation event did not arrive for region $regionId")
  }

  /**
   * Drains once and keeps every event this test has observed. One offline step drains the events of
   * the steps before it, so the waiters below scan what the whole test has seen rather than one
   * batch.
   */
  private suspend fun drain(runtime: RuntimeHandle) {
    runtime.barrier()
    drained += runtime.drainEvents().events
  }

  private fun assertObservedOfflineRegionStatusPayload(
    regionId: Long,
    payload: RuntimeEventPayload,
  ) {
    val statusChanged = assertIs<RuntimeEventPayload.OfflineRegionStatusChanged>(payload)
    assertEquals(regionId, statusChanged.regionId)
    assertEquals(OfflineRegionDownloadState.ACTIVE, statusChanged.status.downloadState)
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
