package org.maplibre.nativeffi.internal.async

import java.util.concurrent.ConcurrentHashMap
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.runtime.CommandCompletion

internal object CompletionBridge {
  private class State<T>(
    val convert: (MaplibreNativeC.mln_completion_result) -> T,
    val acceptErrorStatus: Boolean,
    val closeDropped: (T) -> Unit,
  ) {
    val deferred = CompletableDeferred<T>()
    val token = BytePointer(1L)
  }

  private val states = ConcurrentHashMap<Long, State<*>>()

  /** Counts the completions native holds and has not yet released. */
  fun pendingCountForTesting(): Int = states.size

  private val callback =
    object : MaplibreNativeC.mln_completion_callback() {
      override fun call(userData: Pointer?, result: MaplibreNativeC.mln_completion_result?) {
        if (userData != null && result != null) complete(userData.address(), result)
      }
    }
  private val release =
    object : MaplibreNativeC.mln_completion_release() {
      override fun call(userData: Pointer?) {
        userData ?: return
        states.remove(userData.address())?.token?.close()
      }
    }

  fun <T> submit(
    convert: (MaplibreNativeC.mln_completion_result) -> T,
    call: (MaplibreNativeC.mln_completion) -> Unit,
  ): Deferred<T> = submitInternal(convert, false, false, call)

  /**
   * Submits a completion that produces an owned handle wrapper, handing the wrapper to
   * [closeDropped] when the caller cancelled the deferred before the value arrived.
   */
  fun <T> submitOwned(
    convert: (MaplibreNativeC.mln_completion_result) -> T,
    closeDropped: (T) -> Unit,
    disposeUnadopted: (MaplibreNativeC.mln_completion_result) -> Unit,
    call: (MaplibreNativeC.mln_completion) -> Unit,
  ): Deferred<T> =
    submitInternal(
      { result -> adoptOwned(result, disposeUnadopted, convert) },
      false,
      false,
      call,
      closeDropped,
    )

  private fun <T> submitInternal(
    convert: (MaplibreNativeC.mln_completion_result) -> T,
    rejectSynchronously: Boolean,
    acceptErrorStatus: Boolean,
    call: (MaplibreNativeC.mln_completion) -> Unit,
    closeDropped: (T) -> Unit = {},
  ): Deferred<T> {
    val state = State(convert, acceptErrorStatus, closeDropped)
    states[state.token.address()] = state
    try {
      MaplibreNativeC.mln_completion().use { completion ->
        completion.size(completion.sizeof())
        completion.callback(callback)
        completion.user_data(state.token)
        completion.release_user_data(release)
        call(completion)
      }
    } catch (failure: Throwable) {
      if (states.remove(state.token.address(), state)) state.token.close()
      if (rejectSynchronously) throw failure
      state.deferred.completeExceptionally(failure)
    }
    return state.deferred
  }

  fun unit(call: (MaplibreNativeC.mln_completion) -> Unit): Deferred<Unit> = submit({ _ -> }, call)

  fun unitChecked(call: (MaplibreNativeC.mln_completion) -> Unit): Deferred<Unit> =
    submitInternal({ _ -> }, true, false, call)

  fun command(call: (MaplibreNativeC.mln_completion) -> Unit): Deferred<CommandCompletion> =
    submitInternal(::commandCompletion, false, true, call)

  /** Submits an ordered command and throws instead of deferring a synchronous rejection. */
  fun commandChecked(call: (MaplibreNativeC.mln_completion) -> Unit): Deferred<CommandCompletion> =
    submitInternal(::commandCompletion, true, true, call)

  private fun commandCompletion(result: MaplibreNativeC.mln_completion_result): CommandCompletion =
    CommandCompletion(
      CommandDisposition(result.disposition().toUInt()),
      result.generation().toULong(),
      MaplibreStatus.fromNative(result.status()),
      diagnostic(result),
    )

  @Suppress("UNCHECKED_CAST")
  private fun complete(address: Long, result: MaplibreNativeC.mln_completion_result) {
    val state = states[address] as? State<Any?> ?: return
    try {
      val status = result.status()
      if (status == MaplibreStatus.OK.nativeCode || state.acceptErrorStatus) {
        val value = state.convert(result)
        if (!state.deferred.complete(value)) state.closeDropped(value)
      } else {
        val message = diagnostic(result)
        state.deferred.completeExceptionally(
          MaplibreException.forStatus(MaplibreStatus.fromNative(status), status, message)
        )
      }
    } catch (failure: Throwable) {
      state.deferred.completeExceptionally(failure)
    }
  }

  private fun diagnostic(result: MaplibreNativeC.mln_completion_result): String {
    val diagnostic = result.diagnostic()
    return JavaCppSupport.byteArray(diagnostic.data(), diagnostic.size()).decodeToString()
  }
}
