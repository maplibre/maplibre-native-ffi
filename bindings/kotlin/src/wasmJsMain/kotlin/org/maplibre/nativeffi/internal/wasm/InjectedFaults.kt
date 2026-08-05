package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.status.Status

/**
 * Failures this suite has to prove the binding survives, and which the module will not produce.
 *
 * **This exists for the tests.** Nothing in the binding arms it, and everything below is inert
 * until something does: the entry point armed for failure is null and every flag is false, so each
 * hook is one field read on the path it sits on. The binding specification allows an internal seam
 * for exactly these failures — native destroy, request release, frame release and callback-install
 * failure, and an allocation or copy failure after a native handle is acquired, which covers both a
 * result handle's copy and the wrapper an acquired frame is still to be given — because none of
 * them can be produced through the public library on demand.
 *
 * The drain-turn failure below is this binding's own addition to that set, under the rule the list
 * illustrates rather than under one of its entries. A completion drain turn is a page task the
 * dispatcher scheduled for itself, so nothing a host or the module can be asked to do reaches it,
 * and what it must survive is not a status but the fact of having thrown at all.
 *
 * What the injected failures replace is the answer, never the recovery. A faulted call does not
 * reach native at all, which is what makes the state afterwards the real thing rather than a
 * simulation of it: a frame whose release was refused is still acquired natively, so the retry that
 * follows really does release it, and a provider replacement that was refused really does leave the
 * runtime holding its predecessor. A seam that let the call through and rewrote its status would
 * prove neither.
 *
 * Every arming is one-shot, taken by the first call that matches, so a test that fails part way
 * through cannot leave a fault standing for whichever test the framework runs next.
 */
internal object InjectedFaults {
  private var faultedEntry: String? = null
  private var faultedStatus: Int = 0
  private var faultedDiagnostic: String = ""

  private var failResultCopies = false
  private val copiedResults = mutableListOf<Long>()

  private var failNextDrainTurn = false
  private var failNextFrameWrap = false

  /**
   * Makes the next dispatched call to [entry] report [status] without reaching the owner thread.
   *
   * [diagnostic] stands in for the message native would have left, which the caller's exception
   * copies exactly as it copies a real one.
   */
  fun failNextCall(entry: String, status: MaplibreStatus, diagnostic: String) {
    faultedEntry = entry
    faultedStatus = status.nativeCode
    faultedDiagnostic = diagnostic
  }

  /** Forgets an arming that was never taken, so a failing test cannot leak one. */
  fun reset() {
    faultedEntry = null
    failResultCopies = false
    copiedResults.clear()
    failNextDrainTurn = false
    failNextFrameWrap = false
  }

  /**
   * Makes the next completion drain turn throw before it takes anything.
   *
   * The turn is the one piece of this binding that runs with no caller: a page task the dispatcher
   * scheduled for itself. Nothing a host can do makes one fail, and what a failure costs is the
   * whole page rather than one call -- the turn is what resolves every parked caller -- so it is
   * worth proving that a turn which failed leaves the drain able to run again.
   *
   * The failure it raises stands for any of them rather than naming one, which is the whole point:
   * what the turn has to survive is that it threw, not what it threw. It arrives as an uncaught
   * error on the page, because a task with no caller has nowhere else to report to; a test that
   * arms this takes the page's error handler for as long as the arming stands.
   */
  fun failNextDrainTurn() {
    failNextDrainTurn = true
  }

  /** Fails one drain turn if that is armed. Called at the top of [Dispatcher]'s turn. */
  fun injectDrainFailure() {
    if (!failNextDrainTurn) return
    failNextDrainTurn = false
    throw Status.invalidState("An injected fault failed a MapLibre Native completion drain turn")
  }

  /**
   * Makes the next owned-frame acquisition fail after native has handed the frame over.
   *
   * The window this stands in is the one BND-172 names: native has the frame, and the page has
   * still to copy the descriptor into a Kotlin value and wrap it. Both of those are object
   * construction, which fails only when the Kotlin heap is exhausted -- a condition a page cannot
   * ask for and could not leave behind for the next test if it could.
   */
  fun failNextFrameWrap() {
    failNextFrameWrap = true
  }

  /** Fails one frame wrap, of a descriptor of [bytes], if that is armed. */
  fun beginFrameWrap(bytes: Int) {
    if (!failNextFrameWrap) return
    failNextFrameWrap = false
    throw Heap.allocationFailure(bytes)
  }

  /**
   * Reports the diagnostic [entry] is armed to fail with, having written its status into [result],
   * or null when nothing is armed for it.
   *
   * Called by [Dispatcher.call] where the submission would go. The status lands in the caller's own
   * result slot and the message goes back to the caller rather than being published from here, so
   * both reach the reader above by the ordinary path rather than by one this knows about -- and so
   * a faulted call's message is scoped to that call exactly as a real one is.
   */
  fun injectedCallFailure(entry: String, result: HeapPointer): String? {
    if (faultedEntry != entry) return null
    faultedEntry = null
    Heap.storeInt(result, faultedStatus)
    return faultedDiagnostic
  }

  /** Makes every result-handle copy fail as an allocation failure, until [takeCopiedResults]. */
  fun failResultCopies() {
    failResultCopies = true
    copiedResults.clear()
  }

  /**
   * Disarms the copy failure and reports the result handles it was asked about, oldest first.
   *
   * The handles are what the test replays: a destroyed one is stale to native, so replaying it is
   * how a page tells a released result handle from a leaked one. Nothing else can — a leaked handle
   * does nothing observable until the module runs out of table slots.
   */
  fun takeCopiedResults(): List<Long> {
    failResultCopies = false
    val copied = copiedResults.toList()
    copiedResults.clear()
    return copied
  }

  /**
   * Fails the copy of [handle], which is about to acquire [bytes] of scratch, if that is armed.
   *
   * Called at the top of every read that copies a native snapshot, list, or result handle and
   * destroys it. The failure is the one the module's allocator would have raised for that scratch,
   * because that is the failure the copy has: everything below this line reads native storage
   * through a block the page has to allocate first.
   *
   * What cannot be produced here is the *placement*, not the failure. A real allocation failure is
   * ordinary now that the module reports one instead of aborting — `NativeBufferBrowserTest` asks
   * for more heap than exists and gets it — but exhausting the heap in the window between native
   * handing back a result handle and the page copying it would mean filling half a gigabyte and
   * leaving it full for whichever test ran next.
   */
  fun beginResultCopy(handle: Long, bytes: Int) {
    if (!failResultCopies) return
    copiedResults += handle
    throw Heap.allocationFailure(bytes)
  }
}
