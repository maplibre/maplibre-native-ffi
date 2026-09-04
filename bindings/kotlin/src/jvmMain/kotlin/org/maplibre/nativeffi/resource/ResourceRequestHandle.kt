package org.maplibre.nativeffi.resource

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelBridge
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelRegistration
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelSetResult
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
  private val cancelCallbackSetter:
    (NativeResourceRequest, Long) -> ResourceRequestCancelSetResult =
    { requestHandle, token ->
      NativeAccess.setResourceRequestCancelCallback(
        requestHandle,
        ResourceRequestCancelBridge.stub,
        ResourceRequestCancelBridge.userData(token),
      )
    },
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
      if (reachedNative) cancelState.drop()
      operation.close()
    }
  }

  public actual fun isCancelled(): Boolean {
    NativeAccess.ensureLoaded()
    return core.withLiveHandle { cancellationChecker(handle) }
  }

  public actual fun setCancelCallback(callback: () -> Unit) {
    NativeAccess.ensureLoaded()
    val alreadyCancelled = core.withLiveHandle {
      cancelState.register(callback) { token -> cancelCallbackSetter(handle, token) }
    }
    // The borrow has ended, so the callback may close this handle and release it immediately.
    alreadyCancelled?.let(ResourceRequestCancelState::runContained)
  }

  public actual override fun close() {
    cancelState.drop()
    core.close()
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): Int =
    finishProvider(core.finishProviderDecision(decision))

  internal fun finishProviderException(): Int =
    core.finishProviderException()?.let(::finishProvider) ?: handedBackToNative(UNKNOWN_DECISION)

  private fun finishProvider(decision: ResourceProviderDecision): Int =
    if (decision == ResourceProviderDecision.PASS_THROUGH) {
      handedBackToNative(decision.nativeValue)
    } else {
      decision.nativeValue
    }

  /** MapLibre retires a request the provider did not handle, so the release path never runs. */
  private fun handedBackToNative(result: Int): Int {
    cancelState.drop()
    cancelRegistration.dispose()
    return result
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

  /**
   * Releases the native request and then drops its registry token.
   *
   * Native release returns once a cancel callback running on another thread has returned, so no
   * native use of the token outlives this call. This holds the token alone; the callback state
   * would reach the host callback and whatever it captures.
   */
  private class ReleaseNativeRequest(
    private val handle: NativeResourceRequest,
    private val releaser: (NativeResourceRequest) -> Unit,
    private val cancelRegistration: ResourceRequestCancelRegistration,
  ) : () -> Unit {
    override fun invoke() {
      try {
        releaser(handle)
      } finally {
        cancelRegistration.dispose()
      }
    }
  }

  private companion object {
    private const val UNKNOWN_DECISION: Int = -1
  }
}
