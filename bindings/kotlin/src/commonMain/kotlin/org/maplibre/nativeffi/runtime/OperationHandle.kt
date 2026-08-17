package org.maplibre.nativeffi.runtime

/** Common asynchronous operation observer. */
public expect class OperationHandle<T> : AutoCloseable {
  public val isClosed: Boolean

  /** Returns whether the operation has reached a terminal disposition. */
  public fun poll(): Boolean

  /**
   * Waits for completion.
   *
   * A negative timeout waits without a deadline, zero performs a nonblocking check, and a positive
   * timeout waits for at most that many milliseconds.
   */
  public fun waitForCompletion(timeoutMillis: Long): Boolean

  /** Requests cancellation of this operation. */
  public fun cancel()

  /** Returns the completed operation's terminal status. */
  public fun terminalStatus(): org.maplibre.nativeffi.error.MaplibreStatus

  /** Copies the completed operation's diagnostic text. */
  public fun diagnostic(): String

  /** Finishes the operation without taking its typed result. */
  public fun finish()

  /** Releases this observer. */
  override fun close()
}
