package org.maplibre.nativeffi.internal.async

import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.CommandCompletion
import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.internal.c.CompletionLayout
import org.maplibre.nativeffi.internal.c.CompletionResultLayout
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.callback.NativeRoots
import org.maplibre.nativeffi.internal.delivery.HostDelivery
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readAddress
import org.maplibre.nativeffi.internal.memory.readI32
import org.maplibre.nativeffi.internal.memory.readSize
import org.maplibre.nativeffi.internal.memory.readU32
import org.maplibre.nativeffi.internal.memory.readU64
import org.maplibre.nativeffi.internal.memory.readViewString
import org.maplibre.nativeffi.internal.memory.writeAddress
import org.maplibre.nativeffi.internal.memory.writeI64
import org.maplibre.nativeffi.internal.memory.writeU32

/**
 * Bridges one native completion into an eager Kotlin [Deferred].
 *
 * Each submission builds an `mln_completion` whose `user_data` roots its state until native
 * releases it. On a target with a [HostDelivery] queue, the page runs both calls as it dispatches
 * the queue. A conversion reads the `mln_completion_result` at the address it receives, which stays
 * valid only while the conversion runs. Each submission call throws the mapped status error when
 * native rejects it synchronously. A result with a status other than OK, including a failed or
 * cancelled command, completes the [Deferred] exceptionally with [MaplibreException].
 */
internal object CompletionBridge {
  private class State<T>(private val convert: (Long) -> T) {
    val deferred = CompletableDeferred<T>()

    fun complete(result: Long) {
      try {
        val status = readI32(result + CompletionResultLayout.STATUS)
        if (status == Status.OK.rawValue) {
          deferred.complete(convert(result))
        } else {
          deferred.completeExceptionally(
            MaplibreException(
              Status(status),
              diagnostic(result),
              readU64(result + CompletionResultLayout.GENERATION),
            ),
          )
        }
      } catch (failure: Throwable) {
        deferred.completeExceptionally(failure)
      }
    }
  }

  fun <T> submit(arena: NativeArena, convert: (Long) -> T, call: (Long) -> Unit): Deferred<T> {
    val state = State(convert)
    val token = NativeRoots.retain(state)
    try {
      val completion = arena.allocate(CompletionLayout.SIZEOF)
      writeU32(completion + CompletionLayout.SIZE, CompletionLayout.SIZEOF.toUInt())
      writeAddress(completion + CompletionLayout.USER_DATA, token)
      writeAddress(completion + CompletionLayout.CALLBACK, UpcallStubs.completion)
      writeAddress(completion + CompletionLayout.RELEASE_USER_DATA, UpcallStubs.completionRelease)
      writeI64(completion + CompletionLayout.QUEUE, HostDelivery.queue)
      call(completion)
    } catch (failure: Throwable) {
      // A throw from [call] means native rejected the submission, which then never takes the
      // token, so nothing else releases it. Nothing in [call] may throw once native accepted.
      release(token)
      throw failure
    }
    return state.deferred
  }

  fun unit(arena: NativeArena, call: (Long) -> Unit): Deferred<Unit> = submit(arena, {}, call)

  fun command(arena: NativeArena, call: (Long) -> Unit): Deferred<CommandCompletion> =
    submit(arena, ::commandCompletion, call)

  /** Delivers the result that native passes to the `mln_completion_callback`. */
  fun complete(userData: Long, result: Long) {
    (NativeRoots.get(userData) as? State<*>)?.complete(result)
  }

  /** Drops the state once native calls the completion's `release_user_data`. */
  fun release(userData: Long) {
    NativeRoots.release(userData)
  }

  private fun commandCompletion(result: Long): CommandCompletion =
    CommandCompletion(
      readU64(result + CompletionResultLayout.GENERATION),
      readU64(result + CompletionResultLayout.RENDER_UPDATE_GENERATION),
    )

  private fun diagnostic(result: Long): String =
    readViewString(result + CompletionResultLayout.DIAGNOSTIC)

  /** The `value` of the result at [result], which native must have supplied. */
  fun value(result: Long): Long =
    readAddress(result + CompletionResultLayout.VALUE).also {
      check(it != 0L) { "native completion omitted its result value" }
    }

  /** The `value` pointer of the result at [result], which may be null. */
  fun valuePointer(result: Long): Long = readAddress(result + CompletionResultLayout.VALUE)

  /** The `value_count` of the result at [result]. */
  fun valueCount(result: Long): ULong = readSize(result + CompletionResultLayout.VALUE_COUNT)

  /** The `value_size` of the result at [result]: the byte stride of its array. */
  fun valueSize(result: Long): ULong = readU32(result + CompletionResultLayout.VALUE_SIZE).toULong()
}
