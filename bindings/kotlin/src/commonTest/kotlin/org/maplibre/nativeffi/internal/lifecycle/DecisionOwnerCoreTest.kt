package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.TestThread
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.runSuspendTest

@OptIn(ExperimentalAtomicApi::class)
class DecisionOwnerCoreTest {
  @Test
  fun aCloseOnAnotherThreadWaitsOutACompletionInProgress(): Unit = runSuspendTest {
    val releases = AtomicInt(0)
    val state =
      DecisionOwnerState(
        "TestOwner",
        SyntheticHandles.resourceRequest(),
        accept = 1u,
        passThrough = 0u,
      ) {
        releases.addAndFetch(1)
      }
    state.finishDecision(1u)
    val entered = CompletableDeferred<Unit>()
    val leave = CompletableDeferred<Unit>()
    val worker = TestThread {
      state.complete {
        entered.complete(Unit)
        runBlocking { leave.await() }
      }
    }
    try {
      entered.awaitWithin("the completion to start on the worker")
      // The completion borrows the owner, so the close marks it and leaves the release to the
      // completion's end.
      state.close()
      state.close()
      assertEquals(0, releases.load())
    } finally {
      leave.complete(Unit)
      worker.join()
    }
    assertEquals(1, releases.load())
    assertFailsWith<InvalidStateException> { state.withLive {} }
  }

  @Test
  fun inlineCloseForcesOwnershipThroughPassThroughOrException(): Unit = runSuspendTest {
    for (exception in listOf(false, true)) {
      var releases = 0
      val core = DecisionOwnerCore("TestOwner") { releases++ }
      core.close()
      core.close()
      assertEquals(0, releases)
      val decision =
        if (exception) core.finishException()
        else core.finishDecision(DecisionOwnerCore.Decision.PASS_THROUGH)
      assertEquals(DecisionOwnerCore.Decision.ACCEPT, decision)
      core.close()
      core.releaseIfOwned()
      assertEquals(1, releases)
    }
  }

  @Test
  fun completionBeforeProviderDecisionForcesProviderOwnership(): Unit = runSuspendTest {
    var releases = 0
    val core = DecisionOwnerCore("TestOwner") { releases++ }

    core.beginComplete().use { it.markCompleted() }
    assertEquals(
      DecisionOwnerCore.Decision.ACCEPT,
      core.finishDecision(DecisionOwnerCore.Decision.PASS_THROUGH),
    )

    assertEquals(0, releases)
    core.withLiveHandle {}
    core.close()
    assertEquals(1, releases)
  }

  @Test
  fun failedCompletionBeforeNativeCallLeavesHandleRetryable(): Unit = runSuspendTest {
    var completions = 0
    val core = DecisionOwnerCore("TestOwner") {}

    val first = core.beginComplete()
    first.markNotReachedNative()
    first.close()

    core.beginComplete().use {
      completions++
      it.markCompleted()
    }

    assertEquals(1, completions)
  }

  @Test
  fun completedHandleRejectsFurtherCompletion(): Unit = runSuspendTest {
    var nativeCalls = 0
    val core = DecisionOwnerCore("TestOwner") {}

    core.beginComplete().use {
      nativeCalls += 1
      it.markCompleted()
    }
    val error = assertFailsWith<InvalidStateException> { core.beginComplete() }

    assertEquals(MaplibreStatus.INVALID_STATE, error.status)
    assertEquals(1, nativeCalls)
  }

  @Test
  fun retainedPassThroughHandleCannotStartLaterOperations(): Unit = runSuspendTest {
    var releases = 0
    val core = DecisionOwnerCore("TestOwner") { releases++ }

    assertEquals(
      DecisionOwnerCore.Decision.PASS_THROUGH,
      core.finishDecision(DecisionOwnerCore.Decision.PASS_THROUGH),
    )

    assertFailsWith<InvalidStateException> { core.beginComplete() }
    assertFailsWith<InvalidStateException> { core.withLiveHandle {} }
    core.close()

    assertEquals(0, releases)
  }

  @Test
  fun closeRejectsCompletionAndCancellationBeforeNativeCalls(): Unit = runSuspendTest {
    var releases = 0
    var nativeCalls = 0
    val core = DecisionOwnerCore("TestOwner") { releases++ }

    core.finishDecision(DecisionOwnerCore.Decision.ACCEPT)
    core.close()

    assertFailsWith<InvalidStateException> { core.beginComplete().use { nativeCalls += 1 } }
    assertFailsWith<InvalidStateException> { core.withLiveHandle { nativeCalls += 1 } }

    assertEquals(0, nativeCalls)
    assertEquals(1, releases)
  }
}
