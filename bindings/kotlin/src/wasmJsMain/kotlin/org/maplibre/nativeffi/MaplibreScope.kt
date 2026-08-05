package org.maplibre.nativeffi

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.wasm.WasmExport
import org.maplibre.nativeffi.internal.wasm.AsyncDelivery
import org.maplibre.nativeffi.internal.wasm.BrowserModule
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.internal.wasm.PromisingStack
import org.maplibre.nativeffi.internal.wasm.SuspensionGate
import org.maplibre.nativeffi.internal.wasm.awaitOrThrow

/**
 * The block a promising stack is about to run, and what it produced.
 *
 * A WebAssembly export takes primitives, so the block cannot be passed through one. It is handed
 * over here instead, and the trampoline picks it up. Only one is ever in flight because
 * [SuspensionGate] serializes scopes.
 */
private var pendingBlock: (() -> Unit)? = null
private var pendingFailure: Throwable? = null

/**
 * The entry point every suspension inside a scope unwinds to.
 *
 * `@WasmExport` rather than `@JsExport` because this has to be a *raw* WebAssembly export for
 * `WebAssembly.promising` to accept it, and `@JsExport`'s underlying export name is not a
 * documented contract. The name is fixed here so the trampoline below can resolve it.
 */
@WasmExport("mln_maplibre_scope_entry")
internal fun maplibreScopeEntry(): Int {
  val block = pendingBlock ?: return 0
  pendingBlock = null
  return try {
    // This frame, and nothing above it, is what makes a suspension legal. Marked so that a call
    // reaching the owner thread from anywhere else reports the missing scope instead of trapping.
    PromisingStack.entered(block)
    1
  } catch (failure: Throwable) {
    // Carried out rather than thrown across the promising boundary, where it would arrive as an
    // opaque rejection with the Kotlin type lost.
    pendingFailure = failure
    0
  }
}

@JsFun("() => WebAssembly.promising(wasmExports['mln_maplibre_scope_entry'])()")
private external fun runPromising(): Promise<JsAny?>

/**
 * Runs [block] on a stack that may park on native work.
 *
 * This is the browser binding's one departure from the API every other platform presents, and it
 * exists because of how the same-ness is achieved rather than in spite of it. Owner-affine calls
 * run on a thread the module owns, and a caller waits for them by parking its own stack on a
 * promise — a WebAssembly feature that is only legal on a stack entered through
 * `WebAssembly.promising`. This establishes that stack.
 *
 * Inside, host code is identical to what it would be on JVM, Android, or Kotlin/Native: ordinary
 * synchronous calls on ordinary handles. A map loop lives inside one scope for its whole life.
 *
 * Scopes are serialized. Kotlin's scoped memory allocator is module-global, so two scopes parked at
 * once could unwind each other's allocations; a second caller waits here rather than interleaving.
 */
public suspend fun <T> maplibreScope(block: () -> T): T = SuspensionGate.withGate {
  var produced: Any? = null
  // A scope whose trampoline never ran leaves its block behind, and the next scope must not pick up
  // that one instead of its own.
  pendingBlock = null
  // Calling the entry point from Kotlin is what keeps its export in the linked binary. The only
  // other reference to it is the JavaScript string in runPromising, and dead-code elimination
  // cannot see inside a string: without this call the export survives a build that compiles this
  // module together with its caller, and is dropped from every build that links it as a klib --
  // which is every host that consumes the published artifact. With no block pending the entry
  // returns immediately and does nothing, which is what makes it usable as the reference.
  maplibreScopeEntry()
  pendingBlock = { produced = block() }
  runPromising().awaitOrThrow()
  pendingFailure?.let {
    pendingFailure = null
    throw it
  }
  @Suppress("UNCHECKED_CAST")
  produced as T
}

