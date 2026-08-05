package org.maplibre.nativeffi.internal.wasm

import kotlin.wasm.WasmExport
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback

/**
 * Places this module's tile trampoline in the browser module's function table.
 *
 * `wasmExports` names this module's raw WebAssembly exports, so what reaches `addFunction` is a
 * WebAssembly function rather than a JavaScript closure. Emscripten stores such a function in its
 * table directly, and the signature is the fallback it uses for a function it has to wrap: `v` for
 * a callback that answers nothing, and `i` for each of the five 32-bit values.
 *
 * The entry belongs to the agent that added it, which here is the page. That is exactly the
 * property `src/browser/custom_geometry.c` posts for: a MapLibre worker reaches this entry by
 * asking the main runtime thread to make the call, rather than by calling it itself.
 */
@JsFun(
  "() => globalThis.__maplibreNativeC.addFunction(" +
    "wasmExports['mln_browser_custom_geometry_tile_host'], 'viiiii')"
)
private external fun addTileTrampoline(): Int

@JsFun("(host) => globalThis.__maplibreNativeC._mln_browser_custom_geometry_install(host)")
private external fun installTileHost(host: Int): Boolean

@JsFun("() => globalThis.__maplibreNativeC._mln_browser_custom_geometry_fetch_thunk()")
private external fun fetchTileThunk(): Int

@JsFun("() => globalThis.__maplibreNativeC._mln_browser_custom_geometry_cancel_thunk()")
private external fun cancelTileThunk(): Int

/**
 * Which callback a notification carries, as `src/browser/custom_geometry.c` numbers them.
 *
 * The module posts one notification shape for both callbacks so that one table entry serves a
 * source's whole registration, and this is the field that says which one arrived. It is a contract
 * with that file rather than a C API enum, so it is named here rather than generated.
 */
private const val KIND_FETCH = 0

/**
 * The trampoline a custom geometry source's tile callbacks reach, by way of the module's proxy.
 *
 * A raw WebAssembly export rather than a `@JsExport`, so that the browser module's function table
 * holds this module's function itself. The call then travels from the worker to the page and into
 * Kotlin without passing through JavaScript at all.
 *
 * The tile id arrives as its three components rather than as a pointer, because the notification
 * that carried it across is freed as this returns. The x and y components are unsigned in C and
 * signed here, so they are widened through their bit pattern rather than through their value; the
 * public type carries the whole unsigned domain in a `Long`.
 *
 * Nothing may unwind out of a WebAssembly export: the C frame that called it is the module's own
 * proxied task, which has no handler, and the trap would take the module down with it.
 */
@WasmExport("mln_browser_custom_geometry_tile_host")
internal fun mlnBrowserCustomGeometryTileHost(userData: Int, kind: Int, z: Int, x: Int, y: Int) {
  try {
    CustomGeometryBridge.dispatch(userData, kind, z, x.toUInt().toLong(), y.toUInt().toLong())
  } catch (_: Throwable) {
    // Contained here because there is no Kotlin frame above this one to unwind into, and because a
    // tile callback has no answer a failure could be reported through.
  }
}

/**
 * Names the trampoline from Kotlin, so the linker keeps its export.
 *
 * Calling it rather than merely mentioning it, because a mention of a function that is never
 * invoked is itself removable. Token 0 is never issued — [HostCallbackTable] counts from one — so
 * this finds no registration and returns without reading anything. That is what makes calling it
 * here safe as well as sufficient.
 */
private fun retainTrampolines() {
  mlnBrowserCustomGeometryTileHost(0, 0, 0, 0, 0)
}

/**
 * One custom geometry source's registration of a Kotlin tile callback.
 *
 * MapLibre asks for a tile from the worker its source's tile loader runs on, and the module posts
 * that request to the page rather than waiting for it to be answered there; see
 * `src/browser/custom_geometry.c` for why an asynchronous proxy is the right one for a callback
 * that returns void. What arrives here is therefore a notification on a page task, after the worker
 * that produced it has moved on.
 *
 * The body does **not** run inside a [CallbackScope], and this is the one callback family in the
 * binding that does not. A callback scope exists to refuse a suspension on a stack that cannot take
 * one, and the stack a notification is delivered on is chosen here rather than inherited:
 * [AsyncDelivery] queues it and runs it on a promising stack, so the obvious thing for a host to
 * write — a `fetchTile` that answers immediately with `setCustomGeometrySourceTileData` — is an
 * ordinary owner-affine call and works exactly as it does on JVM, Android, and Kotlin/Native. That
 * is only available here because nothing is waiting: the worker returned as soon as it posted, so
 * parking the page adds no edge to the wait graph the synchronous callbacks live inside.
 *
 * A [CallbackGate] is what makes a late notification safe. The source can be removed, and the map
 * closed, while notifications for it are still in flight on the page; the gate stops delivering as
 * soon as the registration is closed, and the token it was reached by is never issued again, so a
 * notification that arrives afterwards finds nothing and is dropped. The token is resolved at
 * delivery rather than at posting for exactly that reason, which is also what
 * `src/browser/custom_geometry.c` does with the host pointer, and it is what keeps the guarantee
 * across the extra hop the queue adds.
 */
