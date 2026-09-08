package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/** Terminal disposition reported for an accepted runtime command. */
@JvmInline
public value class CommandDisposition(public val nativeValue: Int) {
  public companion object {
    public val COMMITTED: CommandDisposition = CommandDisposition(0)
    public val SUPERSEDED: CommandDisposition = CommandDisposition(1)
    public val FAILED: CommandDisposition = CommandDisposition(2)
    public val CANCELLED: CommandDisposition = CommandDisposition(3)

    internal fun fromNative(nativeValue: UInt): CommandDisposition =
      CommandDisposition(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): CommandDisposition = CommandDisposition(nativeValue)
  }
}
