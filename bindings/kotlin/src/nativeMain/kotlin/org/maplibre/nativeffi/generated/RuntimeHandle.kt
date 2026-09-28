// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
public actual class RuntimeHandle
internal constructor(
  private val handle: ULong,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::runtime,
) : GeneratedRuntimeOperations(), org.maplibre.nativeffi.runtime.AsyncReleasable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "RuntimeHandle",
      handle.toLong(),
      dispose = dispose,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingRuntimeHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseRuntime(call: (ULong) -> Int) {
    core.closeOnce({ call(handle) })
  }

  internal override fun bindingRetireRuntime(
    call: (ULong) -> kotlinx.coroutines.Deferred<Unit>
  ): kotlinx.coroutines.Deferred<Unit> = core.retire({ call(handle) })

  public actual val isClosed: Boolean
    get() = core.isReleased()
}
