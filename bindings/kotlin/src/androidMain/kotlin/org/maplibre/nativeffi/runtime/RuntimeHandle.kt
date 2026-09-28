package org.maplibre.nativeffi.runtime

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.GeneratedOwnerDisposal
import org.maplibre.nativeffi.generated.GeneratedRuntimeOperations
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

public actual class RuntimeHandle internal constructor(private val handle: Long) :
  GeneratedRuntimeOperations() {
  private val core =
    HandleStateCore("RuntimeHandle", handle, dispose = GeneratedOwnerDisposal::runtime)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingRuntimeHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseRuntime(call: (Long) -> Int) {
    core.closeOnce({ call(handle) })
  }

  internal override fun bindingRetireRuntime(call: (Long) -> Deferred<Unit>): Deferred<Unit> =
    core.retire({ call(handle) })

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun close(): Deferred<Unit> = release()

  internal fun nativeHandle(): Long {
    core.requireLive()
    return handle
  }

  internal fun nativeHandleId(): Long = handle
}
