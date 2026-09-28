package org.maplibre.nativeffi.internal.async

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.runtime.runSuspendTest

class OwnedCompletionTest {
  @Test
  fun conversionFailureReleasesTheRawOwnerAndCompletesExceptionally(): Unit = runSuspendTest {
    val raw = CompletableDeferred<Int>()
    val failure = IllegalStateException("conversion")
    var disposed = 0
    val wrapped =
      raw.mapHandleDeferred<Int, String>(
        closeDropped = { error("No wrapper was constructed") },
        disposeUnadopted = { disposed += it },
        transform = { throw failure },
      )
    raw.complete(7)
    assertEquals(
      failure.message,
      assertFailsWith<IllegalStateException> { wrapped.await() }.message,
    )
    assertEquals(7, disposed)
  }

  @Test
  fun cancellationReleasesTheAdoptedWrapperExactlyOnce(): Unit = runSuspendTest {
    val raw = CompletableDeferred<Int>()
    var disposed = 0
    var closed = 0
    val wrapped =
      raw.mapHandleDeferred(
        closeDropped = { value: String -> closed += value.length },
        disposeUnadopted = { disposed++ },
        transform = { it.toString() },
      )
    wrapped.cancel()
    raw.complete(17)
    assertEquals(2, closed)
    assertEquals(0, disposed)
  }

  @Test
  fun successfulAdoptionTransfersTheRawOwner(): Unit = runSuspendTest {
    var disposed = 0
    val wrapped =
      CompletableDeferred(17)
        .mapHandleDeferred(
          closeDropped = { _: String -> error("The result is observed") },
          disposeUnadopted = { disposed++ },
          transform = { it.toString() },
        )
    assertEquals("17", wrapped.await())
    assertEquals(0, disposed)
  }
}
