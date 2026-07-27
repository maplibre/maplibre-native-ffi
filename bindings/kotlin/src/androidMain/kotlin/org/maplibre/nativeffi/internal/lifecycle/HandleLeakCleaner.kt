package org.maplibre.nativeffi.internal.lifecycle

import java.lang.ref.PhantomReference
import java.lang.ref.ReferenceQueue
import java.util.concurrent.ConcurrentHashMap
import org.maplibre.nativeffi.render.OwnedTextureFrameHandleCore
import org.maplibre.nativeffi.runtime.OfflineOperationLeakReport

/**
 * Reports owner-thread-affine native handles that become unreachable before explicit release.
 *
 * Registration runs a diagnostic line on a daemon reporter thread once the public wrapper is
 * unreachable. Runtime, map, projection, and render-session handles are bound to their owner
 * thread, so this hook MUST NOT destroy them: explicit release on the owner thread stays the only
 * path that frees native state. Reporting a leak keeps the failure visible while leaving native
 * ownership untouched.
 *
 * This mirrors the JVM source set, which uses `java.lang.ref.Cleaner`. Android supports that class
 * from API 33 while this binding targets a lower minimum, so the same contract is built here from
 * `PhantomReference` and `ReferenceQueue`, which are available across the supported range.
 *
 * Registered actions capture leak-report state only. Capturing the wrapper would keep it reachable
 * and suppress every report.
 */
internal object HandleLeakCleaner {
  private val queue = ReferenceQueue<Any>()

  /** Keeps registrations reachable so the collector can enqueue them. */
  private val pending = ConcurrentHashMap.newKeySet<LeakRegistration>()

  /** Keeps host callback state reachable for as long as native may invoke it. */
  private val nativeCallbackRoots = ConcurrentHashMap.newKeySet<Any>()

  init {
    Thread(::drain, "maplibre-handle-leak-reporter").apply { isDaemon = true }.start()
  }

  /** Reports [leakReport] when [handle] becomes unreachable before explicit release. */
  fun register(handle: Any, leakReport: HandleStateCore.LeakReport) {
    pending.add(LeakRegistration(handle, queue) { leakReport.report() })
  }

  /** Retains callback state independently of its public native-owner wrapper. */
  fun retainNativeCallbackRoot(root: Any) {
    nativeCallbackRoots.add(root)
  }

  /** Releases callback state after native can no longer invoke it. */
  fun releaseNativeCallbackRoot(root: Any?) {
    if (root != null) {
      nativeCallbackRoots.remove(root)
    }
  }

  /** Reports [frameCore] when [handle] becomes unreachable before explicit release. */
  fun registerFrame(handle: Any, frameCore: OwnedTextureFrameHandleCore) {
    pending.add(LeakRegistration(handle, queue) { frameCore.reportLeak() })
  }

  /** Reports [leakReport] when an offline operation becomes unreachable. */
  fun registerOfflineOperation(handle: Any, leakReport: OfflineOperationLeakReport) {
    pending.add(LeakRegistration(handle, queue) { leakReport.report() })
  }

  private fun drain() {
    while (true) {
      val registration =
        try {
          queue.remove()
        } catch (_: InterruptedException) {
          continue
        }
      if (registration is LeakRegistration) {
        pending.remove(registration)
        registration.report()
      }
    }
  }

  private class LeakRegistration(
    referent: Any,
    queue: ReferenceQueue<Any>,
    private val reportLeak: () -> Unit,
  ) : PhantomReference<Any>(referent, queue) {
    fun report() {
      reportLeak()
    }
  }
}
