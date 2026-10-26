@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.word
import org.maplibre.nativeffi.internal.call.NativeCall
import org.maplibre.nativeffi.internal.callback.CallbackOwner

/** The fixtures of `bindings/kotlin/wasm/test_support.c`, which the test module exports. */
internal object TestSupport {
  /** Calls the callback of the `mln_wake` at [wake] on the page. */
  fun wake(wake: Long) = mln_kw_test_wake(word(wake))

  /** Releases the `mln_wake` at [wake] on the page, as native's final call. */
  fun release(wake: Long) = mln_kw_test_release(word(wake))

  /**
   * Starts a thread that holds a mutex and wakes [held], then wakes [during] once the page contends
   * the mutex in [contend].
   */
  fun holdContention(held: Long, during: Long) {
    check(mln_kw_test_contention_hold(word(held), word(during)) == 0) {
      "the holder thread did not start"
    }
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

/**
 * A wake registration that a fixture calls in native's place, rooted in [owner] until its release
 * reaches Kotlin. [close] frees the descriptor at [address].
 */
internal class TestWake(callback: () -> Unit) : AutoCloseable {
  private val call = NativeCall(0L)
  val owner = CallbackOwner()
  val address: Long = call.writeWake(Wake { callback() }).also { call.accept(owner) }

  override fun close() = call.close()
}

private fun mln_kw_test_wake(wake: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_wake(wake)")

private fun mln_kw_test_release(wake: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_release(wake)")

private fun mln_kw_test_contention_hold(held: Int, during: Int): Int =
  js(
    "globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_contention_hold(held, during)",
  )

private fun mln_kw_test_contention_contend(): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_contention_contend()")

private fun mln_kw_test_pool_workers(all: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_pool_workers(all)")

private fun mln_kw_test_pool_hold(): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_pool_hold()")

private fun mln_kw_test_pool_release(): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_test_pool_release()")
