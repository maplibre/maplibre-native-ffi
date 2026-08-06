package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.internal.callback.CallbackRing
import org.maplibre.nativeffi.internal.callback.RingTileCallbacks
import org.maplibre.nativeffi.internal.wasm.generated.mln_adapter_custom_geometry_callbacks_retire
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_tile_cancel_callback
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_tile_fetch_callback
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback

/**
 * One custom geometry source's registration of a Kotlin tile callback.
 *
 * The callbacks a source is added with are the browser module's own: they queue the tile id into
 * the ring the runtime drains, so a request raised on MapLibre's tile-loader worker arrives on the
 * thread this binding runs on. What carries the registration across that hop is the `user_data`
 * value, which native returns unread with every tile.
 *
 * A source can be removed, and its map closed, while requests for it are still in the ring. [close]
 * stops delivering at once and then asks the adapter to retire the callbacks, which queues a marker
 * behind every record already in flight; the ring forgets the registration when that marker
 * arrives, so a stale record finds nothing rather than a registration that reused its `user_data`.
 */
internal class CustomGeometryBridge
private constructor(private val callback: CustomGeometrySourceCallback) :
  RingTileCallbacks, AutoCloseable {
  private val gate = CallbackGate(SUBJECT, ::retire)
  private var live = true

  /** The `user_data` to register, which the ring resolves back to this registration. */
  val userData: HeapPointer = CallbackRing.addTileCallbacks(this)

  override fun tile(tileId: CanonicalTileId, cancelled: Boolean) {
    val lease = gate.enter() ?: return
    try {
      if (cancelled) callback.cancelTile(tileId) else callback.fetchTile(tileId)
    } catch (_: Throwable) {
      // A host failure leaves the tile unanswered, which is a state the source already has a
      // meaning for: MapLibre shows nothing there until the host supplies data or invalidates it.
      // There is no native frame above this to report into.
    } finally {
      lease.close()
    }
  }

  /** The ring drops the registration on the marker, and there is nothing else here to release. */
  override fun retired() = Unit

  /**
   * Stops delivering to this callback and waits for a delivery already inside it.
   *
   * Waiting is what a retired callback owes its host: the host may dispose of whatever it gave the
   * callback the moment this returns, and a body that resumed afterwards would use it. A host that
   * removes its own source from inside `fetchTile` is the one caller that cannot wait, because the
   * body it would wait for is the frame below it; [CallbackGate] stops admitting and leaves the
   * retirement to that body as it returns.
   */
  override fun close() {
    if (!live) return
    live = false
    liveCount -= 1
    gate.close()
  }

  /** Queues the marker that ends this registration, once the last delivery has left. */
  private fun retire() {
    mln_adapter_custom_geometry_callbacks_retire(
      mln_kotlin_tile_fetch_callback(),
      mln_kotlin_tile_cancel_callback(),
      userData.address,
    )
  }

  internal companion object {
    private const val SUBJECT = "custom geometry callbacks"

    private var liveCount = 0

    /** Registers [callback] with the ring and returns the registration to hold. */
    fun install(callback: CustomGeometrySourceCallback): CustomGeometryBridge =
      CustomGeometryBridge(callback).also { liveCount += 1 }

    /** The callbacks to register in the descriptor, which the browser module compiles in. */
    fun fetchCallback(): Int = mln_kotlin_tile_fetch_callback()

    fun cancelCallback(): Int = mln_kotlin_tile_cancel_callback()

    /** How many sources still hold a registration, for the tests that assert a teardown. */
    val liveRegistrations: Int
      get() = liveCount
  }
}
