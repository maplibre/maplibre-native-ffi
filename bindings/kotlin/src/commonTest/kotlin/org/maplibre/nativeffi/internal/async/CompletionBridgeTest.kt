package org.maplibre.nativeffi.internal.async

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.generated.readLatLng
import org.maplibre.nativeffi.internal.c.CompletionLayout
import org.maplibre.nativeffi.internal.c.CompletionResultLayout
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readAddress
import org.maplibre.nativeffi.internal.memory.readStrided
import org.maplibre.nativeffi.internal.memory.readU64
import org.maplibre.nativeffi.internal.memory.writeAddress
import org.maplibre.nativeffi.internal.memory.writeF64
import org.maplibre.nativeffi.internal.memory.writeI32
import org.maplibre.nativeffi.internal.memory.writeSize
import org.maplibre.nativeffi.internal.memory.writeU32
import org.maplibre.nativeffi.internal.memory.writeU64
import org.maplibre.nativeffi.runSuspendTest

/**
 * A completion submitted through the completion bridge, which the test delivers and releases by
 * hand through the function pointers of the descriptor the bridge built, as native would.
 */
internal class HandDeliveredCompletion<T>(
  val deferred: Deferred<T>,
  private val callback: Long,
  private val userData: Long,
  private val releaseUserData: Long,
) {
  fun deliver(status: Int, generation: ULong, diagnostic: String = "", disposition: UInt = 0u) {
    NativeArena().use { arena ->
      val result = arena.allocate(CompletionResultLayout.SIZEOF)
      writeU32(result + CompletionResultLayout.SIZE, CompletionResultLayout.SIZEOF.toUInt())
      writeI32(result + CompletionResultLayout.STATUS, status)
      writeU32(result + CompletionResultLayout.DISPOSITION, disposition)
      writeU64(result + CompletionResultLayout.GENERATION, generation)
      if (diagnostic.isNotEmpty())
        arena.putView(result + CompletionResultLayout.DIAGNOSTIC, diagnostic)
      callCompletion(callback, userData, result)
    }
  }

  fun release() {
    callCompletionRelease(releaseUserData, userData)
  }
}

/** Calls the `mln_completion_callback` at [callback] on this thread. */
internal expect fun callCompletion(callback: Long, userData: Long, result: Long)

/** Calls the completion's `mln_user_data_release` at [release] on this thread. */
internal expect fun callCompletionRelease(release: Long, userData: Long)

/** Submits through [submit] and keeps the descriptor's fields for hand delivery. */
internal fun <T> capture(submit: ((Long) -> Unit) -> Deferred<T>): HandDeliveredCompletion<T> {
  var fields: Triple<Long, Long, Long>? = null
  val deferred = submit { descriptor ->
    fields =
      Triple(
        readAddress(descriptor + CompletionLayout.CALLBACK),
        readAddress(descriptor + CompletionLayout.USER_DATA),
        readAddress(descriptor + CompletionLayout.RELEASE_USER_DATA),
      )
  }
  val (callback, userData, release) = requireNotNull(fields)
  return HandDeliveredCompletion(deferred, callback, userData, release)
}

private fun generation(result: Long): ULong = readU64(result + CompletionResultLayout.GENERATION)

/** A completion whose value is its result's generation. */
internal fun handDeliveredGeneration(): HandDeliveredCompletion<ULong> = capture { call ->
  NativeArena().use { arena -> CompletionBridge.submit(arena, ::generation, call) }
}

/** The completion bridge and its upcall stubs, driven without native. */
class CompletionBridgeTest {
  /**
   * A native build whose element grew reports a value_size wider than this binding's element, so an
   * array result is read at that stride, as the generated decoders read it. A stride narrower than
   * the element cannot hold one.
   */
  @Test
  fun anArrayResultIsReadAtItsValueSize() {
    NativeArena().use { arena ->
      // Two coordinates, each followed by a member this binding does not know.
      val stride = 24
      val points = arena.allocate(2 * stride)
      listOf(1.0, 2.0, -1.0, 3.0, 4.0, -1.0).forEachIndexed { index, value ->
        writeF64(points + index * 8, value)
      }
      val result = arena.allocate(CompletionResultLayout.SIZEOF)
      writeAddress(result + CompletionResultLayout.VALUE, points)
      writeSize(result + CompletionResultLayout.VALUE_COUNT, 2u)
      writeU32(result + CompletionResultLayout.VALUE_SIZE, stride.toUInt())
      fun read() =
        readStrided(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          CompletionBridge.valueSize(result),
          16,
        ) {
          readLatLng(it)
        }

      assertEquals(listOf(LatLng(1.0, 2.0), LatLng(3.0, 4.0)), read())
      writeU32(result + CompletionResultLayout.VALUE_SIZE, 15u)
      assertFailsWith<IllegalArgumentException> { read() }
    }
  }

  @Test
  fun aFailedStatusBecomesItsExceptionWithTheDiagnostic() = runSuspendTest {
    val completion = handDeliveredGeneration()
    completion.deliver(Status.INVALID_ARGUMENT.rawValue, 0u, "the zoom is out of range")
    completion.release()
    val failure = assertFailsWithStatus(Status.INVALID_ARGUMENT) { completion.deferred.await() }
    assertEquals("the zoom is out of range", failure.diagnostic)
  }
}
