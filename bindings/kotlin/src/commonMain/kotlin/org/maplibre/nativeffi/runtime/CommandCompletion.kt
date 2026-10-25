package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.generated.Status

/**
 * Terminal metadata for one accepted ordered command.
 *
 * Unless the caller cancels it first, the command's `Deferred` completes inside its native
 * completion, after the runtime event handler has returned from the events the command raised, so
 * its completion handlers run there too: on a native thread on a threaded target, and in the page's
 * delivery drain on Kotlin/Wasm. A host that forwards events and completion handlers through one
 * FIFO queue keeps native order. Cancelling completes the `Deferred` at once on the cancelling
 * thread, while native still runs the command.
 */
public data class CommandCompletion(
  public val disposition: CommandDisposition,
  /** Map snapshot generation that a map command published, or zero for a runtime command. */
  public val generation: ULong,
  /** The first map render update that includes the command, or zero when [generation] is zero. */
  public val renderUpdateGeneration: ULong,
  public val status: Status,
  public val diagnostic: String,
)
