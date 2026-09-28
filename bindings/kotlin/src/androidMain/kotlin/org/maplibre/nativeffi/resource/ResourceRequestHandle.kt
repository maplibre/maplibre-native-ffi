package org.maplibre.nativeffi.resource

import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.lifecycle.UnreachableActions
import org.maplibre.nativeffi.internal.status.Status

public actual class ResourceRequestHandle
internal constructor(
  private val handleId: Long,
  releaser: (Long) -> Unit = {
    org.maplibre.nativeffi.generated.GeneratedOwnerDisposal.resourceRequestHandle(it.toLong())
  },
) : org.maplibre.nativeffi.generated.GeneratedResourceRequestHandleOperations(), AutoCloseable {
  internal override fun bindingResourceRequestHandleHandle(): Long = core.withLiveHandle {
    handleId
  }

  internal override fun bindingIssuedResourceRequestHandleHandle(): Long = handleId

  internal override fun bindingCloseResourceRequestHandle(call: (Long) -> Int) {
    close()
  }

  internal override fun bindingCompleteResourceRequestHandle(call: (Long) -> Int) {
    val operation = core.beginComplete()
    try {
      val status = call(handleId)
      if (status == 0) operation.markCompleted() else operation.markNotReachedNative()
      Status.check(status)
    } catch (error: Throwable) {
      operation.markNotReachedNative()
      throw error
    } finally {
      operation.close()
    }
  }

  internal override fun <T> bindingReadResourceRequestHandle(block: (Long) -> T): T =
    core.withLiveHandle {
      block(handleId)
    }

  internal override fun bindingRegisterResourceRequestHandleCancel(
    callback: () -> Unit,
    call: (Long, Long) -> ResourceRequestCancelSetResult,
  ): Boolean = core.withLiveHandle {
    val cancelled = cancelState.register(callback) { token -> call(handleId, token) } != null
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
    ResourceRequestHandleCore(ReleaseNativeRequest(handleId, releaser, cancelRegistration))

  init {
    val state = core
    UnreachableActions.register(this, Runnable { state.close() })
  }

  public actual override fun close() {
    CallbackAdmission.check(handleId.toLong(), "mln_resource_request_release")
    cancelState.drop()
    core.close()
    cancelState.drop()
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): Int =
    finishProvider(core.finishProviderDecision(decision))

  internal fun finishProviderException(): Int =
    core.finishProviderException()?.let(::finishProvider) ?: handedBackToNative(UNKNOWN_DECISION)

  private fun finishProvider(decision: ResourceProviderDecision): Int =
    if (decision == ResourceProviderDecision.PASS_THROUGH) {
      handedBackToNative(decision.rawValue.toInt())
    } else {
      decision.rawValue.toInt()
    }

  private fun handedBackToNative(result: Int): Int {
    cancelState.drop()
    cancelRegistration.dispose()
    return result
  }

  private class ReleaseNativeRequest(
    private val raw: Long,
    private val releaser: (Long) -> Unit,
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

  private companion object {
    const val UNKNOWN_DECISION = -1
  }
}
