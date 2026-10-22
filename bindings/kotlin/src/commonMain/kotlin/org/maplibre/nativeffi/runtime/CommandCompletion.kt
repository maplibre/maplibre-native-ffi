package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.generated.Status

/** Terminal metadata for one accepted ordered command. */
public data class CommandCompletion(
  public val disposition: CommandDisposition,
  /** State generation that the command published. */
  public val generation: ULong,
  /** The first map render update that includes the command. */
  public val renderUpdateGeneration: ULong,
  public val status: Status,
  public val diagnostic: String,
)
