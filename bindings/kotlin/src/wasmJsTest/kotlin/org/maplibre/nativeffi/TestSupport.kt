@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.word

/** The fixtures of `bindings/kotlin/wasm/test_support.c`, which the test module exports. */
internal object TestSupport {
  /** Calls the `mln_completion_callback` at [callback] on the page. */
  fun callCompletion(callback: Long, userData: Long, result: Long) =
    mln_kw_test_call_completion(word(callback), word(userData), word(result))

  /** Calls the `mln_user_data_release` at [release] on the page. */
  fun callRelease(release: Long, userData: Long) =
    mln_kw_test_call_release(word(release), word(userData))

  /** Makes native emit an error log record on the page. */
  fun logError() = mln_kw_test_log_error()

  /**
   * Starts a thread that holds a mutex, and once the page contends the mutex in [contend], emits an
   * error log record before it lets go. Returns once the thread holds the mutex.
   */
  fun holdContention() {
    check(mln_kw_test_contention_hold() == 0) { "the holder thread did not take the mutex" }
  }

  /** Contends the holder's mutex, and returns whether the call spun until the holder let go. */
  fun contend(): Boolean = mln_kw_test_contention_contend() != 0

  /** The Workers that wait in the module's pthread pool. */
  fun idleWorkers(): Int = mln_kw_test_pool_workers(0)

  /** Every Worker of the module: those in the pool and those that run a thread. */
  fun allWorkers(): Int = mln_kw_test_pool_workers(1)

  /**
   * Runs [block] while a waiting thread holds every Worker that the pool had, so a thread that
   * [block] starts needs a new Worker, which starts only after the page yields.
   */
  suspend fun <T> withEmptyPool(block: suspend () -> T): T =
    try {
      check(mln_kw_test_pool_hold() >= 0) { "a thread holding a pool Worker did not start" }
      block()
    } finally {
      mln_kw_test_pool_release()
    }
}

private fun mln_kw_test_call_completion(callback: Int, userData: Int, result: Int): Unit =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_call_completion(callback, userData, result)",
  )

private fun mln_kw_test_call_release(release: Int, userData: Int): Unit =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_call_release(release, userData)",
  )

private fun mln_kw_test_log_error(): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_log_error()")

private fun mln_kw_test_contention_hold(): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_contention_hold()")

private fun mln_kw_test_contention_contend(): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_contention_contend()")

private fun mln_kw_test_pool_workers(all: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_pool_workers(all)")

private fun mln_kw_test_pool_hold(): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_pool_hold()")

private fun mln_kw_test_pool_release(): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_pool_release()")
