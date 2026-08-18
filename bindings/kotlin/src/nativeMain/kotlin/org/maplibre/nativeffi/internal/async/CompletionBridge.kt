@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.internal.async

import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.staticCFunction
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_completion
import org.maplibre.nativeffi.internal.c.mln_completion_result
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.runtime.CommandCompletion
import org.maplibre.nativeffi.runtime.CommandDisposition

@OptIn(ExperimentalForeignApi::class)
private interface CompletionState {
  fun complete(result: CPointer<mln_completion_result>)
}

internal object CompletionBridge {

  private class State<T>(
    private val convert: (CPointer<mln_completion_result>) -> T,
    private val acceptErrorStatus: Boolean,
  ) : CompletionState {
    val deferred = CompletableDeferred<T>()

    override fun complete(result: CPointer<mln_completion_result>) {
      val raw = result.pointed
      try {
        val status = raw.status
        if (status == MaplibreStatus.OK.nativeCode || acceptErrorStatus) {
          deferred.complete(convert(result))
        } else {
          val message =
            raw.diagnostic.data?.readBytes(raw.diagnostic.size.toInt())?.decodeToString().orEmpty()
          deferred.completeExceptionally(
            MaplibreException.forStatus(MaplibreStatus.fromNative(status), status, message)
          )
        }
      } catch (failure: Throwable) {
        deferred.completeExceptionally(failure)
      }
    }
  }

  fun <T> submit(
    convert: (CPointer<mln_completion_result>) -> T,
    call: (CPointer<mln_completion>) -> Int,
  ): Deferred<T> = submitInternal(convert, false, false, call)

  private fun <T> submitInternal(
    convert: (CPointer<mln_completion_result>) -> T,
    rejectSynchronously: Boolean,
    acceptErrorStatus: Boolean,
    call: (CPointer<mln_completion>) -> Int,
  ): Deferred<T> {
    val state = State(convert, acceptErrorStatus)
    val reference = StableRef.create<CompletionState>(state)
    var handedOff = false
    try {
      memScoped {
        val completion = alloc<mln_completion>()
        completion.size = sizeOf<mln_completion>().toUInt()
        completion.callback = staticCFunction(::completeNative)
        completion.user_data = reference.asCPointer()
        completion.release_user_data = staticCFunction(::releaseNative)
        val status = call(completion.ptr)
        handedOff = status == MaplibreStatus.OK.nativeCode
        Status.check(status)
      }
    } catch (failure: Throwable) {
      state.deferred.completeExceptionally(failure)
      if (!handedOff) reference.dispose()
      if (rejectSynchronously) throw failure
    }
    return state.deferred
  }

  fun unit(call: (CPointer<mln_completion>) -> Int): Deferred<Unit> = submit({ _ -> }, call)

  fun unitChecked(call: (CPointer<mln_completion>) -> Int): Deferred<Unit> =
    submitInternal({ _ -> }, true, false, call)

  fun command(call: (CPointer<mln_completion>) -> Int): Deferred<CommandCompletion> =
    submitInternal(
      { result ->
        CommandCompletion(
          CommandDisposition.fromNative(result.pointed.disposition.toInt()),
          result.pointed.generation,
          MaplibreStatus.fromNative(result.pointed.status),
          result.pointed.diagnostic.data
            ?.readBytes(result.pointed.diagnostic.size.toInt())
            ?.decodeToString()
            .orEmpty(),
        )
      },
      false,
      true,
      call,
    )
}

@OptIn(ExperimentalForeignApi::class)
private fun completeNative(userData: COpaquePointer?, result: CPointer<mln_completion_result>?) {
  if (userData != null && result != null)
    userData.asStableRef<CompletionState>().get().complete(result)
}

@OptIn(ExperimentalForeignApi::class)
private fun releaseNative(userData: COpaquePointer?) {
  userData?.asStableRef<CompletionState>()?.dispose()
}
