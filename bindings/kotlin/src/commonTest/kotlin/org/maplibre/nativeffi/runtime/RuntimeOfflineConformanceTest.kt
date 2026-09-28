package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertIs
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.LatLngBounds
import org.maplibre.nativeffi.generated.OfflineRegionDefinition
import org.maplibre.nativeffi.generated.OfflineRegionDefinitionData
import org.maplibre.nativeffi.generated.OfflineRegionDownloadState
import org.maplibre.nativeffi.generated.OfflineTilePyramidRegionDefinition
import org.maplibre.nativeffi.generated.RuntimeEvent
import org.maplibre.nativeffi.generated.RuntimeEventPayload
import org.maplibre.nativeffi.generated.RuntimeEventSourceType

class RuntimeOfflineConformanceTest {
  private val drained = mutableListOf<RuntimeEvent>()

  @Test
  fun offlineRegionApisCreateObserveAndCopyPublicEvents(): Unit = runSuspendTest {
    val runtime =
      GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:"))
    try {
      val definition = tileDefinition()
      val createMetadata = byteArrayOf(1, 2, 3)
      val create = runtime.offlineRegionCreate(definition, createMetadata)
      createMetadata[0] = 9
      val created = create.await()
      assertTrue(created.id > 0)
      assertEquals(definition, created.definition)
      assertContentEquals(byteArrayOf(1, 2, 3), created.metadata)

      assertContentEquals(created.metadata, runtime.offlineRegionGet(created.id).await()!!.metadata)
      assertTrue(
        runtime.offlineRegionsList().await().any {
          it.id == created.id && it.definition == definition
        }
      )

      val updateMetadata = byteArrayOf(4, 5)
      val update = runtime.offlineRegionUpdateMetadata(created.id, updateMetadata)
      updateMetadata[0] = 9
      val updated = update.await()
      assertEquals(created.id, updated.id)
      assertContentEquals(byteArrayOf(4, 5), updated.metadata)

      val status = runtime.offlineRegionGetStatus(created.id).await()
      assertEquals(OfflineRegionDownloadState.INACTIVE, status.downloadState)

      runtime.offlineRegionSetObserved(created.id, true).await()
      runtime.offlineRegionSetDownloadState(created.id, OfflineRegionDownloadState.ACTIVE).await()
      val observed = waitForObservedOfflineRegionEvent(runtime, created.id)
      assertEquals(RuntimeEventSourceType.RUNTIME, observed.sourceType)
      assertTrue(observed.source != 0uL)
      assertObservedOfflineRegionStatusPayload(created.id, observed.payload)
      // A drained value stays readable after the next drain ends the batch window.
      val copiedMessage = observed.message
      drainCopiedEvents(runtime)
      assertEquals(copiedMessage, observed.message)

      runtime.offlineRegionSetObserved(created.id, false).await()
      runtime.offlineRegionSetDownloadState(created.id, OfflineRegionDownloadState.INACTIVE).await()
      runtime.offlineRegionInvalidate(created.id).await()
      runtime.offlineRegionDelete(created.id).await()
      assertNull(runtime.offlineRegionGet(created.id).await())
    } finally {
      runtime.close().await()
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
          is RuntimeEventPayload.OfflineRegionStatus ->
            if (payload.value.regionId == regionId) return event
          is RuntimeEventPayload.OfflineRegionResponseError ->
            if (payload.value.regionId == regionId) return event
          is RuntimeEventPayload.OfflineRegionTileCountLimit ->
            if (payload.value.regionId == regionId) return event
          else -> Unit
        }
      }
      runtime.barrier().await()
    }
    error("offline region observation event did not arrive for region $regionId")
  }

  /**
   * Drains once and keeps every event this test has observed. One offline step drains the events of
   * the steps before it, so the waiters below scan what the whole test has seen rather than one
   * batch.
   */
  private suspend fun drain(runtime: RuntimeHandle) {
    runtime.barrier().await()
    drained += drainCopiedEvents(runtime)
  }

  private fun assertObservedOfflineRegionStatusPayload(
    regionId: Long,
    payload: RuntimeEventPayload,
  ) {
    val statusChanged = assertIs<RuntimeEventPayload.OfflineRegionStatus>(payload)
    assertEquals(regionId, statusChanged.value.regionId)
    assertEquals(OfflineRegionDownloadState.ACTIVE, statusChanged.value.status.downloadState)
  }

  private fun tileDefinition(): OfflineRegionDefinition =
    OfflineRegionDefinition(
      OfflineRegionDefinitionData.TilePyramid(
        OfflineTilePyramidRegionDefinition(
          "custom://offline-style.json",
          LatLngBounds(LatLng(0.0, 0.0), LatLng(1.0, 1.0)),
          0.0,
          1.0,
          1.0f,
          true,
        )
      )
    )

  private fun drainCopiedEvents(runtime: RuntimeHandle): List<RuntimeEvent> {
    val batch =
      try {
        runtime.drainEvents()
      } catch (error: org.maplibre.nativeffi.error.MaplibreException) {
        if (error.status == org.maplibre.nativeffi.error.MaplibreStatus.NOT_READY)
          return emptyList()
        throw error
      }
    return batch.use { it.get().events }
  }
}
