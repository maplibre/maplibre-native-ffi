package org.maplibre.nativeffi.map

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.runtime.RuntimeHandle

/** Owned JVM FFM map handle. */
public actual class MapHandle
internal constructor(private val runtime: RuntimeHandle, private val handle: NativeMap) :
  GeneratedMapOperations() {
  internal override fun bindingMapHandle(): Long = requireLiveHandle().raw

  internal override fun bindingCloseMap(call: (Long) -> Int) {
    core.closeOnce(destroy = { call(handle.raw) })
  }

  internal override fun bindingRetireMap(call: (Long) -> Deferred<Unit>): Deferred<Unit> =
    core.retire({ call(handle.raw) })

  private val core =
    HandleStateCore(
      "MapHandle",
      handle.raw,
      dispose = org.maplibre.nativeffi.generated.GeneratedOwnerDisposal::map,
    )

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  internal fun disposeAbandoned() {
    dispose()
  }

  public actual fun close(): Deferred<Unit> = release()

  internal fun nativeHandleId(): Long = handle.raw

  internal fun nativeHandle(): NativeMap = requireLiveHandle()

  private fun requireLiveHandle(): NativeMap {
    core.requireLive()
    return handle
  }
}