internal class CustomGeometryBridge
private constructor(private val callback: CustomGeometrySourceCallback, private val token: Int) :
  AutoCloseable {
  private val gate = CallbackGate(SUBJECT)

  /** The `user_data` to register, which native carries back to [dispatch] with every tile. */
  val userData: HeapPointer
    get() = HeapPointer(token)

  /**
   * Stops delivering to this callback and releases the trampoline when it was the last one.
   *
   * Called once the source it belongs to is gone from the style, or once the map that held it has
   * been destroyed. Neither of those waits for an in-flight notification, and neither has to: a
   * notification is a copy of a tile id on a page task, so the worst a late one can do is arrive
   * after this, which is what the gate and the retired token answer.
   *
   * A host may reach this from inside its own callback, because a delivery can call the owner
   * thread and removing a source is such a call. That needs no refusal the way the synchronous
   * callbacks' does: the gate stops admitting as this runs, the delivery already inside it finishes
   * on its own, and the gate holds nothing native that a still-running body could be left without.
   * A source that ends itself from `fetchTile` therefore hears nothing further, which is what a
   * host asked for.
   *
   * Closed without draining, and it is the only gate in the binding that is. A delivery reaching
   * the owner thread parks its stack, and the host's stack -- parked on a call it made earlier --
   * is resumed first, so the ordinary case is a close arriving from a stack that is not the
   * delivery's while the delivery is suspended partway through a host body. Draining there is not
   * slow but impossible: the suspended frame resumes from the event loop, which a close spinning on
   * a count is never going to reach. [CallbackGate.closeWithoutDraining] says the rest.
   */
  override fun close() {
    gate.closeWithoutDraining()
    host.remove(token)
  }

  private fun invoke(kind: Int, tileId: CanonicalTileId) {
    val lease = gate.enter() ?: return
    try {
      if (kind == KIND_FETCH) callback.fetchTile(tileId) else callback.cancelTile(tileId)
    } catch (_: Throwable) {
      // A host failure leaves the tile unanswered, which is a state the source already has a
      // meaning for: MapLibre shows nothing for that tile until the host supplies data or
      // invalidates it. There is no native frame above this to report into.
    } finally {
      lease.close()
    }
  }

  internal companion object {
    private const val SUBJECT = "custom geometry callbacks"

    private val host =
      HostCallbackTable<CustomGeometryBridge>(
        SUBJECT,
        ::addTileTrampoline,
        ::installTileHost,
        ::retainTrampolines,
      )

    /** Installs the host trampoline and registers [callback] with it. */
    fun install(callback: CustomGeometrySourceCallback): CustomGeometryBridge = host.add { token ->
      CustomGeometryBridge(callback, token)
    }

    /** How many sources still hold a registration, for the tests that assert a teardown. */
    val liveRegistrations: Int
      get() = host.registrationCount

    /** The callbacks to register in the descriptor, which are compiled into the browser module. */
    fun fetchThunk(): Int = fetchTileThunk()

    fun cancelThunk(): Int = cancelTileThunk()

    /**
     * Queues a tile notification for the registration [token] names.
     *
     * The tile id is built here, on the frame the module's proxied task called, because the
     * notification carrying it is freed as that task returns. The registration is *not* looked up
     * here: it is resolved when the queued body runs, so a source removed between the posting and
     * the delivery is one whose notification is dropped rather than delivered to a callback that
     * has retired. A token with no registration names such a source, or a map that has been closed.
     *
     * Queued rather than delivered, so that the body runs on a stack that may reach the owner
     * thread; [AsyncDelivery] says why that is available here and nowhere else in this binding.
     */
    fun dispatch(token: Int, kind: Int, z: Int, x: Long, y: Long) {
      // Rejected here as well, because a token is issued once and never again: one that names
      // nothing now will never name anything, so queuing it would cost a drain and deliver nothing.
      // The lookup in the queued body is what carries the guarantee; this only saves the work.
      if (host.find(token) == null) return
      val tileId = CanonicalTileId(z, x, y)
      AsyncDelivery.post { host.find(token)?.invoke(kind, tileId) }
    }
  }
}
