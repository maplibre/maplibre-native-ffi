package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus

/**
 * Failures this suite has to prove the binding survives, and which the module will not produce.
 *
 * **This exists for the tests.** Nothing in the binding arms it, and everything below is inert
 * until something does, so each hook is one field read on the path it sits on. The binding
 * specification allows an internal seam for an allocation or copy failure raised after a native
 * handle has been acquired -- both a result handle's copy and the wrapper an acquired frame is
 * still to be given -- because neither can be produced through the public library on demand.
 *
 * What the injected failures replace is the answer, never the recovery. A faulted copy does not
 * reach native at all, which is what makes the state afterwards the real thing rather than a
 * simulation of it: the result handle it refused is still live, so the replay that follows really
 * does tell a released handle from a leaked one.
 */
internal object InjectedFaults {
  private var failResultCopies = false
  private val copiedResults = mutableListOf<Long>()

  private var failNextFrameWrap = false

  private var failedEntryPoint: String? = null
  private var failedStatus = MaplibreStatus.OK
  private var failedDiagnostic = ""

  /** Forgets an arming that was never taken, so a failing test cannot leak one. */
  fun reset() {
    failResultCopies = false
    copiedResults.clear()
    failNextFrameWrap = false
    failedEntryPoint = null
  }

  /**
   * Makes the next call to [entryPoint] report [status] with [diagnostic] instead of reaching it.
   *
   * The calls that carry this seam are the registration installs and the frame release: each one
   * hands native something the binding has already built, and the recovery afterwards -- keeping
   * the previous registration, keeping the frame open for a retry -- is what BND-122 and BND-169
   * ask for. Native refuses none of them for any input the public library can produce.
   *
   * Naming the entry point rather than intercepting one is deliberate. Every call is now a direct
   * extern, so there is no chokepoint to hook; the seam is the [beginCall] line at the few sites
   * that need it, and it is inert for every other call in the binding.
   */
  fun failNextCall(entryPoint: String, status: MaplibreStatus, diagnostic: String) {
    failedEntryPoint = entryPoint
    failedStatus = status
    failedDiagnostic = diagnostic
  }

  /** Reports the armed failure for [entryPoint], once, instead of letting the call through. */
  fun beginCall(entryPoint: String) {
    if (failedEntryPoint != entryPoint) return
    failedEntryPoint = null
    throw MaplibreException.forStatus(failedStatus, failedStatus.nativeCode, failedDiagnostic)
  }

  /**
   * Makes the next owned-frame acquisition fail after native has handed the frame over.
   *
   * The window this stands in is the one BND-172 names: native has the frame, and the binding has
   * still to copy the descriptor into a Kotlin value and wrap it. Both of those are object
   * construction, which fails only when the Kotlin heap is exhausted -- a condition a host cannot
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

  /** Makes every result-handle copy fail as an allocation failure, until [takeCopiedResults]. */
  fun failResultCopies() {
    failResultCopies = true
    copiedResults.clear()
  }

  /**
   * Disarms the copy failure and reports the result handles it was asked about, oldest first.
   *
   * The handles are what the test replays: a destroyed one is stale to native, so replaying it is
   * how a caller tells a released result handle from a leaked one. Nothing else can -- a leaked
   * handle does nothing observable until the module runs out of table slots.
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
   * through a block this binding has to allocate first.
   *
   * What cannot be produced here is the *placement*, not the failure. A real allocation failure is
   * ordinary now that the module reports one instead of aborting, but exhausting the heap in the
   * window between native handing back a result handle and the copy would mean filling half a
   * gigabyte and leaving it full for whichever test ran next.
   */
  fun beginResultCopy(handle: Long, bytes: Int) {
    if (!failResultCopies) return
    copiedResults += handle
    throw Heap.allocationFailure(bytes)
  }
}
