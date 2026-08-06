package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.pumpUntil
import org.maplibre.nativeffi.withRuntime

/**
 * The offline database, driven through the runtime's operation handles.
 *
 * Every operation completes through the event queue, so an operation is two calls with a pump
 * between them. The database is in memory, which is what a browser has: the module's default file
 * system does not survive the page.
 */
class RuntimeOfflineBrowserTest {
  // Spec coverage: BND-041, BND-060, BND-061, BND-069, BND-084, BND-085.

  private val deferred = ArrayDeque<RuntimeEvent>()

  @Test
  fun anOfflineRegionIsCreatedObservedAndDeletedThroughCopiedResults() {
    withRuntime(RuntimeOptions().apply { cachePath = ":memory:" }) { runtime ->
      val definition = tileDefinition()

      // The metadata is caller-owned storage, so the descriptor has to snapshot it: the array
      // is mutated the instant the call returns.
      val createMetadata = byteArrayOf(1, 2, 3)
      val createOperation = runtime.startCreateOfflineRegion(definition, createMetadata)
      createMetadata[0] = 9

      waitForOperation(runtime, createOperation)
      val created = runtime.takeCreateOfflineRegionResult(createOperation)

      assertTrue(created.id > 0)
      assertEquals(definition, created.definition)
      assertContentEquals(byteArrayOf(1, 2, 3), created.metadata)
      // And the accessor hands back a copy rather than the stored array.
      created.metadata[0] = 9
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

      // Observation is what turns database progress into runtime events.
      completeVoid(runtime, runtime.startSetOfflineRegionObserved(created.id, true))
      completeVoid(
        runtime,
        runtime.startSetOfflineRegionDownloadState(created.id, OfflineRegionDownloadState.ACTIVE),
      )
      val observed = waitForObservation(runtime, created.id)
      val copiedMessage = observed.message
      assertEquals(RuntimeEventSourceType.RUNTIME, observed.sourceType)
      assertEquals(runtime, observed.runtimeSource)
      assertNull(observed.mapSource)
      assertObservationPayload(created.id, observed.payload)
      runtime.pollEvent()
      assertEquals(copiedMessage, observed.message)

      completeVoid(runtime, runtime.startSetOfflineRegionObserved(created.id, false))
      completeVoid(
        runtime,
        runtime.startSetOfflineRegionDownloadState(created.id, OfflineRegionDownloadState.INACTIVE),
      )
      completeVoid(runtime, runtime.startInvalidateOfflineRegion(created.id))
      completeVoid(runtime, runtime.startDeleteOfflineRegion(created.id))
      assertNull(offlineRegion(runtime, created.id))
    }
  }

  @Test
  fun aTakeThatFailsBeforeOwnershipMovesLeavesTheOperationRetryable() {
    withRuntime(RuntimeOptions().apply { cachePath = ":memory:" }) { runtime ->
      // A region id no database row has. The operation completes, and the take reports that
      // nothing was found without consuming the handle.
      val operation = runtime.startOfflineRegionStatus(MISSING_REGION_ID)
      waitForOperationStatus(runtime, operation)

      val first =
        assertFailsWith<MaplibreException> { runtime.takeOfflineRegionStatusResult(operation) }
      assertFalse(operation.isClosed)

      // Retryable, and reporting the same thing each time.
      val second =
        assertFailsWith<MaplibreException> { runtime.takeOfflineRegionStatusResult(operation) }
      assertEquals(first.status, second.status)
      assertFalse(operation.isClosed)

      operation.close()
      assertTrue(operation.isClosed)
    }
  }

  @Test
  fun aTakeOfTheWrongResultKindIsRefusedBeforeCrossingIntoTheModule() {
    withRuntime(RuntimeOptions().apply { cachePath = ":memory:" }) { runtime ->
      // An ambient cache operation carries no result, so asking it for a region is a
      // binding-owned invariant rather than something native has to be asked about. The kinds
      // are checked against the wrapper's own record, which is why this is built rather than
      // started: the public API cannot produce a handle whose kinds disagree with its call.
      val operation =
        OfflineOperationHandle<OfflineRegionInfo>(
          runtime,
          1L,
          OfflineOperationKind.AMBIENT_CACHE,
          OfflineOperationResultKind.NONE,
        )
      try {
        assertFailsWith<InvalidStateException> { runtime.takeCreateOfflineRegionResult(operation) }
        assertFalse(operation.isClosed)
      } finally {
        operation.markConsumed()
      }

      // Binding-owned validation on inputs, too.
      assertFailsWith<InvalidArgumentException> { runtime.startSetMaximumAmbientCacheSize(-1L) }
      assertFailsWith<InvalidArgumentException> {
        runtime.startSetOfflineRegionDownloadState(1, OfflineRegionDownloadState(900))
      }
    }
  }

