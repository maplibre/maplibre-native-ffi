package org.maplibre.nativeffi.internal.wasm

import kotlin.coroutines.Continuation
import kotlin.coroutines.resume
import kotlin.coroutines.suspendCoroutine
import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.wasm.WasmExport

/**
 * The queue drain about to run on a promising stack.
 *
 * A WebAssembly export takes primitives, so the body cannot be passed through one; it is handed
 * over here and the trampoline picks it up. The assignment and the call below it are one step as
 * far as the page is concerned, because a promising call enters its export synchronously, so a
 * single slot is enough.
 */
private var pendingDelivery: (() -> Unit)? = null

/**
 * The entry point an asynchronous delivery unwinds to when it parks.
 *
 * `@WasmExport` rather than `@JsExport` for the same reason `mln_maplibre_scope_entry` is one:
 * `WebAssembly.promising` accepts a raw WebAssembly function, and `@JsExport`'s underlying export
 * name is not a documented contract. Nothing may unwind out of here, so the drain's own failures
 * are contained below rather than crossing the promising boundary as an opaque rejection.
 */
@WasmExport("mln_browser_async_delivery_entry")
internal fun mlnBrowserAsyncDeliveryEntry(): Int {
  val delivery = pendingDelivery ?: return 0
  pendingDelivery = null
  return try {
    // This frame, and nothing above it, is what makes a suspension legal, which is the whole point
    // of routing a notification through here: the host body below may call the owner thread.
    PromisingStack.entered(delivery)
    1
  } catch (_: Throwable) {
    // Each queued body already contains its own failures, so reaching this means the drain itself
    // failed. There is nobody to report that to -- a notification answers nothing -- and letting it
    // out would reject the promising call rather than name anything.
    0
  }
}

@JsFun("() => WebAssembly.promising(wasmExports['mln_browser_async_delivery_entry'])()")
private external fun runDeliveryPromising(): Promise<JsAny?>

/**
 * Schedules the drain for the end of the current task.
 *
 * A microtask rather than a timer, so a notification is delivered in the same turn of the event
 * loop that the module's proxied task placed it in. A timer would add a browser's clamped delay to
 * every tile, and would put the delivery behind whatever else the page had already scheduled.
 */
@JsFun("(pump) => { queueMicrotask(pump) }") private external fun scheduleDelivery(pump: () -> Unit)

/**
 * Runs notifications native sends without waiting for an answer, on a stack that may park.
 *
 * A callback native waits for cannot reach the owner thread: a MapLibre worker is blocked inside
 * `emscripten_proxy_sync` until the page answers, so parking the page on owner-thread work would
 * close the wait graph `src/browser/sync_callback.c` describes. **A callback native does not wait
 * for has no such graph.** `src/browser/custom_geometry.c` posts its tile callbacks with
 * `emscripten_proxy_async` and the worker returns immediately, so nothing at all is blocked while
 * the host body runs, and the only obstacle left is a WebAssembly rule rather than a lock: a
 * suspension is legal only on a stack entered through `WebAssembly.promising`, and a proxied task
 * is an ordinary event-loop task.
 *
 * So the notification is queued here and the module's task returns, and the queue is drained on a
 * promising stack of its own. A host body may then make ordinary owner-affine calls -- the shared
 * `fetchTile { map.setCustomGeometrySourceTileData(...) }` that is the natural shape on every other
 * target -- and the delivery becoming asynchronous is invisible to native, which was told nothing
 * about when the call would happen and is given no answer either way.
 *
 * **What the ordering guarantees are.** One drain runs at a time and it takes the queue in the
 * order it was filled, so notifications are delivered in the order the module posted them to the
 * page -- which is what `src/browser/custom_geometry.c` promises for one MapLibre thread, and it
 * promises nothing about two. What changes is when: a delivery no longer happens inside the
 * module's proxied task but at the first microtask checkpoint after it, and a body that reaches the
 * owner thread holds the drain for as long as its call takes, so everything queued behind it waits.
 * Nothing else the page does is ordered against a delivery, and nothing was before either.
 *
 * **This does not take [SuspensionGate].** It must not: a host scope holds that gate for its whole
 * life, and the notifications this delivers arrive *while that scope is parked* -- a drain that
 * queued behind the gate would be waiting for the very loop that is waiting for its tiles. Taking
 * the gate is also unnecessary, because what the gate protects is not reached here. Kotlin's
 * module-global scoped memory allocator is opened and closed inside one synchronous marshalling
 * step and never held across a park, so a drain's allocator scopes never interleave with a scope's;
 * dispatcher tokens are unique per call rather than per stack; and a handle's use count is not held
 * across a park either, which is what `MapHandle.live` exists to guarantee. What is left is that
 * two stacks are parked at once, which is what JSPI is for.
 */
