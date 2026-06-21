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
