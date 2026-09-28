package org.maplibre.nativeffi.runtime

import kotlinx.coroutines.Deferred

/** Owns the runtime and reports native retirement through its close result. */
public expect class RuntimeHandle : org.maplibre.nativeffi.generated.GeneratedRuntimeOperations {
  public val isClosed: Boolean

  public fun close(): Deferred<Unit>
}
