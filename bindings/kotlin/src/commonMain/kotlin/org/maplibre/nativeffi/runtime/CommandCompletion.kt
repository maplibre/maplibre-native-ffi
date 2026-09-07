package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.MaplibreStatus

/** Terminal metadata for one accepted ordered command. */
public data class CommandCompletion(
  public val disposition: CommandDisposition,
  /**
   * State generation the command published. Native `uint64_t` preserved as a [Long] bit pattern;
   * format through `toULong()`.
   */
  public val generation: Long,
  public val status: MaplibreStatus,
  public val diagnostic: String,
)
