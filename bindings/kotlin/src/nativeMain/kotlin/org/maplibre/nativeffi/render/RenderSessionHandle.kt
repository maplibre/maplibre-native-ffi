package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner
import org.maplibre.nativeffi.generated.GeneratedAcquiredFrameOperations
import org.maplibre.nativeffi.generated.GeneratedOwnerDisposal
import org.maplibre.nativeffi.generated.GeneratedRenderSessionOperations
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.map.MapHandle

@OptIn(ExperimentalNativeApi::class, ExperimentalAtomicApi::class)
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
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }
  private val acquiredFrameScopes = AtomicReference<List<FrameScope>>(emptyList())

  internal override fun bindingRenderSessionHandle(): ULong {
    core.requireLive()
    return handle.raw.toULong()
  }

  internal override fun bindingCloseRenderSession(call: (ULong) -> Int) {
    core.closeOnce({ call(handle.raw.toULong()) }) { invalidateBindingViews() }
  }

  internal override fun invalidateBindingViews() {
    acquiredFrameScopes.load().forEach(FrameScope::close)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun map(): MapHandle = ownerMap

  public actual override fun close() {
    destroy()
  }

  internal fun adoptAcquiredFrame(raw: ULong): AcquiredFrameHandle {
    val scope = FrameScope()
    updateScopes { it + scope }
    return AcquiredFrameHandle(this, raw, scope)
  }

  internal fun frameReleased(scope: FrameScope) {
    updateScopes { it - scope }
  }

  private fun updateScopes(change: (List<FrameScope>) -> List<FrameScope>) {
    while (true) {
      val old = acquiredFrameScopes.load()
      if (acquiredFrameScopes.compareAndSet(old, change(old))) return
    }
  }
}

@OptIn(ExperimentalNativeApi::class, ExperimentalAtomicApi::class)
public actual class AcquiredFrameHandle
internal constructor(
  private val session: RenderSessionHandle,
  private val handle: ULong,
  private val scope: FrameScope,
) : GeneratedAcquiredFrameOperations() {
  private val core =
    HandleStateCore(
      "AcquiredFrameHandle",
      handle.toLong(),
      session,
      dispose = GeneratedOwnerDisposal::acquiredFrame,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingAcquiredFrameHandle(): ULong {
    core.requireLive()
    scope.ensureActive()
    return handle
  }

  internal override fun bindingCloseAcquiredFrame(call: (ULong) -> Int) {
    core.closeOnce({ call(handle) }) {
      scope.close()
      session.frameReleased(scope)
    }
  }

  public actual val isReleased: Boolean
    get() = core.isReleased()
}
