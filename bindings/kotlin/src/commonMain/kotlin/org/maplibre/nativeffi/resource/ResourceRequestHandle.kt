package org.maplibre.nativeffi.resource

/**
 * Owned handle for a resource provider request that Kotlin chose to handle. Platform actuals own
 * the native request carrier.
 */
public expect class ResourceRequestHandle : AutoCloseable {
  public fun complete(response: ResourceResponse)

  public fun isCancelled(): Boolean

  /**
   * Registers the callback that runs when MapLibre cancels this request, or null to clear it.
   *
   * MapLibre runs the callback once per request, on the thread that cancels it. A request this
   * handle already completed reports no cancellation. Registering on a request that MapLibre
   * already cancelled runs the callback before this call returns. Each call replaces the previous
   * callback.
   *
   * The callback returns quickly and uses this request alone: it may complete or close the request,
   * where a completion reports an invalid-state error for a cancelled request. Map and runtime
   * operations belong outside it. The binding contains a host failure thrown from the callback, and
   * closing the handle from another thread waits for a callback that is already running.
   */
  public fun setCancelCallback(callback: (() -> Unit)?)

  override fun close()
}
