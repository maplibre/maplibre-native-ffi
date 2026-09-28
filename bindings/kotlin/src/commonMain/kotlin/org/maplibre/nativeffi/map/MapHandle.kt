package org.maplibre.nativeffi.map

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.runtime.RuntimeHandle

/** Owned map handle. Platform actuals own the native map carrier. */
public expect class MapHandle : GeneratedMapOperations {
  public val isClosed: Boolean

  public fun runtime(): RuntimeHandle

  /** Reports native map retirement. Queued events keep this map's source ID. */
  public fun close(): Deferred<Unit>
}
