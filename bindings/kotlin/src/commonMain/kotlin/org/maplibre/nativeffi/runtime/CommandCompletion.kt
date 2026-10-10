package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.CommandDisposition

/** Terminal metadata for one accepted ordered command. */
public data class CommandCompletion(
  public val disposition: CommandDisposition,
  /** State generation that the command published. */
  public val generation: ULong,
  public val status: MaplibreStatus,
  public val diagnostic: String,
)
