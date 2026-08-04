package org.maplibre.nativeffi.internal.wasm

import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.coroutines.suspendCoroutine
import kotlin.js.ExperimentalWasmJsInterop
import kotlin.js.JsAny
import kotlin.js.Promise
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
 * Callbacks deliberately do not take this gate. A callback runs to completion before it returns to
 * native, so any allocator scope it opens is properly nested inside the holder's, which is exactly
 * the nesting the allocator supports.
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
 * Tracks whether the calling frame is inside one of this binding's callbacks.
 *
 * A callback is entered from native, on a stack that was never wrapped by `WebAssembly.promising`,
 * so nothing in it may park on a promise. It also runs *while a scope may be parked*, which is why
 * the suspension gate cannot describe it: the gate is held by the parked scope, and a callback that
 * waited for it would deadlock, while one that ignored it would dispatch a second call the gate was
 * supposed to prevent.
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

// A "may this frame suspend?" predicate beyond the above deliberately does not live here.
// Suspendability is a
// property of one stack, not of the module: while a public stack is parked, a callback can enter
// Kotlin on a stack that is *not* promising, and a global flag would report the parked stack's
// answer to it. Two scopes completing out of order defeat save-and-restore for the same reason.
// The dispatch path carries an explicit per-invocation token instead, so a callback frame is
// constructed with `canSuspend = false` and cannot consult anything ambient.
