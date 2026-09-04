package org.maplibre.nativeffi.resource

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_resource_request_cancel_callback
import org.maplibre.nativeffi.internal.c.mln_resource_request_cancelled
import org.maplibre.nativeffi.internal.c.mln_resource_request_complete
import org.maplibre.nativeffi.internal.c.mln_resource_request_release
import org.maplibre.nativeffi.internal.c.mln_resource_request_set_cancel_callback
import org.maplibre.nativeffi.internal.c.mln_resource_response
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelBridge
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelRegistration
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
  private val cancelCallbackSetter:
    (ULong, mln_resource_request_cancel_callback?, COpaquePointer?) -> Int =
    { requestHandle, callback, userData ->
      mln_resource_request_set_cancel_callback(requestHandle, callback, userData)
    },
  private val releaser: (ULong) -> Unit = ::mln_resource_request_release,
) : AutoCloseable {
  private val cancelRegistration = ResourceRequestCancelRegistration()
  private val cancelState = ResourceRequestCancelState(cancelRegistration)
  private val core = ResourceRequestHandleCore {
    try {
      releaser(handle.rawHandleValue)
    } finally {
      // The native release returns once a cancel callback running elsewhere has returned, so
      // the token outlives every native use of it.
      cancelRegistration.dispose()
    }
  }
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

  public actual fun setCancelCallback(callback: (() -> Unit)?) {
    core.withLiveHandle {
      cancelState.store(callback)
      val status =
        if (callback == null) {
          cancelCallbackSetter(handle.rawHandleValue, null, null)
        } else {
          cancelCallbackSetter(
            handle.rawHandleValue,
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

  internal fun finishProviderDecision(decision: ResourceProviderDecision): UInt =
    finishProvider(core.finishProviderDecision(decision))

  internal fun finishProviderException(): UInt =
    core.finishProviderException()?.let(::finishProvider)
      ?: run {
        // A provider failure also hands the request back to MapLibre, and the binding's release
        // path never runs for it.
        cancelRegistration.dispose()
        UInt.MAX_VALUE
      }

  private fun finishProvider(decision: ResourceProviderDecision): UInt {
    if (decision == ResourceProviderDecision.PASS_THROUGH) cancelRegistration.dispose()
    return decision.nativeValue.toUInt()
  }
}
