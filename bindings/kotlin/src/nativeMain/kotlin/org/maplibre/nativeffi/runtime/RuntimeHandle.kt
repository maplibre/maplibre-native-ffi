package org.maplibre.nativeffi.runtime

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.GeneratedOwnerDisposal
import org.maplibre.nativeffi.generated.GeneratedRuntimeOperations
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRuntime

@OptIn(ExperimentalNativeApi::class)
public actual class RuntimeHandle internal constructor(private val handle: NativeRuntime) :
  GeneratedRuntimeOperations() {
  private val core =
    HandleStateCore("RuntimeHandle", handle.raw, dispose = GeneratedOwnerDisposal::runtime)
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingRuntimeHandle(): ULong {
    core.requireLive()
    return handle.raw.toULong()
  }

  internal override fun bindingCloseRuntime(call: (ULong) -> Int) {
    core.closeOnce({ call(handle.raw.toULong()) })
  }

  internal override fun bindingRetireRuntime(call: (ULong) -> Deferred<Unit>): Deferred<Unit> =
    core.retire({ call(handle.raw.toULong()) })

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun close(): Deferred<Unit> = release()

  internal fun nativeHandle(): NativeRuntime {
    core.requireLive()
    return handle
  }

  internal fun nativeHandleId(): Long = handle.raw
}