/**
 * Stops the thread this binding's runtimes ran on, for good.
 *
 * Called once, after every handle is closed. The module's own contract is destroy, then drain, then
 * stop: a call still in flight has storage the owner thread may still be writing, and an
 * owner-affine handle can only ever be destroyed on that thread.
 *
 * Suspending, and it takes the same gate [maplibreScope] does, because it must not run while a
 * scope is parked. Stopping there would leave that scope waiting on a completion nothing polls for
 * any more: its stack never resumes, its scratch is never freed, and any handle it had just created
 * is lost with the thread that alone could destroy it. Holding that gate is also what makes the
 * "drain" half of the contract hold by itself, since a call is outstanding only while the scope
 * that placed it holds the same gate.
 *
 * **A handle still open refuses the shutdown**, naming what is open, the way closing a runtime with
 * a live map does. The refusal leaves the owner thread running, so a host closes what was named and
 * calls this again. It is the destroy half of the contract made checkable: a handle that outlived
 * its scope is an ordinary live object, and stopping the one thread that could destroy it would
 * otherwise lose it with nothing said.
 *
 * **What it does not refuse for is state that lives only in the module's heap.** A
 * [WakeSource][org.maplibre.nativeffi.runtime.WakeSource], a
 * [NativeBuffer][org.maplibre.nativeffi.render.NativeBuffer], and a resource request handle a
 * provider still holds need no particular thread to release, and the release below reclaims all
 * three at once by discarding the heap they live in. Closing one afterwards therefore succeeds and
 * does nothing, rather than reaching a module that is gone. The process-global log callback is the
 * same rule inverted: it is a Kotlin reference that releasing the module would *not* reclaim, and
 * that no later call could drop either, so this drops it.
 *
 * **This is final.** No later call starts another owner thread; every one reports an invalid-state
 * failure naming the shutdown instead. A thread started afterwards would be a thread that has never
 * seen the handles the host still holds, so it could only answer them with the C API's wrong-thread
 * status -- and a page cannot restart the part that matters anyway, because a canvas given to a
 * thread cannot be given to a second one. A host that has shut down and wants a map again reloads
 * the page.
 *
 * **This releases the module too.** Stopping the owner thread alone would leave the Emscripten
 * module's worker pool and its heap reachable for the life of the document, which is most of what a
 * single-page host wanted back. So the pool is terminated and this page's reference to the instance
 * is dropped, in that order and as one step: a worker terminated while it holds the module's
 * allocator lock leaves that lock held, and dropping the reference is what guarantees nothing on
 * the page allocates afterwards. Every later call reports the release rather than loading a second
 * sixteen-worker module.
 *
 * **The owner thread is given a moment to finish first.** Its stop arrives as a wake rather than as
 * a call that returns, so terminating the pool straight away can kill the thread before it has
 * drained what was queued, dropped the keepalive that ends it, or freed the dispatcher. The wait is
 * bounded and polls page tasks rather than blocking, and expiry releases the module anyway: a page
 * that cannot finish shutting down is worse than a thread killed part way through a teardown whose
 * storage goes with the heap in any case.
 *
 * @throws org.maplibre.nativeffi.error.InvalidStateException if a handle created on the owner
 *   thread is still open.
 */
public suspend fun shutdownMaplibre() {
  SuspensionGate.withGate {
    // A tile notification is delivered on a stack of its own, which this gate does not describe, so
    // one can be parked on the owner thread at this moment, and stopping underneath it would leave
    // that stack suspended for the life of the page. Waited for only once every handle has gone,
    // because until then native can still be asking this page for tiles and there would be no end
    // to wait for -- and a shutdown with a handle open is refused below in any case.
    if (Dispatcher.openHandles.isEmpty()) AsyncDelivery.awaitIdle()
    Dispatcher.stop()
    // The one root that survives releasing the module, because it is a Kotlin reference rather than
    // anything in the module's heap, and the one a host could not drop afterwards. Dropped here,
    // while the module is still there for the clear to take its ordinary path.
    Maplibre.discardLogCallbackAfterShutdown()
    // Between the two because that is the only place it fits: the stop above is what it waits for,
    // and the release below is what it has to happen before. Nothing it waits through can reach the
    // page any more either, since the callback that a drain turn would have called has just gone.
    BrowserModule.awaitOwnerThreadRelease()
    // Last, and only once the stop was accepted: a refused stop throws above, which is what leaves
    // the module intact for the host to close what was named and try again. Nothing between the two
    // reaches the module for anything but that wait -- stopping allocates nothing after it posts
    // its wake, and a drain turn returns immediately once the dispatcher handle is gone.
    BrowserModule.discardAfterShutdown()
  }
}
