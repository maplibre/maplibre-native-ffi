package org.maplibre.nativeffi.internal.lifecycle

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.PromisingStack

/**
 * Hands the page back for one turn of its event loop, and resumes on the next.
 *
 * A JSPI suspending import, the same mechanism a dispatched call parks on: it unwinds this Kotlin
 * stack to the event loop and resumes it when the timer below fires. A timer rather than a
 * microtask, because a microtask that queues another is drained before the page runs anything else
 * -- including the timer the dispatcher's drain turn is scheduled on, which is what resumes the
 * stack this is waiting for. Yielding by microtask would starve the progress it yields for.
 */
@JsFun(
  """
  new WebAssembly.Suspending(() => new Promise((resolve) => { setTimeout(() => resolve(0), 0) }))
"""
)
private external fun awaitTurn(): Int

@JsFun("() => Date.now()") private external fun nowMillis(): Double

/**
 * How long one close may go on waiting before what it waits for is called unreachable.
 *
 * Generous, because a legitimate wait is a handful of turns and a page under load can stretch them.
 * What this bounds is the case that never ends, not the case that is slow.
 */
private const val WAIT_LIMIT_MILLIS = 10_000.0

/**
 * The gap that separates one close's waiting from the next one's.
 *
 * A close's turns follow each other within a few milliseconds, because each posts the next as it
 * returns; a larger gap means the previous close finished and this is a new one, whose deadline
 * starts again. Erring long only makes a wait more patient, never less.
 */
private const val RUN_GAP_MILLIS = 250.0

private var lastTurnEndedAt = 0.0
private var waitingSince = 0.0

/**
 * Waits by parking this stack, because on this target nothing else can make the progress.
 *
 * A close drains what has already been admitted: a use count a live call holds, or a callback body
 * that has been entered. Every other platform waits for that by yielding to the thread holding it.
 * A Kotlin/Wasm module has one thread and more than one stack, so what a close waits for here is
 * never another thread — it is a stack that parked partway through and can only be resumed from the
 * event loop. A spin is therefore not slow but fatal: the frame that would release the count cannot
 * run until this one hands the page back, which a spin never does.
 *
 * So this hands it back, a turn at a time, and the count is re-read on the caller's next pass. That
 * is what makes a draining close mean here what it means everywhere else — it returns once the work
 * it is retiring has finished, which is what the binding specification requires of clearing,
 * replacing, and closing a callback.
 *
 * Parking is legal only on a stack `WebAssembly.promising` entered, and every close that can reach
 * this stands on one: a close reaches native through the same dispatch as any other owner-affine
 * call, which is refused outside a `maplibreScope`. A stack without one is reported rather than
 * parked, because the alternative is a virtual-machine trap that names nothing.
 *
 * **The park goes through [PromisingStack.parked], like every other one.** A close that waits is a
 * stack that has left the page, so for the length of the wait it is not the stack running: whatever
 * the page runs meanwhile -- a host's own timer, a queued log record, a proxied callback -- arrives
 * on a stack of its own that may not park and is inside none of the callbacks this one entered.
 * Waiting without saying so would leave the promising count standing while nothing promising was
 * running, which is exactly the reading that lets a page task reach a suspending import and trap
 * instead of being told which scope it left out.
 *
 * **The wait is bounded**, because not everything is waiting for something that can arrive. A use
 * count held by a frame further up this same stack is the invariant the binding states and cannot
 * check in advance, and a callback body that never returns is a host's own; either would leave a
 * scope's promise unsettled for the life of the page with nothing said about why. A run of waits
 * that gets nowhere for [WAIT_LIMIT_MILLIS] reports that instead.
 */
internal actual fun yieldWhileClosing() {
  if (!PromisingStack.isInside()) {
    throw Status.invalidState(
      "A close is waiting for in-flight work on a stack that cannot wait for it. This binding " +
        "waits by parking the calling stack, and WebAssembly allows that only on a stack that " +
        "maplibreScope established."
    )
  }
  val startedAt = nowMillis()
  if (startedAt - lastTurnEndedAt > RUN_GAP_MILLIS) waitingSince = startedAt
  if (startedAt - waitingSince > WAIT_LIMIT_MILLIS) {
    // Cleared so the next close starts its own deadline rather than inheriting this one's.
    lastTurnEndedAt = 0.0
    throw Status.invalidState(
      "A close waited more than ${(WAIT_LIMIT_MILLIS / 1_000).toInt()} seconds for in-flight work " +
        "that never finished. Either a handle's use count was held across a suspending call, " +
        "which the browser binding does not permit, or a callback body it is retiring never " +
        "returned."
    )
  }
  PromisingStack.parked { awaitTurn() }
  lastTurnEndedAt = nowMillis()
}
