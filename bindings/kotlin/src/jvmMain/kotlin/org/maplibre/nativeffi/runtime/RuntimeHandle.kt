package org.maplibre.nativeffi.runtime

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.GeneratedOwnerDisposal
import org.maplibre.nativeffi.generated.GeneratedRuntimeOperations
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRuntime

public actual class RuntimeHandle internal constructor(private val handle: NativeRuntime) :
  GeneratedRuntimeOperations() {
  private val core =
    HandleStateCore("RuntimeHandle", handle.raw, dispose = GeneratedOwnerDisposal::runtime)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingRuntimeHandle(): Long {
    core.requireLive()
    return handle.raw
  }

  internal override fun bindingCloseRuntime(call: (Long) -> Int) {
    core.closeOnce({ call(handle.raw) })
  }

  internal override fun bindingRetireRuntime(call: (Long) -> Deferred<Unit>): Deferred<Unit> =
    core.retire({ call(handle.raw) })

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun close(): Deferred<Unit> = release()

  internal fun nativeHandle(): NativeRuntime {
    core.requireLive()
    return handle
  }

  internal fun nativeHandleId(): Long = handle.raw
}
