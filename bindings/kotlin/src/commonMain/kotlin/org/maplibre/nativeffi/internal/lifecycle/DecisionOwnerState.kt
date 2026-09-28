package org.maplibre.nativeffi.internal.lifecycle

import org.maplibre.nativeffi.internal.callback.DecisionCancelRegistration
import org.maplibre.nativeffi.internal.callback.DecisionCancelSetResult
import org.maplibre.nativeffi.internal.callback.DecisionCancelState
import org.maplibre.nativeffi.internal.status.Status

/**
 * The owner state behind a handle that a native callback issues and then decides about.
 *
 * The callback's result settles ownership: [accept] keeps the handle with the host, and
 * [passThrough] hands it back to native. Completion, cancel registration, and close all borrow the
 * live handle, and the native release waits until those borrows drain.
 */
internal class DecisionOwnerState(
  typeName: String,
  handle: Long,
  private val accept: UInt,
  private val passThrough: UInt,
  dispose: (Long) -> Unit,
) {
  private val registration = DecisionCancelRegistration()
  private val cancel = DecisionCancelState(typeName, registration)

  /**
   * The release half of this state. It holds no host callback, so unreachable-owner cleanup can
   * hold it without keeping an owner that a cancel callback captures reachable.
   */
  val core: DecisionOwnerCore =
    DecisionOwnerCore(typeName, ReleaseNative(handle, dispose, registration))

  /** Whether close or completion has marked the owner, even while borrows still drain. */
  val isClosed: Boolean
    get() = core.isClosed

  fun <T> withLive(block: () -> T): T = core.withLiveHandle(block)

  /** Runs a native completion call, keeping the owner retryable when native rejects it. */
  fun complete(call: () -> Int) {
    val operation = core.beginComplete()
    try {
      val status = call()
      if (status == 0) operation.markCompleted() else operation.markNotReachedNative()
      Status.check(status)
    } catch (error: Throwable) {
      operation.markNotReachedNative()
      throw error
    } finally {
      operation.close()
    }
  }

  /** Installs [callback] and returns whether native reported the owner already cancelled. */
  fun registerCancel(
    callback: () -> Unit,
    setNative: (token: Long) -> DecisionCancelSetResult,
  ): Boolean = core.withLiveHandle {
    val cancelled = cancel.register(callback, setNative) != null
    if (core.isClosed) cancel.drop()
    cancelled
  }

  fun close() {
    cancel.drop()
    core.close()
    cancel.drop()
  }

  /** Settles the callback's [raw] decision and returns the value native receives. */
  fun finishDecision(raw: UInt): UInt =
    when (raw) {
      passThrough -> finish(core.finishDecision(DecisionOwnerCore.Decision.PASS_THROUGH))
      accept -> finish(core.finishDecision(DecisionOwnerCore.Decision.ACCEPT))
      else -> finishException()
    }

  /** Settles a callback that threw instead of deciding. */
  fun finishException(): UInt =
    core.finishException()?.let(::finish) ?: handedBack(UNKNOWN_DECISION)

  private fun finish(decision: DecisionOwnerCore.Decision): UInt =
    if (decision == DecisionOwnerCore.Decision.PASS_THROUGH) handedBack(passThrough) else accept

  private fun handedBack(result: UInt): UInt {
    cancel.drop()
    registration.dispose()
    return result
  }

  private class ReleaseNative(
    private val handle: Long,
    private val dispose: (Long) -> Unit,
    private val registration: DecisionCancelRegistration,
  ) : () -> Unit {
    override fun invoke() {
      try {
        dispose(handle)
      } finally {
        registration.dispose()
      }
    }
  }

  private companion object {
    /** A value no decision enum uses, which native treats as a failed callback. */
    const val UNKNOWN_DECISION = UInt.MAX_VALUE
  }
}
