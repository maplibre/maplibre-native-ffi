package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

class ResourceRequestHandleCoreTest {
  @Test
  fun providerOwnedHandleReleasesAfterCloseExactlyOnce() {
    var releases = 0
    val core = ResourceRequestHandleCore { releases++ }

    assertEquals(
      ResourceProviderDecision.HANDLE,
      core.finishProviderDecision(ResourceProviderDecision.HANDLE),
    )
    core.close()
    core.close()
    core.releaseIfOwned()

    assertEquals(1, releases)
  }

  @Test
  fun passThroughDecisionLetsNativeOwnRelease() {
    var releases = 0
    val core = ResourceRequestHandleCore { releases++ }

    assertEquals(
      ResourceProviderDecision.PASS_THROUGH,
      core.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH),
    )
    core.close()
    core.releaseIfOwned()

    assertEquals(0, releases)
  }

  @Test
  fun completionBeforeProviderDecisionForcesProviderOwnership() {
    var releases = 0
    val core = ResourceRequestHandleCore { releases++ }

    core.beginComplete().use { it.markCompleted() }
    assertEquals(
      ResourceProviderDecision.HANDLE,
      core.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH),
    )

    assertEquals(1, releases)
  }

  @Test
  fun failedCompletionBeforeNativeCallLeavesHandleRetryable() {
    var completions = 0
    val core = ResourceRequestHandleCore {}

    val first = core.beginComplete()
    first.markNotReachedNative()
    first.close()

    core.beginComplete().use {
      completions++
      it.markCompleted()
    }

    assertEquals(1, completions)
  }

  /**
   * A release that failed leaves the request this wrapper's to release, rather than nobody's.
   *
   * Releasing is the one call a request handle makes that has no answer to report and no second
   * chance: the wrapper accounts for it before calling native, so that two stacks closing together
   * cannot both call. The failure this drives is what that accounting must not survive -- a page
   * whose allocator refuses the block the call's arguments are packed into never reaches C at all,
   * and a reference that recorded the release anyway would leave MapLibre waiting on a request
   * nothing can now answer or retire.
   *
   * So the claim is about the state the failure left, not the error: the close reports it, and the
   * close after it really does reach native.
   */
  @Test
  fun aReleaseThatFailedLeavesTheRequestReleasableAgain() {
    var attempts = 0
    val core = ResourceRequestHandleCore {
      attempts++
      // Only the first, so that what the second close does is observable rather than another
      // failure.
      if (attempts == 1) error("the release never reached native")
    }

    assertEquals(
      ResourceProviderDecision.HANDLE,
      core.finishProviderDecision(ResourceProviderDecision.HANDLE),
    )
    assertFailsWith<IllegalStateException> { core.close() }
    core.close()

    assertEquals(2, attempts, "a failed release left the request accounted for but never given up")
  }

  @Test
  fun completedHandleRejectsFurtherCompletion() {
    val core = ResourceRequestHandleCore {}

    core.beginComplete().use { it.markCompleted() }
    val error = assertFailsWith<InvalidStateException> { core.beginComplete() }

    assertEquals(MaplibreStatus.INVALID_STATE, error.status)
  }

  @Test
  fun closeDuringLiveOperationDefersProviderOwnedReleaseUntilOperationExits() {
    var releases = 0
    val core = ResourceRequestHandleCore { releases++ }

    assertEquals(
      ResourceProviderDecision.HANDLE,
      core.finishProviderDecision(ResourceProviderDecision.HANDLE),
    )
    val operation = core.beginComplete()
    core.close()

    assertEquals(0, releases)

    operation.markCompleted()
    operation.close()

    assertEquals(1, releases)
  }
}
