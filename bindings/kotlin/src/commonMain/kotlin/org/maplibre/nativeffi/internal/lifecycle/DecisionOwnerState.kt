package org.maplibre.nativeffi.internal.lifecycle

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
  /**
   * The release half of this state. It holds no host callback, so unreachable-owner cleanup can
   * hold it without keeping reachable an owner that its own cancel callback captures.
   */
  val core: DecisionOwnerCore = DecisionOwnerCore(typeName, ReleaseNative(handle, dispose))

  /** Whether close or completion has marked the owner, even while borrows still drain. */
  val isClosed: Boolean
    get() = core.isClosed

  fun <T> withLive(block: () -> T): T = core.withLiveHandle(block)

  /** Runs a native completion call, keeping the owner retryable when native rejects it. */
  fun complete(call: () -> Unit) {
    val operation = core.beginComplete()
    try {
      call()
      operation.markCompleted()
    } catch (error: Throwable) {
      operation.markNotReachedNative()
      throw error
    } finally {
      operation.close()
    }
  }

  fun close() {
    core.close()
  }

  /** Settles the callback's [raw] decision and returns the value native receives. */
  fun finishDecision(raw: UInt): UInt =
    when (raw) {
      passThrough -> finish(core.finishDecision(DecisionOwnerCore.Decision.PASS_THROUGH))
      accept -> finish(core.finishDecision(DecisionOwnerCore.Decision.ACCEPT))
      else -> finishException()
    }

  /** Settles a callback that threw instead of deciding. */
  fun finishException(): UInt = core.finishException()?.let(::finish) ?: UNKNOWN_DECISION

  private fun finish(decision: DecisionOwnerCore.Decision): UInt =
    if (decision == DecisionOwnerCore.Decision.PASS_THROUGH) passThrough else accept

  private class ReleaseNative(private val handle: Long, private val dispose: (Long) -> Unit) :
    () -> Unit {
    override fun invoke() {
      dispose(handle)
    }
  }

  private companion object {
    /** A value no decision enum uses, which native treats as a failed callback. */
    const val UNKNOWN_DECISION = UInt.MAX_VALUE
  }
}
