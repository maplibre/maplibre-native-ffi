package org.maplibre.nativeffi.internal.async

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.CommandCompletion

/**
 * A completion submitted through the platform's completion bridge, which the test delivers and
 * releases by hand through the descriptor the bridge built, as native would.
 */
internal expect class HandDeliveredCompletion<T> {
  val deferred: Deferred<T>

  fun deliver(status: Int, generation: ULong, diagnostic: String = "", disposition: UInt = 0u)

  fun release()
}

/** A completion whose value is its result's generation. */
internal expect fun handDeliveredGeneration(): HandDeliveredCompletion<ULong>

/** An owned completion that hands a value nobody adopted to [closeDropped]. */
internal expect fun handDeliveredOwned(
  closeDropped: (ULong) -> Unit
): HandDeliveredCompletion<ULong>

/** An ordered command completion. */
internal expect fun handDeliveredCommand(): HandDeliveredCompletion<CommandCompletion>

/** Each emitter's completion bridge, driven without native. */
class CompletionBridgeTest {
  @Test
  fun aCompletionKeepsItsFirstResultAndIgnoresDeliveryAfterRelease(): Unit = runSuspendTest {
    val completion = handDeliveredGeneration()
    completion.deliver(MaplibreStatus.OK.nativeCode, 7u)
    completion.deliver(MaplibreStatus.OK.nativeCode, 8u)
    assertEquals(7u, completion.deferred.await())
    completion.release()
    // The released state is gone, so a stray delivery finds nothing to complete.
    completion.deliver(MaplibreStatus.OK.nativeCode, 9u)
    assertEquals(7u, completion.deferred.await())
  }

  @Test
  fun aFailedStatusBecomesItsExceptionWithTheDiagnostic(): Unit = runSuspendTest {
    val completion = handDeliveredGeneration()
    completion.deliver(MaplibreStatus.INVALID_ARGUMENT.nativeCode, 0u, "the zoom is out of range")
    completion.release()
    val failure = assertFailsWith<InvalidArgumentException> { completion.deferred.await() }
    assertEquals(MaplibreStatus.INVALID_ARGUMENT, failure.status)
    assertEquals("the zoom is out of range", failure.diagnostic)
  }

  @Test
  fun aCommandKeepsAFailedStatusAsData(): Unit = runSuspendTest {
    val completion = handDeliveredCommand()
    completion.deliver(
      MaplibreStatus.NOT_FOUND.nativeCode,
      3u,
      "no such layer",
      CommandDisposition.FAILED.rawValue,
    )
    completion.release()
    val command = completion.deferred.await()
    assertEquals(MaplibreStatus.NOT_FOUND, command.status)
    assertEquals(3u, command.generation)
    assertEquals("no such layer", command.diagnostic)
    assertEquals(CommandDisposition.FAILED, command.disposition)
  }

  @Test
  fun aValueArrivingAfterCancellationIsHandedToCloseDropped(): Unit = runSuspendTest {
    val dropped = mutableListOf<ULong>()
    val completion = handDeliveredOwned(dropped::add)
    completion.deferred.cancel()
    completion.deliver(MaplibreStatus.OK.nativeCode, 11u)
    completion.release()
    assertEquals(listOf(11uL), dropped)
    assertFalse(completion.deferred.isActive)
  }
}
