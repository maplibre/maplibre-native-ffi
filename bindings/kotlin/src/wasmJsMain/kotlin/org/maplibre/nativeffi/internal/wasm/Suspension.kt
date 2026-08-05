package org.maplibre.nativeffi.internal.wasm

import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.coroutines.suspendCoroutine
import kotlin.js.ExperimentalWasmJsInterop
import kotlin.js.JsAny
import kotlin.js.Promise
import org.maplibre.nativeffi.internal.callback.RunningCallbacks
import org.maplibre.nativeffi.internal.status.Status

/**
 * Awaits a JavaScript promise from a Kotlin coroutine.
 *
 * The binding depends on the standard library's coroutine primitives rather than on
 * kotlinx-coroutines: a low-level binding should not put a concurrency library on every consumer's
 * classpath to await one promise.
 */
@OptIn(ExperimentalWasmJsInterop::class)
internal suspend fun Promise<JsAny?>.awaitOrThrow(): JsAny? = suspendCoroutine { continuation ->
  then(
    onFulfilled = { value ->
      continuation.resume(value)
      null
    },
    onRejected = { error ->
      // A rejection carries a JavaScript value rather than a Kotlin throwable, and nothing here
      // can recover the original type. The value's own description is what the caller sees, in
      // the binding's own error shape so a host catches one family of exception.
      continuation.resumeWithException(
        Status.invalidState("The browser module rejected a call: " + error?.toString())
      )
      null
    },
  )
}

/**
 * Serializes every suspending region that touches Kotlin's linear memory or binding state.
 *
 * `withScopedMemoryAllocator` is a module-global parent/child stack, not a per-caller one. Two
 * regions that suspend while both hold a scope can therefore resume out of order and unwind each
 * other's allocators, and the shared callback registries take short spin locks that a suspended
 * holder would never release. Neither problem is per-runtime, so this gate is not either: it is one
 * gate for the whole module.
 *
 * This is a queue rather than a lock. A waiter parks its continuation and yields the page, so the
 * event loop keeps running -- which is what lets the suspended holder's promise settle at all.
 *
 * Callbacks deliberately do not take this gate, and could not: they arrive while the scope that
 * holds it is parked, so a callback that waited for it would be waiting for the loop that is
 * waiting for it. A synchronous callback runs to completion before it returns to native, so any
 * allocator scope it opens is properly nested inside the holder's, which is exactly the nesting the
 * allocator supports. An [AsyncDelivery] drain may park instead, and is safe for a different
 * reason: no allocator scope is ever held across a park, so its scopes and a host scope's never
 * interleave.
 */
internal object SuspensionGate {
  private var held = false
  private val waiting = ArrayDeque<(Unit) -> Unit>()

  suspend fun <T> withGate(block: suspend () -> T): T {
    acquire()
    try {
      return block()
    } finally {
      release()
    }
  }

  private suspend fun acquire() {
    if (!held) {
      held = true
      return
    }
    suspendCoroutine { continuation -> waiting.addLast { continuation.resume(Unit) } }
  }

  private fun release() {
    val next = waiting.removeFirstOrNull()
    if (next == null) {
      held = false
      return
    }
    // The gate stays held across the handoff, so a caller that acquires between this resumption
    // being scheduled and it running cannot overtake the queue.
    next(Unit)
  }
}

/**
 * Tracks whether the calling frame is inside one of this binding's *synchronous* callbacks.
 *
 * Such a callback is entered from native on a stack that was never wrapped by
 * `WebAssembly.promising`, so nothing in it may park on a promise, and it is entered while a
 * MapLibre worker is blocked waiting for what it returns. Both halves matter. The stack rule alone
 * could be met by delivering on a promising stack instead, which is what [AsyncDelivery] does for
 * the callbacks native does not wait for; the blocked worker is what makes that unavailable here,
 * because a page parked on the owner thread while a worker is parked on the page is the cycle
 * `src/browser/sync_callback.c` describes.
 *
 * The suspension gate cannot describe this either: the gate is held by the parked scope the
 * callback arrived under, so a callback that waited for it would deadlock.
 *
 * So dispatch asks this instead. It is a plain counter because a Kotlin/Wasm module is one thread.
 */
internal object CallbackScope {
  private var depth = 0

  fun <T> inside(body: () -> T): T {
    depth++
    try {
      return body()
    } finally {
      depth--
    }
  }

  fun isInside(): Boolean = depth > 0
}

/**
 * Tracks whether the calling frame stands on a stack that `WebAssembly.promising` entered.
 *
 * Parking on a promise is legal only on such a stack, and `maplibreScope` is what establishes one.
 * A host that calls an owner-affine API without it would otherwise get a virtual-machine trap or an
 * opaque JavaScript error from inside a suspending import, naming neither the scope nor the call --
 * and forgetting the scope is easy, because every other target's actuals are ordinary synchronous
 * functions with no wrapper around them. So the dispatch path asks this first and fails closed.
 *
 * Suspendability is a property of one stack rather than of the module, which is why this is more
 * than a flag set for the length of a scope. While a scope is parked, its promising stack is not
 * running: anything that enters Kotlin meanwhile -- a queued log record, a proxied callback, a
 * host's own timer -- arrives on a fresh page stack that may not park. [parked] is what makes those
 * frames read as what they are, by surrendering the count for exactly as long as the suspension
 * lasts.
 *
 * A plain counter because a Kotlin/Wasm module is one thread. More than one promising stack can
 * exist at once — a host scope and an [AsyncDelivery] drain that started while it was parked — and
 * the count is still only ever zero or one, because a page runs one of them at a time and the
 * parked ones have each given their count back.
 */
internal object PromisingStack {
  private var depth = 0

  /** Runs [body] as the body of a promising stack. */
  fun <T> entered(body: () -> T): T {
    depth++
    try {
      return body()
    } finally {
      depth--
    }
  }

  /**
   * Runs [body], which unwinds this stack to the event loop and is resumed on it.
   *
   * The count is given back for the duration, so that anything entering Kotlin while this stack is
   * away is told it may not park -- which it may not, being on a stack of its own.
   *
   * The callbacks this stack is inside are surrendered here too, and for the same reason: the stack
   * that runs meanwhile entered none of them. That is what lets a close tell a callback body it is
   * standing on from one that is suspended elsewhere, which is the difference between a wait it
   * must not make and one it must. This is the only place a stack parks, so it is the only place
   * either has to be said.
   *
   * The module has two suspending imports and they both come through here: `Dispatcher.awaitCall`,
   * which waits for a call placed on the owner thread, and `CloseYield.awaitTurn`, which is how a
   * draining close hands the page back to whatever it is waiting for. A park added anywhere else
   * has to be routed through this too, because everything that reads the two counts reads them as
   * belonging to the stack that is running.
   */
  fun <T> parked(body: () -> T): T {
    depth--
    try {
      return RunningCallbacks.whileParked(body)
    } finally {
      depth++
    }
  }

  fun isInside(): Boolean = depth > 0
}
