package org.maplibre.nativeffi.internal.lifecycle

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.runtime.runSuspendTest

class DecisionOwnerCoreTest {
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
  fun providerOwnedHandleReleasesAfterCloseExactlyOnce(): Unit = runSuspendTest {
    var releases = 0
    val core = DecisionOwnerCore("TestOwner") { releases++ }

    assertEquals(
      DecisionOwnerCore.Decision.ACCEPT,
      core.finishDecision(DecisionOwnerCore.Decision.ACCEPT),
    )
    core.close()
    core.close()
    core.releaseIfOwned()

    assertEquals(1, releases)
  }

  @Test
  fun passThroughDecisionLetsNativeOwnRelease(): Unit = runSuspendTest {
    var releases = 0
    val core = DecisionOwnerCore("TestOwner") { releases++ }

    assertEquals(
      DecisionOwnerCore.Decision.PASS_THROUGH,
      core.finishDecision(DecisionOwnerCore.Decision.PASS_THROUGH),
    )
    core.close()
    core.releaseIfOwned()

    assertEquals(0, releases)
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
  fun closeDuringLiveOperationDefersProviderOwnedReleaseUntilOperationExits(): Unit =
    runSuspendTest {
      var releases = 0
      val core = DecisionOwnerCore("TestOwner") { releases++ }

      assertEquals(
        DecisionOwnerCore.Decision.ACCEPT,
        core.finishDecision(DecisionOwnerCore.Decision.ACCEPT),
      )
      val operation = core.beginComplete()
      core.close()

      assertEquals(0, releases)

      operation.markCompleted()
      operation.close()

      assertEquals(1, releases)
    }

  @Test
  fun providerOwnedHandleClosedBeforeDecisionReleasesAfterDecisionExactlyOnce(): Unit =
    runSuspendTest {
      var releases = 0
      val core = DecisionOwnerCore("TestOwner") { releases++ }

      core.close()
      assertEquals(
        DecisionOwnerCore.Decision.ACCEPT,
        core.finishDecision(DecisionOwnerCore.Decision.ACCEPT),
      )
      core.close()
      core.releaseIfOwned()

      assertEquals(1, releases)
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
