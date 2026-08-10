package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException

class OfflineOperationHandleCoreTest {
  @Test
  fun takeValidatesOperationKindBeforeReturningTheNativeId() {
    val runtime = Any()
    val core = operation(runtime)

    assertFailsWith<InvalidStateException> {
      core.requireLive(
        runtime,
        OfflineOperationKind.REGION_GET,
        OfflineOperationResultKind.OPTIONAL_REGION,
      )
    }

    assertFalse(core.isClosed)
    assertEquals(
      7L,
      core.requireLive(
        runtime,
        OfflineOperationKind.REGION_CREATE,
        OfflineOperationResultKind.REGION,
      ),
    )
  }

  @Test
  fun failedTakeLeavesOperationRetryable() {
    val runtime = Any()
    val core = operation(runtime)

    repeat(2) {
      assertEquals(7L, core.requireLive(runtime))
      assertFailsWith<IllegalStateException> { error("native take failed") }
      assertFalse(core.isClosed)
    }
  }

  @Test
  fun consumedOperationCloseIsIdempotent() {
    var retentionReleases = 0
    val runtime = Any()
    val core = operation(runtime) { retentionReleases += 1 }

    core.markConsumed()
    core.markConsumed()

    assertTrue(core.isClosed)
    assertEquals(1, retentionReleases)
    assertFailsWith<InvalidStateException> { core.requireLive(runtime) }
  }

  @Test
  fun liveOperationRetainsRuntimeUntilItIsConsumed() {
    var retentionReleases = 0
    val core = operation(Any()) { retentionReleases += 1 }

    assertEquals(0, retentionReleases)
    core.markConsumed()
    assertEquals(1, retentionReleases)
  }

  private fun operation(
    runtime: Any,
    releaseRuntimeRetention: () -> Unit = {},
  ): OfflineOperationHandleCore =
    OfflineOperationHandleCore(
      runtime,
      7L,
      OfflineOperationKind.REGION_CREATE,
      OfflineOperationResultKind.REGION,
      releaseRuntimeRetention,
    )
}
