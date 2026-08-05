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
 * The body runs inside a [CallbackScope], which makes a dispatched call from it report an error
 * rather than park this stack. That matters more here than the deadlock it prevents elsewhere: the
 * obvious thing for a host to write is a `fetchTile` that answers immediately with
 * `setCustomGeometrySourceTileData`, and that call is owner-affine, so it is refused. A host
 * answers from its own stack instead — the request and the answer are separate in the C API on
 * every platform, and this target is the one where that separation is enforced.
 *
 * A [CallbackGate] is what makes a late notification safe. The source can be removed, and the map
 * closed, while notifications for it are still in flight on the page; the gate stops delivering as
 * soon as the registration is closed, and the token it was reached by is never issued again, so a
 * notification that arrives afterwards finds nothing and is dropped.
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
   * There is no reentrancy check to make, unlike the synchronous callbacks: everything that ends a
   * source — removing it, replacing the style, closing the map — reaches the owner thread, and a
   * dispatched call from inside a callback is already refused.
   */
  override fun close() {
    gate.close()
    host.remove(token)
  }

  private fun invoke(kind: Int, tileId: CanonicalTileId) {
    val lease = gate.enter() ?: return
    try {
      CallbackScope.inside {
        if (kind == KIND_FETCH) callback.fetchTile(tileId) else callback.cancelTile(tileId)
      }
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
     * Delivers a tile notification to the registration [token] names.
     *
     * A token with no registration names a source that has already been removed, or a map that has
     * been closed, so the notification is dropped rather than reaching a callback that has retired.
     */
    fun dispatch(token: Int, kind: Int, z: Int, x: Long, y: Long) {
      val bridge = host.find(token) ?: return
      bridge.invoke(kind, CanonicalTileId(z, x, y))
    }
  }
}
