package org.maplibre.nativeffi.resource

import java.lang.foreign.MemorySegment
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelBridge
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelRegistration
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelState
import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.lifecycle.UnreachableActions
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status

/** Owned JVM FFM handle for a resource provider request. */
public actual class ResourceRequestHandle
internal constructor(
  private val handle: NativeResourceRequest,
  private val completer: (NativeResourceRequest, ResourceResponse) -> Int =
    NativeAccess::completeResourceRequest,
  private val cancellationChecker: (NativeResourceRequest) -> Boolean =
    NativeAccess::isResourceRequestCancelled,
  private val cancelCallbackSetter: (NativeResourceRequest, MemorySegment, MemorySegment) -> Int =
    NativeAccess::setResourceRequestCancelCallback,
  releaser: (NativeResourceRequest) -> Unit = NativeAccess::releaseResourceRequest,
) : AutoCloseable {
  private val cancelRegistration = ResourceRequestCancelRegistration()
  private val cancelState = ResourceRequestCancelState(cancelRegistration)
  private val core =
    ResourceRequestHandleCore(ReleaseNativeRequest(handle, releaser, cancelRegistration))

  init {
    UnreachableActions.register(this, CloseWhenUnreachableAction(core))
  }

  public actual fun complete(response: ResourceResponse) {
    NativeAccess.ensureLoaded()
    val operation = core.beginComplete()
    var reachedNative = false
    try {
      val nativeStatus = completer(handle, response).also { reachedNative = true }
      val nativeFailure =
        if (nativeStatus == MaplibreStatus.OK.nativeCode) null else Status.exception(nativeStatus)
      operation.markCompleted()
      nativeFailure?.let { throw it }
    } catch (error: Throwable) {
      if (reachedNative) {
        operation.markCompleted()
      } else {
        operation.markNotReachedNative()
      }
      throw error
    } finally {
      operation.close()
    }
  }

  public actual fun isCancelled(): Boolean {
    NativeAccess.ensureLoaded()
    return core.withLiveHandle { cancellationChecker(handle) }
  }

  public actual fun setCancelCallback(callback: (() -> Unit)?) {
    NativeAccess.ensureLoaded()
    core.withLiveHandle {
      // The callback lands before C hears about it, so a request that MapLibre already cancelled
      // runs it inside this call.
      cancelState.store(callback)
      val status =
        if (callback == null) {
          cancelCallbackSetter(handle, MemorySegment.NULL, MemorySegment.NULL)
        } else {
          cancelCallbackSetter(
            handle,
            ResourceRequestCancelBridge.stub,
            ResourceRequestCancelBridge.userData(cancelState.token()),
          )
        }
      Status.check(status)
    }
  }

  public actual override fun close() {
    core.close()
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): Int =
    finishProvider(core.finishProviderDecision(decision))

  internal fun finishProviderException(): Int =
    core.finishProviderException()?.let(::finishProvider)
      ?: run {
        // A provider failure also hands the request back to MapLibre, and the binding's release
        // path never runs for it.
        cancelRegistration.dispose()
        UNKNOWN_DECISION
      }

  /**
   * Drops cancel routing for a request that goes back to MapLibre. Native releases that request, so
   * the binding's release path never runs.
   */
  private fun finishProvider(decision: ResourceProviderDecision): Int {
    if (decision == ResourceProviderDecision.PASS_THROUGH) cancelRegistration.dispose()
    return decision.nativeValue
  }

  /**
   * Releases the native request once the wrapper becomes unreachable.
   *
   * Request handles carry no owner-thread affinity, so the cleanup thread may reclaim one. This
   * holds the ownership state alone; holding the wrapper would keep it reachable and suppress every
   * reclaim.
   */
  private class CloseWhenUnreachableAction(private val core: ResourceRequestHandleCore) : Runnable {
    override fun run() {
      core.close()
    }
  }

  private class ReleaseNativeRequest(
    private val handle: NativeResourceRequest,
    private val releaser: (NativeResourceRequest) -> Unit,
    private val cancelRegistration: ResourceRequestCancelRegistration,
  ) : () -> Unit {
    override fun invoke() {
      try {
        releaser(handle)
      } finally {
        // The native release returns once a cancel callback running elsewhere has returned, so
        // the token outlives every native use of it.
        cancelRegistration.dispose()
      }
    }
  }

  private companion object {
    private const val UNKNOWN_DECISION: Int = -1
  }
}
