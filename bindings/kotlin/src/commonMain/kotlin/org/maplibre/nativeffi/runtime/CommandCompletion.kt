package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.MaplibreStatus

/** Terminal metadata for one accepted ordered command. */
public data class CommandCompletion(
  public val disposition: CommandDisposition,
  public val generation: ULong,
  public val status: MaplibreStatus,
  public val diagnostic: String,
)
