// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class RuntimeHandle
internal constructor(
  private val handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::runtime,
) : GeneratedRuntimeOperations(), org.maplibre.nativeffi.runtime.AsyncReleasable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "RuntimeHandle",
      handle,
      dispose = dispose,
    )

  init {
    org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingRuntimeHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseRuntime(call: (Long) -> Int) {
    core.closeOnce({ call(handle) })
  }

  internal override fun bindingRetireRuntime(
    call: (Long) -> kotlinx.coroutines.Deferred<Unit>
  ): kotlinx.coroutines.Deferred<Unit> = core.retire({ call(handle) })

  public actual val isClosed: Boolean
    get() = core.isReleased()
}
