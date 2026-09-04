package org.maplibre.nativeffi.resource

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_resource_request_cancelled
import org.maplibre.nativeffi.internal.c.mln_resource_request_complete
import org.maplibre.nativeffi.internal.c.mln_resource_request_release
import org.maplibre.nativeffi.internal.c.mln_resource_request_set_cancel_callback
import org.maplibre.nativeffi.internal.c.mln_resource_response
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelBridge
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelRegistration
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelSetResult
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelState
import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ResourceStructs

@OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)
public actual class ResourceRequestHandle
internal constructor(
  private val handle: NativeResourceRequest,
  private val completer: (ULong, CPointer<mln_resource_response>) -> Int =
    ::mln_resource_request_complete,
  private val cancellationChecker: (ULong, CPointer<BooleanVar>) -> Int =
    { requestHandle, outCancelled ->
      mln_resource_request_cancelled(requestHandle, outCancelled)
    },
  private val cancelCallbackSetter: (ULong, Long) -> ResourceRequestCancelSetResult =
    { requestHandle, token ->
      memScoped {
        val outCancelled = alloc<BooleanVar>()
        outCancelled.value = false
        val status =
          mln_resource_request_set_cancel_callback(
            requestHandle,
            ResourceRequestCancelBridge.stub,
            ResourceRequestCancelBridge.userData(token),
            outCancelled.ptr,
          )
        ResourceRequestCancelSetResult(status, outCancelled.value)
      }
    },
  private val releaser: (ULong) -> Unit = ::mln_resource_request_release,
) : AutoCloseable {
  private val cancelRegistration = ResourceRequestCancelRegistration()
  private val cancelState = ResourceRequestCancelState(cancelRegistration)
  // Native release returns once a cancel callback running on another thread has returned, so no
  // native use of the token outlives it. This closure holds the token alone; the callback state
  // would reach the host callback and whatever it captures.
  private val core =
    ResourceRequestHandleCore(
      ReleaseNativeRequest(handle.rawHandleValue, releaser, cancelRegistration)
    )
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(core) { it.close() }

  public actual fun complete(response: ResourceResponse) {
    val operation = core.beginComplete()
    var reachedNative = false
    try {
      val nativeStatus = memScoped {
        val nativeResponse = ResourceStructs.resourceResponse(response, this)
        reachedNative = true
        completer(handle.rawHandleValue, nativeResponse)
      }
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

  public actual fun isCancelled(): Boolean = core.withLiveHandle {
    memScoped {
      val outCancelled = alloc<BooleanVar>()
      outCancelled.value = false
      Status.check(cancellationChecker(handle.rawHandleValue, outCancelled.ptr))
      outCancelled.value
    }
  }

  public actual fun setCancelCallback(callback: () -> Unit) {
    val alreadyCancelled = core.withLiveHandle {
      cancelState.register(callback) { token -> cancelCallbackSetter(handle.rawHandleValue, token) }
    }
    // The borrow has ended, so the callback may close this handle and release it immediately.
    alreadyCancelled?.let(ResourceRequestCancelState::runContained)
  }

  public actual override fun close() {
    cancelState.drop()
    core.close()
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): UInt =
    finishProvider(core.finishProviderDecision(decision))

  internal fun finishProviderException(): UInt =
    core.finishProviderException()?.let(::finishProvider) ?: handedBackToNative(UInt.MAX_VALUE)

  private fun finishProvider(decision: ResourceProviderDecision): UInt =
    if (decision == ResourceProviderDecision.PASS_THROUGH) {
      handedBackToNative(decision.nativeValue.toUInt())
    } else {
      decision.nativeValue.toUInt()
    }

  /** MapLibre retires a request the provider did not handle, so the release path never runs. */
  private fun handedBackToNative(result: UInt): UInt {
    cancelState.drop()
    cancelRegistration.dispose()
    return result
  }

  private class ReleaseNativeRequest(
    private val rawHandle: ULong,
    private val releaser: (ULong) -> Unit,
    private val cancelRegistration: ResourceRequestCancelRegistration,
  ) : () -> Unit {
    override fun invoke() {
      try {
        releaser(rawHandle)
      } finally {
        cancelRegistration.dispose()
      }
    }
  }
}
