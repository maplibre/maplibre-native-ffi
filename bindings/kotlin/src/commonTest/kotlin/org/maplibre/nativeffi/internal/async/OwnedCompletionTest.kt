package org.maplibre.nativeffi.internal.async

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.internal.c.CompletionResultLayout
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readU64
import org.maplibre.nativeffi.runSuspendTest

private fun <T> ownedCompletion(
  closeDropped: (T) -> Unit,
  disposeUnadopted: (ULong) -> Unit,
  convert: (ULong) -> T,
): HandDeliveredCompletion<T> = capture { call ->
  NativeArena().use { arena ->
    CompletionBridge.submitOwned(
      arena,
      convert = { convert(readU64(it + CompletionResultLayout.GENERATION)) },
      closeDropped = closeDropped,
      disposeUnadopted = { disposeUnadopted(readU64(it + CompletionResultLayout.GENERATION)) },
      call = call,
    )
  }
}

class OwnedCompletionTest {
  @Test
  fun conversionFailureReleasesTheRawOwnerAndCompletesExceptionally() = runSuspendTest {
    val failure = IllegalStateException("conversion")
    var disposed = 0uL
    val completion =
      ownedCompletion<String>(
        closeDropped = { error("No wrapper was constructed") },
        disposeUnadopted = { disposed += it },
        convert = { throw failure },
      )
    completion.deliver(Status.OK.rawValue, 7u)
    completion.release()
    assertEquals(
      failure.message,
      assertFailsWith<IllegalStateException> { completion.deferred.await() }.message,
    )
    assertEquals(7uL, disposed)
  }

  @Test
  fun cancellationReleasesTheAdoptedWrapperExactlyOnce() = runSuspendTest {
    var disposed = 0
    var closed = 0
    val completion =
      ownedCompletion(
        closeDropped = { value: String -> closed += value.length },
        disposeUnadopted = { disposed++ },
        convert = { it.toString() },
      )
    completion.deferred.cancel()
    completion.deliver(Status.OK.rawValue, 17u)
    completion.release()
    assertEquals(2, closed)
    assertEquals(0, disposed)
  }
}
