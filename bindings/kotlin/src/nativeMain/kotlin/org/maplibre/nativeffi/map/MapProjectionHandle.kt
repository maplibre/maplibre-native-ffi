package org.maplibre.nativeffi.map

import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.NativeMapProjection
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue

/**
 * Owned standalone projection snapshot created from a map.
 *
 * See the common declaration for the projection's threading and lifetime rules.
 */
@OptIn(ExperimentalForeignApi::class)
public actual class MapProjectionHandle internal constructor(handle: NativeMapProjection) :
  org.maplibre.nativeffi.generated.GeneratedMapProjectionOperations(), AutoCloseable {
  internal override fun bindingMapProjectionHandle(): ULong = state.requireLive().rawHandleValue

  private val state =
    HandleState(
      "MapProjectionHandle",
      handle,
      dispose = org.maplibre.nativeffi.generated.GeneratedOwnerDisposal::mapProjection,
    )

  internal override fun bindingCloseMapProjection(call: (ULong) -> Int) {
    state.closeOnce { call(it.rawHandleValue) }
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()
}
