package org.maplibre.nativeffi.render

import java.util.concurrent.ConcurrentHashMap
import org.maplibre.nativeffi.generated.GeneratedAcquiredFrameOperations
import org.maplibre.nativeffi.generated.GeneratedOwnerDisposal
import org.maplibre.nativeffi.generated.GeneratedRenderSessionOperations
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.map.MapHandle

public actual class RenderSessionHandle
internal constructor(private val ownerMap: MapHandle, private val handle: NativeRenderSession) :
  GeneratedRenderSessionOperations(), AutoCloseable {
  private val core =
    HandleStateCore(
      "RenderSessionHandle",
      handle.raw,
      ownerMap,
      dispose = GeneratedOwnerDisposal::renderSession,
    )

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private val acquiredFrameScopes = ConcurrentHashMap.newKeySet<FrameScope>()

  internal override fun bindingRenderSessionHandle(): Long {
    core.requireLive()
    return handle.raw
  }

  internal override fun bindingCloseRenderSession(call: (Long) -> Int) {
    core.closeOnce({ call(handle.raw) }) { invalidateBindingViews() }
  }

  internal override fun invalidateBindingViews() {
    acquiredFrameScopes.forEach(FrameScope::close)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun map(): MapHandle = ownerMap

  public actual override fun close() {
    destroy()
  }

  internal fun adoptAcquiredFrame(raw: Long): AcquiredFrameHandle {
    val scope = FrameScope()
    acquiredFrameScopes.add(scope)
    return AcquiredFrameHandle(this, raw, scope)
  }

  internal fun frameReleased(scope: FrameScope) {
    acquiredFrameScopes.remove(scope)
  }
}

public actual class AcquiredFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val handle: Long,
  private val scope: FrameScope,
) : GeneratedAcquiredFrameOperations() {
  private val core =
    HandleStateCore(
      "AcquiredFrameHandle",
      handle.toLong(),
      session,
      dispose = GeneratedOwnerDisposal::acquiredFrame,
    )

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingAcquiredFrameHandle(): Long {
    core.requireLive()
    scope.ensureActive()
    return handle
  }

  internal override fun bindingCloseAcquiredFrame(call: (Long) -> Int) {
    core.closeOnce({ call(handle) }) {
      scope.close()
      session.frameReleased(scope)
    }
  }

  public actual val isReleased: Boolean
    get() = core.isReleased()
}
