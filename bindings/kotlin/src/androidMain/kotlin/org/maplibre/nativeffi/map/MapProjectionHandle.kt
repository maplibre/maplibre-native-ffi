package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

/**
 * Owned Android JNI standalone projection snapshot.
 *
 * See the common declaration for the projection's threading and lifetime rules.
 */
public actual class MapProjectionHandle internal constructor(private val handleId: Long) :
  org.maplibre.nativeffi.generated.GeneratedMapProjectionOperations(), AutoCloseable {
  internal override fun bindingMapProjectionHandle(): Long = run {
    core.requireLive()
    handleId
  }

  private val core =
    HandleStateCore(
      "MapProjectionHandle",
      handleId,
      dispose = org.maplibre.nativeffi.generated.GeneratedOwnerDisposal::mapProjection,
    )

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  internal override fun bindingCloseMapProjection(call: (Long) -> Int) {
    core.closeOnce(destroy = { call(handleId) })
  }
}
