package org.maplibre.nativeffi.internal.async

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.internal.c.CompletionLayout
import org.maplibre.nativeffi.internal.c.CompletionResultLayout
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.callback.NativeRoots
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readAddress
import org.maplibre.nativeffi.internal.memory.readI32
import org.maplibre.nativeffi.internal.memory.readSize
import org.maplibre.nativeffi.internal.memory.readU32
import org.maplibre.nativeffi.internal.memory.readU64
import org.maplibre.nativeffi.internal.memory.readViewString
import org.maplibre.nativeffi.internal.memory.writeAddress
import org.maplibre.nativeffi.internal.memory.writeU32
import org.maplibre.nativeffi.runtime.CommandCompletion

/**
 * Bridges one native completion into an eager Kotlin [Deferred].
 *
 * Each submission builds an `mln_completion` whose `user_data` roots its state until native
 * releases it. A conversion reads the `mln_completion_result` at the address it receives, which
 * stays valid only while the conversion runs. Each submission call throws the mapped status error
 * when native rejects it synchronously.
 */
@OptIn(ExperimentalAtomicApi::class)
internal object CompletionBridge {
  private val pending = AtomicInt(0)

  /** Counts the completions native holds and has not yet released. */
  fun pendingCountForTesting(): Int = pending.load()

  private class State<T>(
    private val convert: (Long) -> T,
    private val acceptErrorStatus: Boolean,
    private val closeDropped: (T) -> Unit,
  ) {
    val deferred = CompletableDeferred<T>()

    fun complete(result: Long) {
      try {
        val status = readI32(result + CompletionResultLayout.STATUS)
        if (status == MaplibreStatus.OK.nativeCode || acceptErrorStatus) {
          val value = convert(result)
          if (!deferred.complete(value)) closeDropped(value)
        } else {
          deferred.completeExceptionally(
            MaplibreException.forStatus(
              MaplibreStatus.fromNative(status),
              status,
              diagnostic(result),
            )
          )
        }
      } catch (failure: Throwable) {
        deferred.completeExceptionally(failure)
      }
    }
  }

  fun <T> submit(convert: (Long) -> T, call: (Long) -> Unit): Deferred<T> =
    submitInternal(convert, false, call)

  /**
   * Submits a completion that produces an owned handle wrapper, handing the wrapper to
   * [closeDropped] when the caller cancelled the deferred before the value arrived.
   */
  fun <T> submitOwned(
    convert: (Long) -> T,
    closeDropped: (T) -> Unit,
    disposeUnadopted: (Long) -> Unit,
    call: (Long) -> Unit,
  ): Deferred<T> =
    submitInternal(
      { result -> adoptOwned(result, disposeUnadopted, convert) },
      false,
      call,
      closeDropped,
    )

  fun unit(call: (Long) -> Unit): Deferred<Unit> = submit({}, call)

  fun command(call: (Long) -> Unit): Deferred<CommandCompletion> =
    submitInternal(::commandCompletion, true, call)

  private fun <T> submitInternal(
    convert: (Long) -> T,
    acceptErrorStatus: Boolean,
    call: (Long) -> Unit,
    closeDropped: (T) -> Unit = {},
  ): Deferred<T> {
    val state = State(convert, acceptErrorStatus, closeDropped)
    val token = NativeRoots.retain(state)
    pending.addAndFetch(1)
    try {
      NativeArena().use { arena ->
        val completion = arena.allocate(CompletionLayout.SIZEOF)
        writeU32(completion + CompletionLayout.SIZE, CompletionLayout.SIZEOF.toUInt())
        writeAddress(completion + CompletionLayout.CALLBACK, UpcallStubs.completion)
        writeAddress(completion + CompletionLayout.USER_DATA, token)
        writeAddress(completion + CompletionLayout.RELEASE_USER_DATA, UpcallStubs.completionRelease)
        call(completion)
      }
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
    if (result != 0L) (NativeRoots.get(userData) as? State<*>)?.complete(result)
  }

  /** Drops the state once native calls the completion's `release_user_data`. */
  fun release(userData: Long) {
    if (NativeRoots.release(userData) is State<*>) pending.addAndFetch(-1)
  }

  private fun commandCompletion(result: Long): CommandCompletion =
    CommandCompletion(
      CommandDisposition(readU32(result + CompletionResultLayout.DISPOSITION)),
      readU64(result + CompletionResultLayout.GENERATION),
      MaplibreStatus.fromNative(readI32(result + CompletionResultLayout.STATUS)),
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
}
