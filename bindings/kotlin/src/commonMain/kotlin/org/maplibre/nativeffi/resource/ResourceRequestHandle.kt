package org.maplibre.nativeffi.resource

/**
 * Owned handle for a resource provider request that Kotlin chose to handle. Platform actuals own
 * the native request carrier.
 */
public expect class ResourceRequestHandle : AutoCloseable {
  public fun complete(response: ResourceResponse)

  public fun isCancelled(): Boolean

  /**
   * Registers the callback that runs when MapLibre cancels this request.
   *
   * A request accepts one callback for its lifetime; a second registration throws
   * [org.maplibre.nativeffi.error.InvalidStateException], as does registration on a closed handle.
   * The callback runs at most once, on the thread that cancels the request: the owner thread inside
   * a map or runtime call, and a MapLibre thread otherwise. When MapLibre already cancelled the
   * request, the callback runs on the calling thread before this method returns, as part of this
   * call: a concurrent close on another thread does not wait for it. A request that this handle
   * completed never runs the callback.
   *
   * The callback returns quickly and uses this request alone: it may complete or close the request,
   * where a completion reports an invalid-state error for a cancelled request. Map and runtime
   * operations belong outside it. The binding contains a failure thrown from the callback, and
   * closing the handle from another thread waits for a callback that is already running.
   */
  public fun setCancelCallback(callback: () -> Unit)

  override fun close()
}
