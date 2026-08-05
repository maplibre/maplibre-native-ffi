package org.maplibre.nativeffi.resource

/**
 * Owned handle for a resource provider request that Kotlin chose to handle. Platform actuals own
 * the native request carrier.
 */
public expect class ResourceRequestHandle : AutoCloseable {
  public fun complete(response: ResourceResponse)

  public fun isCancelled(): Boolean

  override fun close()
}
