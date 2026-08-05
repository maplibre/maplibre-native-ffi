package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

/**
 * Failures this suite has to prove the binding survives, and which the module will not produce.
 *
 * **This exists for the tests.** Nothing in the binding arms it, and everything below is inert
 * until something does: the entry point armed for failure is null and the result-copy flag is
 * false, so each hook is one field read on the path it sits on. The binding specification allows an
 * internal seam for exactly these failures — native destroy, request release, frame release and
 * callback-install failure, and an allocation or copy failure after a result handle is acquired —
 * because none of them can be produced through the public library on demand.
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
  }

  /**
   * Reports whether [entry] is armed to fail, having written its status into [result].
   *
   * Called by [Dispatcher.call] where the submission would go. The status lands in the caller's own
   * result slot, so the reader above turns it into an exception by the ordinary path rather than by
   * one this knows about.
   */
  fun injectCallFailure(entry: String, result: HeapPointer): Boolean {
    if (faultedEntry != entry) return false
    faultedEntry = null
    NativeDiagnostics.setProxiedDiagnostic(faultedDiagnostic)
    Heap.storeInt(result, faultedStatus)
    return true
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
