package org.maplibre.nativeffi.resource

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.status.Status

@OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)
public actual class ResourceRequestHandle
internal constructor(
  private val handle: NativeResourceRequest,
  releaser: (ULong) -> Unit = {
    org.maplibre.nativeffi.generated.GeneratedOwnerDisposal.resourceRequestHandle(it.toLong())
  },
) : org.maplibre.nativeffi.generated.GeneratedResourceRequestHandleOperations(), AutoCloseable {
  internal override fun bindingResourceRequestHandleHandle(): ULong = core.withLiveHandle {
    handle.rawHandleValue
  }

  internal override fun bindingIssuedResourceRequestHandleHandle(): ULong = handle.rawHandleValue

  internal override fun bindingCloseResourceRequestHandle(call: (ULong) -> Int) {
    close()
  }

  internal override fun bindingCompleteResourceRequestHandle(call: (ULong) -> Int) {
    val operation = core.beginComplete()
    try {
      val status = call(handle.rawHandleValue)
      if (status == 0) operation.markCompleted() else operation.markNotReachedNative()
      Status.check(status)
    } catch (error: Throwable) {
      operation.markNotReachedNative()
      throw error
    } finally {
      operation.close()
    }
  }

  internal override fun <T> bindingReadResourceRequestHandle(block: (ULong) -> T): T =
    core.withLiveHandle {
      block(handle.rawHandleValue)
    }

  internal override fun bindingRegisterResourceRequestHandleCancel(
    callback: () -> Unit,
    call: (ULong, Long) -> ResourceRequestCancelSetResult,
  ): Boolean = core.withLiveHandle {
    val cancelled =
      cancelState.register(callback) { token -> call(handle.rawHandleValue, token) } != null
    if (core.isClosed) cancelState.drop()
    cancelled
  }

  internal fun finishBindingDecision(raw: UInt): UInt =
    when (raw) {
      ResourceProviderDecision.PASS_THROUGH.rawValue ->
        finishProviderDecision(ResourceProviderDecision.PASS_THROUGH).toUInt()
      ResourceProviderDecision.HANDLE.rawValue ->
        finishProviderDecision(ResourceProviderDecision.HANDLE).toUInt()
      else -> finishProviderException().toUInt()
    }

  internal fun finishBindingException(): UInt = finishProviderException().toUInt()

  private val cancelRegistration = ResourceRequestCancelRegistration()
  private val cancelState = ResourceRequestCancelState(cancelRegistration)
  private val core =
    ResourceRequestHandleCore(
      ReleaseNativeRequest(handle.rawHandleValue, releaser, cancelRegistration)
    )
  @Suppress("unused") private val cleaner = createCleaner(core) { it.close() }

  public actual override fun close() {
    CallbackAdmission.check(handle.rawHandleValue.toLong(), "mln_resource_request_release")
    cancelState.drop()
    core.close()
    cancelState.drop()
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): UInt =
    finishProvider(core.finishProviderDecision(decision))

  internal fun finishProviderException(): UInt =
    core.finishProviderException()?.let(::finishProvider) ?: handedBackToNative(UInt.MAX_VALUE)

  private fun finishProvider(decision: ResourceProviderDecision): UInt =
    if (decision == ResourceProviderDecision.PASS_THROUGH) {
      handedBackToNative(decision.rawValue)
    } else {
      decision.rawValue
    }

  private fun handedBackToNative(result: UInt): UInt {
    cancelState.drop()
    cancelRegistration.dispose()
    return result
  }

  private class ReleaseNativeRequest(
    private val raw: ULong,
    private val releaser: (ULong) -> Unit,
    private val registration: ResourceRequestCancelRegistration,
  ) : () -> Unit {
    override fun invoke() {
      try {
        releaser(raw)
      } finally {
        registration.dispose()
      }
    }
  }
}
