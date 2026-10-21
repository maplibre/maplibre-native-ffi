package org.maplibre.nativeffi.internal.async

import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.generated.Status
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
import org.maplibre.nativeffi.runtime.CommandCompletion

/**
 * Bridges one native completion into an eager Kotlin [Deferred].
 *
 * Each submission builds an `mln_completion` whose `user_data` roots its state until native
 * releases it, or on a target with a [HostDelivery] queue, until the page delivers the result. A
 * conversion reads the `mln_completion_result` at the address it receives, which stays valid only
 * while the conversion runs. Each submission call throws the mapped status error when native
 * rejects it synchronously.
 */
internal object CompletionBridge {
  private class State<T>(
    private val convert: (Long) -> T,
    private val acceptErrorStatus: Boolean,
    private val closeDropped: (T) -> Unit,
  ) {
    val deferred = CompletableDeferred<T>()

    fun complete(result: Long) {
      try {
        val status = readI32(result + CompletionResultLayout.STATUS)
        if (status == Status.OK.rawValue || acceptErrorStatus) {
          val value = convert(result)
          if (!deferred.complete(value)) closeDropped(value)
        } else {
          deferred.completeExceptionally(MaplibreException(Status(status), diagnostic(result)))
        }
      } catch (failure: Throwable) {
        deferred.completeExceptionally(failure)
      }
    }
  }

  fun <T> submit(arena: NativeArena, convert: (Long) -> T, call: (Long) -> Unit): Deferred<T> =
    submitInternal(arena, convert, false, call)

  /**
   * Submits a completion that produces an owned handle wrapper, handing the wrapper to
   * [closeDropped] when the caller cancelled the deferred before the value arrived.
   */
  fun <T> submitOwned(
    arena: NativeArena,
    convert: (Long) -> T,
    closeDropped: (T) -> Unit,
    disposeUnadopted: (Long) -> Unit,
    call: (Long) -> Unit,
  ): Deferred<T> =
    submitInternal(
      arena,
      { result -> adoptOwned(result, disposeUnadopted, convert) },
      false,
      call,
      closeDropped,
    )

  fun unit(arena: NativeArena, call: (Long) -> Unit): Deferred<Unit> = submit(arena, {}, call)

  fun command(arena: NativeArena, call: (Long) -> Unit): Deferred<CommandCompletion> =
    submitInternal(arena, ::commandCompletion, true, call)

  private fun <T> submitInternal(
    arena: NativeArena,
    convert: (Long) -> T,
    acceptErrorStatus: Boolean,
    call: (Long) -> Unit,
    closeDropped: (T) -> Unit = {},
  ): Deferred<T> {
    val state = State(convert, acceptErrorStatus, closeDropped)
    val token = NativeRoots.retain(state)
    try {
      val completion = arena.allocate(CompletionLayout.SIZEOF)
      writeU32(completion + CompletionLayout.SIZE, CompletionLayout.SIZEOF.toUInt())
      writeAddress(completion + CompletionLayout.USER_DATA, token)
      val queue = HostDelivery.queue
      if (queue != 0L) {
        // The page drains the result into [complete] and then [release].
        writeI64(completion + CompletionLayout.QUEUE, queue)
      } else {
        writeAddress(completion + CompletionLayout.CALLBACK, UpcallStubs.completion)
        writeAddress(completion + CompletionLayout.RELEASE_USER_DATA, UpcallStubs.completionRelease)
      }
      call(completion)
    } catch (failure: Throwable) {
      // A throw from [call] means native rejected the submission, which then never takes the
      // token, so nothing else releases it. Nothing in [call] may throw once native accepted.
      release(token)
      throw failure
    }
    return state.deferred
  }

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
      CommandDisposition(readU32(result + CompletionResultLayout.DISPOSITION)),
      readU64(result + CompletionResultLayout.GENERATION),
      Status(readI32(result + CompletionResultLayout.STATUS)),
      diagnostic(result),
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
