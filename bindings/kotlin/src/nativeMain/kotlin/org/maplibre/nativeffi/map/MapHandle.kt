package org.maplibre.nativeffi.map

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.get
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.runtime.RuntimeHandle

/** Owned any-thread native map handle. */
@OptIn(ExperimentalForeignApi::class)
public actual class MapHandle
internal constructor(private val runtime: RuntimeHandle, handle: NativeMap) :
  GeneratedMapOperations() {
  internal override fun bindingMapHandle(): ULong = state.requireLive().rawHandleValue

  internal override fun bindingCloseMap(call: (ULong) -> Int) {
    state.closeOnce { call(it.rawHandleValue) }
  }

  internal override fun bindingRetireMap(call: (ULong) -> Deferred<Unit>): Deferred<Unit> =
    state.retire({ call(it.rawHandleValue) })

  private val state =
    HandleState(
      "MapHandle",
      handle,
      runtime,
      dispose = org.maplibre.nativeffi.generated.GeneratedOwnerDisposal::map,
    )

  internal fun disposeAbandoned() {
    dispose()
  }

  public actual fun close(): Deferred<Unit> = release()

  public actual val isClosed: Boolean
    get() = state.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  internal fun nativeHandle(): NativeMap = state.requireLive()

  internal fun nativeHandleId(): Long = state.handleId()
}
