package org.maplibre.nativeffi.map

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.runtime.RuntimeHandle

/** Owned Android JNI map handle. */
public actual class MapHandle
internal constructor(private val runtime: RuntimeHandle, private val handleId: Long) :
  GeneratedMapOperations() {
  internal override fun bindingMapHandle(): Long = requireLiveHandle()

  internal override fun bindingCloseMap(call: (Long) -> Int) {
    core.closeOnce(destroy = { call(handleId) })
  }

  internal override fun bindingRetireMap(call: (Long) -> Deferred<Unit>): Deferred<Unit> =
    core.retire({ call(handleId) })

  private val core =
    HandleStateCore(
      "MapHandle",
      handleId,
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

  internal fun nativeHandleId(): Long = handleId

  private fun requireLiveHandle(): Long {
    core.requireLive()
    return handleId
  }
}
