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
    var nativeCalls = 0
    val core = ResourceRequestHandleCore {}

    core.beginComplete().use {
      nativeCalls += 1
      it.markCompleted()
    }
    val error = assertFailsWith<InvalidStateException> { core.beginComplete() }

    assertEquals(MaplibreStatus.INVALID_STATE, error.status)
    assertEquals(1, nativeCalls)
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

  @Test
  fun providerOwnedHandleClosedBeforeDecisionReleasesAfterDecisionExactlyOnce() {
    var releases = 0
    val core = ResourceRequestHandleCore { releases++ }

    core.close()
    assertEquals(
      ResourceProviderDecision.HANDLE,
      core.finishProviderDecision(ResourceProviderDecision.HANDLE),
    )
    core.close()
    core.releaseIfOwned()

    assertEquals(1, releases)
  }

  @Test
  fun retainedPassThroughHandleCannotStartLaterOperations() {
    var releases = 0
    val core = ResourceRequestHandleCore { releases++ }

    assertEquals(
      ResourceProviderDecision.PASS_THROUGH,
      core.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH),
    )

    assertFailsWith<InvalidStateException> { core.beginComplete() }
    assertFailsWith<InvalidStateException> { core.withLiveHandle {} }
    core.close()

    assertEquals(0, releases)
  }

  @Test
  fun closeRejectsCompletionAndCancellationBeforeNativeCalls() {
    var releases = 0
    var nativeCalls = 0
    val core = ResourceRequestHandleCore { releases++ }

    core.finishProviderDecision(ResourceProviderDecision.HANDLE)
    core.close()

    assertFailsWith<InvalidStateException> { core.beginComplete().use { nativeCalls += 1 } }
    assertFailsWith<InvalidStateException> { core.withLiveHandle { nativeCalls += 1 } }

    assertEquals(0, nativeCalls)
    assertEquals(1, releases)
  }
}
