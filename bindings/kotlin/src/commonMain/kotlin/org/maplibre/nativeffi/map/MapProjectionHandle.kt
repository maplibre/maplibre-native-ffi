package org.maplibre.nativeffi.map

/**
 * Owned standalone projection snapshot created from a map.
 *
 * Every call is synchronous, runs on the calling thread, is internally serialized, and may be made
 * from any thread. A projection copies the map's transform state at creation, never observes later
 * map changes, and stays usable after its source map and runtime close.
 */
public expect class MapProjectionHandle :
  org.maplibre.nativeffi.generated.GeneratedMapProjectionOperations, AutoCloseable {
  public val isClosed: Boolean
}
