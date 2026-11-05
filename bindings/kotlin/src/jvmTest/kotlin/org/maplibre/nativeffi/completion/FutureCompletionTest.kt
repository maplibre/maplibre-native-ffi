package org.maplibre.nativeffi.completion

import java.util.concurrent.Executor
import java.util.concurrent.TimeUnit
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.Completion
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.RuntimeEventHandler
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** A Completion as the CompletableFuture that Java receives. */
class FutureCompletionTest {
  @Test
  fun onlyTheLibraryCompletesAResultAndDerivedStagesStayCompletions() {
    val result = Completion.pending<Int>()
    val now = Executor { it.run() }
    val attempts: List<() -> Any> =
      listOf(
        { result.complete(1) },
        { result.completeExceptionally(IllegalStateException()) },
        { result.cancel(false) },
        { result.obtrudeValue(1) },
        { result.obtrudeException(IllegalStateException()) },
        { result.completeAsync({ 1 }) },
        { result.completeAsync({ 1 }, now) },
        { result.completeOnTimeout(1, 1, TimeUnit.NANOSECONDS) },
        { result.orTimeout(1, TimeUnit.NANOSECONDS) },
      )
    for (attempt in attempts) assertFailsWith<UnsupportedOperationException> { attempt() }
    assertFalse(result.isDone)

    val derived = result.thenApply { it + 1 }
    assertIs<Completion<*>>(derived)
    // A derived stage belongs to its caller, who may complete it.
    val cancelled = result.thenApply { it }
    assertTrue(cancelled.cancel(false))
    result.succeed(1)
    assertEquals(2, derived.join())
    assertTrue(cancelled.isCancelled)
  }

  @Test
  fun aWaitOnANativeCallbackThreadThrowsUntilTheResultCompletes() = runSuspendTest {
    withMap {
      val pending = Completion.pending<Unit>()
      val completed = Completion.completed(Unit)
      // What each wait on the pending result threw, then what the wait on the completed one threw.
      val outcomes = CompletableDeferred<Pair<List<Throwable?>, Throwable?>>()
      runtime
        .setEventHandler(
          RuntimeEventHandler {
            if (outcomes.isCompleted) return@RuntimeEventHandler
            val waits: List<() -> Unit> =
              listOf(
                { pending.join() },
                { pending.get() },
                { pending.get(1, TimeUnit.DAYS) },
                { pending.thenApply { it }.join() },
              )
            outcomes.complete(
              waits.map { runCatching(it).exceptionOrNull() } to
                runCatching { completed.join() }.exceptionOrNull(),
            )
          },
        )
        .awaitWithin("the event handler")
      map.setStyleJson(EMPTY_STYLE_JSON).awaitWithin("the style command")
      val (pendingWaits, completedWait) = outcomes.awaitWithin("an event")
      for (outcome in pendingWaits) assertIs<IllegalStateException>(outcome)
      assertNull(completedWait)
    }
  }
}
