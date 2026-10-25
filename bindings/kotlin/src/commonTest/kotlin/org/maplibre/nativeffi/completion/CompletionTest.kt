package org.maplibre.nativeffi.completion

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlin.time.Duration.Companion.milliseconds
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.withTimeoutOrNull
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapDebugFlag
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.RuntimeEventHandler
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.CompletionLayout
import org.maplibre.nativeffi.internal.call.nativeUnit
import org.maplibre.nativeffi.internal.callback.NativeRoots
import org.maplibre.nativeffi.internal.memory.readAddress
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** How a native completion reaches Kotlin as a Deferred. */
class CompletionTest {
  @Test
  fun aDiscardedCommandIsFencedAndSynchronousRejectionThrows() = runSuspendTest {
    withMap {
      map.discarding.setDebugOptions(MapDebugFlag.TILE_BORDERS)
      // A later command on the same map completes after the discarded one.
      map.requestRepaint().await()
      assertEquals(MapDebugFlag.TILE_BORDERS, map.getSnapshot().debugOptions)
      assertFailsWithStatus(Status.INVALID_ARGUMENT) {
        map.discarding.resize(LogicalExtent(0u, 8u, 1.0))
      }
    }
  }

  @Test
  fun aSubmissionNativeRejectsLeavesNoCompletionBehind() = runSuspendTest {
    withMap {
      var token = 0L
      // Native rejects the unknown state before it takes the completion, so the call itself throws
      // the rejection and the bridge frees its state without returning a Deferred.
      val failure =
        assertFailsWithStatus(Status.INVALID_ARGUMENT) {
          nativeUnit(runtime, runtime.binding) {
            token = readAddress(completion + CompletionLayout.USER_DATA)
            check(
              C.mln_runtime_set_offline_region_download_state(
                handle,
                1,
                900,
                completion,
                diagnostic,
              ),
            )
          }
        }
      assertTrue(failure.diagnostic.isNotEmpty(), "the rejection carries its diagnostic")
      assertTrue(token != 0L, "the rejected call received a completion token")
      // Setup completions may still be releasing their state, so inspect only this call's root.
      assertNull(NativeRoots.get(token))
    }
  }

  @Test
  fun aCommandCompletesAfterItsEventsReachTheHandler() = runSuspendTest {
    withMap {
      val events = Channel<RuntimeEventType>(Channel.UNLIMITED)
      runtime
        .setEventHandler(
          RuntimeEventHandler { batch -> batch.forEach { events.trySend(it.type) } },
        )
        .await()
      val seenAtCompletion = CompletableDeferred<List<RuntimeEventType>>()
      val style = map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray())
      // The handler runs inside native's completion, after the callbacks for the command's events.
      style.invokeOnCompletion {
        seenAtCompletion.complete(generateSequence { events.tryReceive().getOrNull() }.toList())
      }
      style.await()
      assertTrue(RuntimeEventType.MAP_STYLE_LOADED in seenAtCompletion.await())
    }
  }

  @Test
  fun aFailedCommandFailsItsDeferredWithItsStatusAndGeneration() = runSuspendTest {
    withMap {
      loadStyle()
      val failure =
        assertFailsWithStatus(Status.NOT_FOUND) { map.removeStyleSource("missing").await() }
      assertTrue(failure.diagnostic.isNotEmpty(), "a failed command carries its diagnostic")
      // The command ran and published the generation the map now reports.
      assertNotEquals(0uL, failure.generation)
      assertEquals(map.getSnapshot().generation, failure.generation)
    }
  }

  @Test
  fun waitingOnACompletionTimesOutOrIsCancelledWithoutEndingIt() = runSuspendTest {
    withMap(mapMode = MapMode.STATIC) {
      loadStyle()
      // A static map renders its still image only for a render session, and none is attached.
      val still = map.requestStillImage()

      assertNull(withTimeoutOrNull(1.milliseconds) { still.await() })
      coroutineScope {
        val waiter = async(start = CoroutineStart.UNDISPATCHED) { still.await() }
        waiter.cancelAndJoin()
        assertTrue(waiter.isCancelled)
      }
      // Neither the timeout nor the cancelled wait touched the operation, which the map's release
      // ends instead.
      assertTrue(still.isActive)

      map.release().await()
      val failure = assertFailsWith<MaplibreException> { still.await() }
      assertEquals(Status.CANCELLED, failure.status)
    }
  }
}
