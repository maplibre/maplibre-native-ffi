package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.callback.CallbackAdmission
import org.maplibre.nativeffi.internal.status.Status

/** Platform-neutral release-state bookkeeping for native handles. */
@OptIn(ExperimentalAtomicApi::class)
internal class HandleStateCore(
  private val typeName: String,
  private val handleId: Long,
  vararg parents: Any,
  dispose: ((Long) -> Unit)? = null,
) : OwnerState {
  @Suppress("unused") private val parents: Array<out Any> = parents
  val leakReport: LeakReport = LeakReport(typeName, handleId, dispose = dispose)
  private val releaseState = AtomicInt(STATE_LIVE)
  private val readers = AtomicInt(0)
  private val retirement = AtomicReference<CompletableDeferred<Unit>?>(null)

  init {
    Status.requireArgument(handleId != 0L) { "$typeName handle must not be zero" }
  }

  fun requireLive() {
    when (releaseState.load()) {
      STATE_LIVE -> return
      STATE_RELEASING -> throw Status.closing(typeName)
      else -> throw Status.closed(typeName)
    }
  }

  /** Runs [block] after checking that this wrapper still owns its native handle. */
  fun <T> withLive(block: () -> T): T {
    while (true) {
      val count = readers.load()
      if (count < 0) {
        requireLive()
        throw Status.closing(typeName)
      }
      check(count < Int.MAX_VALUE)
      if (readers.compareAndSet(count, count + 1)) break
    }
    try {
      requireLive()
      return block()
    } finally {
      readers.addAndFetch(-1)
    }
  }

  fun isReleased(): Boolean = releaseState.load() == STATE_CLOSED

  override fun handle(): Long {
    requireLive()
    return handleId
  }

  override fun <T> read(block: (Long) -> T): T = withLive { block(handleId) }

  override fun issued(): Long = handleId

  override fun closeHandle(name: String, call: (Long) -> Unit) {
    closeOnce({
      CallbackAdmission.check(handleId, name)
      call(handleId)
    })
  }

  /** Runs the owner's asynchronous release once; later calls share its result. */
  fun retireHandle(call: (Long) -> Deferred<Unit>): Deferred<Unit> = retire({ call(handleId) })

  /**
   * Acquires the exclusive close lease before an asynchronous native close starts.
   *
   * Returns false when the handle is already closed. The caller must pair a true result with
   * [completeClose] or [abortClose].
   */
  fun beginClose(): Boolean {
    if (releaseState.load() == STATE_CLOSED) return false
    if (!readers.compareAndSet(0, -1)) {
      if (readers.load() > 0) throw Status.inUse(typeName)
      if (releaseState.load() == STATE_CLOSED) return false
      throw Status.closing(typeName)
    }
    if (!releaseState.compareAndSet(STATE_LIVE, STATE_RELEASING)) {
      when (releaseState.load()) {
        STATE_CLOSED -> return false
        else -> throw Status.closing(typeName)
      }
    }
    return true
  }

  fun completeClose(afterSuccess: () -> Unit = {}) {
    check(releaseState.load() == STATE_RELEASING)
    leakReport.markReleased()
    releaseState.store(STATE_CLOSED)
    afterSuccess()
  }

  fun abortClose() {
    check(releaseState.compareAndSet(STATE_RELEASING, STATE_LIVE))
    readers.store(0)
  }

  /**
   * Claims this handle's one asynchronous close and takes the close lease with it.
   *
   * The first caller gets [claim] back and owns the close; every later caller gets the deferred the
   * first one published, so a repeated or concurrent close awaits the same native teardown.
   */
  fun claimRetirement(claim: CompletableDeferred<Unit>): Deferred<Unit> {
    while (true) {
      retirement.load()?.let {
        return it
      }
      if (retirement.compareAndSet(null, claim)) {
        try {
          check(beginClose()) { "$typeName retired without a claim" }
        } catch (failure: Throwable) {
          retirement.compareAndSet(claim, null)
          claim.completeExceptionally(failure)
          throw failure
        }
        return claim
      }
    }
  }

  /** Reports a rejected close to its waiters and leaves the handle available for a retry. */
  fun rejectRetirement(claim: CompletableDeferred<Unit>, failure: Throwable) {
    abortClose()
    check(retirement.compareAndSet(claim, null))
    claim.completeExceptionally(failure)
  }

  fun retire(call: () -> Deferred<Unit>, afterSuccess: () -> Unit = {}): Deferred<Unit> {
    val claim = CompletableDeferred<Unit>()
    val existing = claimRetirement(claim)
    if (existing !== claim) return existing
    val completion =
      try {
        call()
      } catch (failure: Throwable) {
        rejectRetirement(claim, failure)
        return claim
      }
    completeClose(afterSuccess)
    completion.invokeOnCompletion { failure ->
      if (failure == null) claim.complete(Unit) else claim.completeExceptionally(failure)
    }
    return claim
  }

  fun closeOnce(destroy: () -> Unit, afterSuccess: () -> Unit = {}) {
    if (!beginClose()) return
    try {
      destroy()
    } catch (error: Throwable) {
      abortClose()
      throw error
    }
    completeClose(afterSuccess)
  }

  @OptIn(ExperimentalAtomicApi::class)
  internal class LeakReport(
    private val typeName: String,
    private val handleId: Long,
    private val writeLine: (String) -> Unit = { message -> println(message) },
    private val dispose: ((Long) -> Unit)? = null,
  ) {
    private val released = AtomicInt(0)

    fun markReleased() {
      released.store(1)
    }

    /**
     * Disposes a handle nobody released, then reports the leak through [writeLine], with the
     * disposal's failure when it had one. Runs once, and never for a released handle.
     */
    fun report() {
      if (!released.compareAndSet(0, 1)) return
      val failure =
        try {
          dispose?.invoke(handleId)
          null
        } catch (error: Throwable) {
          error
        }
      val leak = "Leaked $typeName native handle 0x${handleId.toString(16)}; close it explicitly."
      writeLine(if (failure == null) leak else "$leak Disposing it failed: ${failure.message}")
    }
  }

  private companion object {
    private const val STATE_LIVE = 0
    private const val STATE_RELEASING = 1
    private const val STATE_CLOSED = 2
  }
}
