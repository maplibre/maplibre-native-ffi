@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.internal.call

import kotlin.coroutines.Continuation
import kotlin.coroutines.resume
import kotlin.coroutines.suspendCoroutine
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.internal.callback.NativeRoots
import org.maplibre.nativeffi.internal.status.checkStatus

/**
 * Runs each thread-start call on a pthread of its own, because the page cannot wait for a thread to
 * start: a thread starts only after the page yields.
 *
 * [run] stages a thread start that the generated wrapper copies the call's arguments into, and
 * suspends until the wake shim posts the call's status. The wait does not cancel, because the call
 * writes to memory that the caller's arena holds until the call returns.
 */
internal actual object ThreadStart {
  private class Waiting {
    var caller: Continuation<Int>? = null
  }

  private var staged = 0

  /** The thread start of the call in progress, which its generated wrapper passes. */
  fun staged(): Int {
    check(staged != 0) { "a thread-start call ran outside ThreadStart.run" }
    return staged
  }

  actual suspend fun run(call: () -> Int): Int {
    val waiting = Waiting()
    val token = NativeRoots.retain(waiting)
    val start = mln_kw_thread_start_create(token.toInt())
    if (start == 0) {
      NativeRoots.release(token)
      checkStatus(Status.NATIVE_ERROR.rawValue) { "thread start allocation failed" }
    }
    val started =
      try {
        staged = start
        call()
      } catch (failure: Throwable) {
        discard(token, start)
        throw failure
      } finally {
        staged = 0
      }
    if (started != Status.OK.rawValue) {
      discard(token, start)
      checkStatus(started) { "could not start a thread for the call" }
    }
    // The page delivers the status from a later task, after this suspends.
    return suspendCoroutine { waiting.caller = it }
  }

  /** Resumes the caller of the thread-start call that [token] roots with its [status]. */
  fun deliver(token: Long, status: Int) {
    (NativeRoots.release(token) as? Waiting)?.caller?.resume(status)
  }

  private fun discard(token: Long, start: Int) {
    mln_kw_thread_start_destroy(start)
    NativeRoots.release(token)
  }
}

private fun mln_kw_thread_start_create(token: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_thread_start_create(token)")

private fun mln_kw_thread_start_destroy(start: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_thread_start_destroy(start)")