  @Test
  fun aReleaseNativeRefusesLeavesTheHandleLiveForALaterOne() {
    withRuntime(RuntimeOptions().apply { cachePath = ":memory:" }) { runtime ->
      // An operation id this runtime never issued. Discarding it is a native call that fails, and
      // a failed release must leave the wrapper live rather than consuming it: the wrapper still
      // holds its runtime open, and something has to be able to try again.
      val stale =
        OfflineOperationHandle<Unit>(
          runtime,
          UNISSUED_OPERATION_ID,
          OfflineOperationKind.AMBIENT_CACHE,
          OfflineOperationResultKind.NONE,
        )
      assertFailsWith<InvalidArgumentException> { stale.close() }
      assertFalse(stale.isClosed)
      // Still holding the runtime open, which is the state a consumed wrapper would have left.
      assertFailsWith<InvalidStateException> { runtime.close() }
      stale.markConsumed()

      // A release native accepts destroys the handle, and a second one is a no-op.
      val real = runtime.startAmbientCacheOperation(AmbientCacheOperation.INVALIDATE)
      real.close()
      assertTrue(real.isClosed)
      real.close()
      assertTrue(real.isClosed)
    }
  }

  private fun waitForOperation(
    runtime: RuntimeHandle,
    operation: OfflineOperationHandle<*>,
  ): RuntimeEventPayload.OfflineOperationCompleted {
    val completed = waitForOperationStatus(runtime, operation)
    if (completed.resultStatus != MaplibreStatus.OK.nativeCode) {
      throw MaplibreException.forStatus(
        MaplibreStatus.fromNative(completed.resultStatus),
        completed.resultStatus,
        "offline operation ${operation.id} failed",
      )
    }
    return completed
  }

  /** Waits for the completion event, whatever status it carries. */
  private fun waitForOperationStatus(
    runtime: RuntimeHandle,
    operation: OfflineOperationHandle<*>,
  ): RuntimeEventPayload.OfflineOperationCompleted {
    var completed: RuntimeEventPayload.OfflineOperationCompleted? = null
    pumpUntil(
      runtime,
      onEvent = { event ->
        val payload = event.payload as? RuntimeEventPayload.OfflineOperationCompleted
        if (payload != null && payload.operationId == operation.id) {
          assertEquals(operation.kind, payload.operationKind)
          assertEquals(operation.resultKind, payload.resultKind)
          completed = payload
        } else {
          deferred.addLast(event)
        }
      },
    ) {
      completed != null
    }
    return completed ?: error("offline operation ${operation.id} did not complete")
  }

  private fun completeVoid(runtime: RuntimeHandle, operation: OfflineOperationHandle<Unit>) {
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

  private fun waitForObservation(runtime: RuntimeHandle, regionId: Long): RuntimeEvent {
    var observed: RuntimeEvent? = null
    // Events deferred by an earlier wait may already hold the observation this one wants.
    while (observed == null && deferred.isNotEmpty()) {
      val event = deferred.removeFirst()
      if (namesRegion(event.payload, regionId)) observed = event
    }
    if (observed == null) {
      pumpUntil(runtime, onEvent = { if (namesRegion(it.payload, regionId)) observed = it }) {
        observed != null
      }
    }
    return observed ?: error("no observation event arrived for offline region $regionId")
  }

  private fun namesRegion(payload: RuntimeEventPayload, regionId: Long): Boolean =
    when (payload) {
      is RuntimeEventPayload.OfflineRegionStatusChanged -> payload.regionId == regionId
      is RuntimeEventPayload.OfflineRegionResponseError -> payload.regionId == regionId
      is RuntimeEventPayload.OfflineRegionTileCountLimit -> payload.regionId == regionId
      else -> false
    }

  private fun assertObservationPayload(regionId: Long, payload: RuntimeEventPayload) {
    val changed = assertIs<RuntimeEventPayload.OfflineRegionStatusChanged>(payload)
    assertEquals(regionId, changed.regionId)
    assertTrue(changed.status.completedResourceCount >= 0)
    assertTrue(changed.status.completedTileCount >= 0)
    assertTrue(changed.status.requiredTileCount >= 0)
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

  private companion object {
    /** No row is created with this id, so every operation naming it reports not found. */
    const val MISSING_REGION_ID = 987_654L

    /** An operation id no runtime here issued, so discarding it is a native call that fails. */
    const val UNISSUED_OPERATION_ID = 424_242L
  }
}