internal object AsyncDelivery {
  private val queued = ArrayDeque<() -> Unit>()

  /** Whether a drain stack exists, whether it is running or parked. */
  private var active = false

  /** Whether a microtask that will start a drain has been queued and has not run yet. */
  private var scheduled = false

  private val idleWaiters = mutableListOf<Continuation<Unit>>()

  /**
   * Queues [body] for delivery on a promising stack.
   *
   * Called from a WebAssembly export the module's proxied task reaches, so this must be cheap and
   * must not throw: it appends and returns, and everything that could fail happens in the drain.
   */
  fun post(body: () -> Unit) {
    queued.addLast(body)
    schedule()
  }

  /**
   * Waits until nothing is queued and no drain is in flight.
   *
   * The shutdown is what needs this. A drain parked on the owner thread is waiting for a completion
   * that only the dispatcher's drain turn delivers, so stopping the owner thread underneath it
   * would leave that stack suspended for the life of the page, holding whatever its body was
   * inside. Nothing new can be posted by then -- a notification is dropped once its registration
   * has gone, and the shutdown already refuses while any handle is open -- so this terminates.
   */
  suspend fun awaitIdle() {
    if (!active && !scheduled && queued.isEmpty()) return
    suspendCoroutine { continuation -> idleWaiters.add(continuation) }
  }

  private fun schedule() {
    if (scheduled || active) return
    scheduled = true
    scheduleDelivery(::pump)
  }

  private fun pump() {
    scheduled = false
    if (active) return
    if (queued.isEmpty()) {
      wakeIdle()
      return
    }
    active = true
    // Calling the entry point from Kotlin is what keeps its export in the linked binary. The only
    // other reference is the JavaScript string above, and dead-code elimination cannot see inside
    // one, so without this the export survives a build that compiles this module with its caller
    // and is dropped from every build that links it as a klib. With nothing pending it returns
    // immediately and does nothing, which is what makes it usable as the reference.
    mlnBrowserAsyncDeliveryEntry()
    pendingDelivery = ::drain
    runDeliveryPromising()
      .then(
        onFulfilled = {
          settled()
          null
        },
        onRejected = {
          settled()
          null
        },
      )
  }

  /**
   * Delivers everything queued, including whatever arrives while this is parked.
   *
   * The queue is re-read on every turn rather than snapshotted, because a body that reaches the
   * owner thread hands the page back, and the module's proxied tasks run there -- so a notification
   * posted during one delivery is delivered by this same drain, in the order it arrived, rather
   * than waiting for a drain after it.
   */
  private fun drain() {
    while (true) {
      val next = queued.removeFirstOrNull()
      if (next == null) {
        // Cleared from inside the stack rather than from the settlement below, so that a post made
        // between this stack returning and its promise being observed schedules a new drain instead
        // of being left for one that is no longer coming.
        active = false
        return
      }
      next()
    }
  }

  /**
   * Picks up after a drain stack has finished, however it finished.
   *
   * A stack that trapped never reached the end of [drain], so it left the queue as it was and
   * [active] set; this is where that is put right, and the notifications it did not reach are taken
   * up by the next drain rather than stranded.
   */
  private fun settled() {
    active = false
    if (queued.isEmpty()) wakeIdle() else schedule()
  }

  private fun wakeIdle() {
    if (idleWaiters.isEmpty()) return
    val waiting = idleWaiters.toList()
    idleWaiters.clear()
    for (continuation in waiting) continuation.resume(Unit)
  }
}
