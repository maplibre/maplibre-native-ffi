package org.maplibre.nativeffi.internal.loader

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.nio.charset.StandardCharsets
import java.util.concurrent.ConcurrentHashMap
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_buffer_view
import org.maplibre.nativeffi.internal.c.mln_completion
import org.maplibre.nativeffi.internal.c.mln_completion_callback
import org.maplibre.nativeffi.internal.c.mln_completion_release
import org.maplibre.nativeffi.internal.c.mln_completion_result
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.runtime.CommandCompletion
import org.maplibre.nativeffi.runtime.CommandDisposition

/** Bridges one native completion into an eager Kotlin [Deferred]. */
internal object CompletionBridge {
  private class State<T>(val convert: (MemorySegment) -> T, val acceptErrorStatus: Boolean) {
    val result = CompletableDeferred<T>()
    val arena: Arena = Arena.ofShared()
    val token: MemorySegment = arena.allocate(1)
  }

  private val states = ConcurrentHashMap<Long, State<*>>()
  private val callback =
    mln_completion_callback.allocate(
      { userData, result -> complete(userData.address(), result) },
      Arena.global(),
    )
  private val release =
    mln_completion_release.allocate(
      { userData -> states.remove(userData.address())?.arena?.close() },
      Arena.global(),
    )

  fun <T> submit(convert: (MemorySegment) -> T, call: (MemorySegment) -> Int): Deferred<T> =
    submitInternal(convert, false, false, call)

  private fun <T> submitInternal(
    convert: (MemorySegment) -> T,
    rejectSynchronously: Boolean,
    acceptErrorStatus: Boolean,
    call: (MemorySegment) -> Int,
  ): Deferred<T> {
    val state = State(convert, acceptErrorStatus)
    states[state.token.address()] = state
    try {
      Arena.ofConfined().use { arena ->
        val descriptor = mln_completion.allocate(arena)
        mln_completion.size(descriptor, mln_completion.sizeof().toInt())
        mln_completion.callback(descriptor, callback)
        mln_completion.user_data(descriptor, state.token)
        mln_completion.release_user_data(descriptor, release)
        Status.check(call(descriptor))
      }
    } catch (failure: Throwable) {
      if (states.remove(state.token.address(), state)) state.arena.close()
      if (rejectSynchronously) throw failure
      state.result.completeExceptionally(failure)
    }
    return state.result
  }

  fun command(call: (MemorySegment) -> Int): Deferred<CommandCompletion> =
    submitInternal(
      { result ->
        CommandCompletion(
          CommandDisposition.fromNative(mln_completion_result.disposition(result)),
          mln_completion_result.generation(result).toULong(),
          MaplibreStatus.fromNative(mln_completion_result.status(result)),
          diagnostic(result),
        )
      },
      false,
      true,
      call,
    )

  fun unit(call: (MemorySegment) -> Int): Deferred<Unit> = submit(convert = { _ -> }, call = call)

  fun unitChecked(call: (MemorySegment) -> Int): Deferred<Unit> =
    submitInternal(
      convert = { _ -> },
      rejectSynchronously = true,
      acceptErrorStatus = false,
      call = call,
    )

  @Suppress("UNCHECKED_CAST")
  private fun complete(address: Long, rawResult: MemorySegment) {
    val state = states[address] as? State<Any?> ?: return
    val result = rawResult.reinterpret(mln_completion_result.sizeof())
    try {
      val status = mln_completion_result.status(result)
      if (status == MaplibreStatus.OK.nativeCode || state.acceptErrorStatus) {
        state.result.complete(state.convert(result))
      } else {
        val message = diagnostic(result)
        state.result.completeExceptionally(
          MaplibreException.forStatus(MaplibreStatus.fromNative(status), status, message)
        )
      }
    } catch (failure: Throwable) {
      state.result.completeExceptionally(failure)
    }
  }

  private fun diagnostic(result: MemorySegment): String {
    val diagnostic = mln_completion_result.diagnostic(result)
    val bytes = mln_buffer_view.data(diagnostic)
    val size = mln_buffer_view.size(diagnostic)
    return if (bytes == MemorySegment.NULL || size == 0L) ""
    else
      String(
        bytes.reinterpret(size).toArray(java.lang.foreign.ValueLayout.JAVA_BYTE),
        StandardCharsets.UTF_8,
      )
  }
}
