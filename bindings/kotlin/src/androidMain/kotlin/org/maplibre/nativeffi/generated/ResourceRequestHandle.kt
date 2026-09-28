// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class ResourceRequestHandle
internal constructor(
  private val handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::resourceRequestHandle,
) : GeneratedResourceRequestHandleOperations(), AutoCloseable {
  private val state =
    org.maplibre.nativeffi.internal.lifecycle.DecisionOwnerState(
      "ResourceRequestHandle",
      handle,
      accept = 1u,
      passThrough = 0u,
      dispose = dispose,
    )

  init {
    val core = state.core
    org.maplibre.nativeffi.internal.lifecycle.UnreachableActions.register(
      this,
      Runnable { core.close() },
    )
  }

  internal override fun bindingResourceRequestHandleHandle(): Long = state.withLive { handle }

  internal override fun <T> bindingReadResourceRequestHandle(block: (Long) -> T): T =
    state.withLive {
      block(handle)
    }

  internal override fun bindingCompleteResourceRequestHandle(call: (Long) -> Int) {
    state.complete { call(handle) }
  }

  internal override fun bindingRegisterResourceRequestHandleCancel(
    callback: () -> Unit,
    call: (Long, Long) -> org.maplibre.nativeffi.internal.callback.DecisionCancelSetResult,
  ): Boolean = state.registerCancel(callback) { token -> call(handle, token) }

  internal override fun bindingCloseResourceRequestHandle(call: (Long) -> Int) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_resource_request_release",
    )
    state.close()
  }

  internal fun finishBindingDecision(raw: UInt): UInt = state.finishDecision(raw)

  internal fun finishBindingException(): UInt = state.finishException()

  public actual val isClosed: Boolean
    get() = state.isClosed

  internal override fun bindingIssuedResourceRequestHandleHandle(): Long = handle

  public actual override fun close() {
    resourceRequestRelease()
  }
}
