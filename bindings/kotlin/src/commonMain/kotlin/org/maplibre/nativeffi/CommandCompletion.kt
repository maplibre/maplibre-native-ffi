package org.maplibre.nativeffi

import kotlin.jvm.JvmName
import kotlin.jvm.JvmStatic

/**
 * The generations that one committed ordered command published.
 *
 * A command's [Completion] completes with this value only when the command commits. When the
 * command fails or native cancels it, the [Completion] completes exceptionally with a
 * [org.maplibre.nativeffi.MaplibreException] that carries the status, the diagnostic, and the
 * generation the command published.
 *
 * The command's [Completion] completes inside its native completion, after the runtime event
 * handler has returned from the events the command raised, so what its completion runs, such as a
 * Java stage, starts there too: on a native thread on a threaded target, and in the page's dispatch
 * of the delivery queue on Kotlin/Wasm. A host that forwards events and completion handlers through
 * one FIFO queue keeps native order.
 */
public data class CommandCompletion(
  /** Map snapshot generation that a map command published, or zero for a runtime command. */
  @get:JvmName("getGeneration") public val generation: ULong,
  /** The first map render update that includes the command, or zero when [generation] is zero. */
  @get:JvmName("getRenderUpdateGeneration") public val renderUpdateGeneration: ULong,
) {
  public companion object {
    /** Creates a value, for Java, which can't call the constructor. */
    @JvmStatic
    @PublishedApi
    internal fun of(generation: Long, renderUpdateGeneration: Long): CommandCompletion =
      CommandCompletion(generation.toULong(), renderUpdateGeneration.toULong())
  }
}
