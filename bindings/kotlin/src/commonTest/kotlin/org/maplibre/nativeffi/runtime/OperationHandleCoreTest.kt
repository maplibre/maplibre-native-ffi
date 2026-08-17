package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException

class OperationHandleCoreTest {
  @Test
  fun takeValidatesOperationKindInsideUseLease(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = Any()
      val core = operation(runtime)

      assertFailsWith<InvalidStateException> {
        core.withUse(runtime, OperationKind.REGION_GET, OperationResultKind.OPTIONAL_REGION) {}
      }

      assertFalse(core.isClosed)
      assertEquals(
        7L,
        core.withUse(runtime, OperationKind.REGION_CREATE, OperationResultKind.REGION) { it },
      )
    }

  @Test
  fun failedTakeLeavesResultRetryable(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = Any()
      val core = operation(runtime)

      repeat(2) {
        assertFailsWith<IllegalStateException> {
          core.withUse(runtime) { error("native take failed") }
        }
        assertFalse(core.isClosed)
      }
    }

  @Test
  fun consumedResultKeepsObserverLive(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = Any()
      val core = operation(runtime)

      core.markResultConsumed()

      assertFalse(core.isClosed)
      assertEquals(7L, core.withUse(runtime) { it })
      assertFailsWith<InvalidStateException> {
        core.withUse(runtime, OperationKind.REGION_CREATE, OperationResultKind.REGION) {}
      }
    }

  @Test
  fun closeIsIdempotent(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = Any()
      val core = operation(runtime)

      assertTrue(core.beginClose())
      core.finishClose()
      assertFalse(core.beginClose())

      assertTrue(core.isClosed)
      assertFailsWith<InvalidStateException> { core.withUse(runtime) {} }
    }

  private fun operation(runtime: Any): OperationHandleCore =
    OperationHandleCore(runtime, 7L, OperationKind.REGION_CREATE, OperationResultKind.REGION)
}
