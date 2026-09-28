// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
public actual class ResourceRequestHandle
internal constructor(
  private val handle: ULong,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::resourceRequestHandle,
) : GeneratedResourceRequestHandleOperations(), AutoCloseable {
  private val state =
    org.maplibre.nativeffi.internal.lifecycle.DecisionOwnerState(
      "ResourceRequestHandle",
      handle.toLong(),
      accept = 1u,
      passThrough = 0u,
      dispose = dispose,
    )
  @Suppress("unused") private val cleaner = createCleaner(state.core) { it.close() }

  internal override fun bindingResourceRequestHandleHandle(): ULong = state.withLive { handle }

  internal override fun <T> bindingReadResourceRequestHandle(block: (ULong) -> T): T =
    state.withLive {
      block(handle)
    }

  internal override fun bindingCompleteResourceRequestHandle(call: (ULong) -> Int) {
    state.complete { call(handle) }
  }

  internal override fun bindingRegisterResourceRequestHandleCancel(
    callback: () -> Unit,
    call: (ULong, Long) -> org.maplibre.nativeffi.internal.callback.DecisionCancelSetResult,
  ): Boolean = state.registerCancel(callback) { token -> call(handle, token) }

  internal override fun bindingCloseResourceRequestHandle(call: (ULong) -> Int) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle.toLong(),
      "mln_resource_request_release",
    )
    state.close()
  }

  internal fun finishBindingDecision(raw: UInt): UInt = state.finishDecision(raw)

  internal fun finishBindingException(): UInt = state.finishException()

  public actual val isClosed: Boolean
    get() = state.isClosed

  internal override fun bindingIssuedResourceRequestHandleHandle(): ULong = handle

  public actual override fun close() {
    resourceRequestRelease()
  }
}
