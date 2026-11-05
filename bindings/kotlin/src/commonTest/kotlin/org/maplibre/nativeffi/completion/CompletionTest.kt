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
import org.maplibre.nativeffi.LogicalSize
import org.maplibre.nativeffi.MapDebugFlag
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.RuntimeEventHandler
import org.maplibre.nativeffi.RuntimeEventType
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** How a native completion reaches Kotlin as a Completion. */
class CompletionTest {
  @Test
  fun aDiscardedCommandIsFencedAndSynchronousRejectionThrows() = runSuspendTest {
    withMap {
      map.discarding.setDebugFlags(MapDebugFlag.TILE_BORDERS)
      // A later command on the same map completes after the discarded one.
      map.requestRepaint().await()
      assertEquals(MapDebugFlag.TILE_BORDERS, map.getSnapshot().debugFlags)
      assertFailsWithStatus(Status.INVALID_ARGUMENT) {
        map.discarding.resize(LogicalSize(0u, 8u))
      }
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
      val style = map.setStyleJson(EMPTY_STYLE_JSON)
      // The handler runs inside native's completion, after the callbacks for the command's events.
      style.asDeferred().invokeOnCompletion {
        seenAtCompletion.complete(generateSequence { events.tryReceive().getOrNull() }.toList())
      }
      style.await()
      assertTrue(RuntimeEventType.MAP_STYLE_LOADED in seenAtCompletion.await())
    }
  }

  @Test
  fun aFailedCommandFailsItsCompletionWithItsStatusAndGeneration() = runSuspendTest {
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
      still.asDeferred().cancel()
      // Neither the timeout, the cancelled wait, nor the cancelled Deferred touched the operation,
      // which the map's release ends instead.
      assertTrue(still.asDeferred().isActive)

      map.release().await()
      val failure = assertFailsWith<MaplibreException> { still.await() }
      assertEquals(Status.CANCELLED, failure.status)
    }
  }
}
