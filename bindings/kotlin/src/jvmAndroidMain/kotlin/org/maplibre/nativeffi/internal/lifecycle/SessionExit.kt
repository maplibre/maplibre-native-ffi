package org.maplibre.nativeffi.internal.lifecycle

import java.util.concurrent.atomic.AtomicBoolean
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.RenderAbandonResultLayout
import org.maplibre.nativeffi.internal.c.Upcalls
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

// Every render session that the binding attached, for the shutdown hook to abandon.
private val exitSessions = ExitSessions()

private val exitHookInstalled = AtomicBoolean(false)

internal actual fun endGraphicsAtExit(state: HandleState) {
  if (exitHookInstalled.compareAndSet(false, true)) {
    try {
      Runtime.getRuntime().addShutdownHook(Thread(::runExitHook, "maplibre-render-session-exit"))
    } catch (_: IllegalStateException) {
      // Shutdown has already begun, so no hook will run: abandon what other threads enrolled
      // meanwhile, and let enroll abandon this session and every later one.
      exitSessions.abandonAll()
    }
  }
  exitSessions.enroll(state)
}

// Abandons every render session still held, unless exit began inside a MapLibre callback.
//
// The thread that calls `System.exit` waits for this hook to finish. When that thread is inside a
// MapLibre callback, it can be a core worker inside a session's driver call, and abandon from
// another thread waits for that call to end, so abandoning would hang the process. C also forbids
// abandon from inside a MapLibre callback. A host that exits from a callback therefore ends its
// sessions' graphics calls itself first.
private fun runExitHook() {
  if (exitStartedInCallback(Thread.getAllStackTraces().values)) return
  exitSessions.abandonAll()
}

// The class whose static methods every native callback into the binding enters through: the FFM
// upcall stubs target it on the JVM, and the JNI shim dispatches to it on Android.
private val UPCALL_ENTRY_CLASS: String = Upcalls::class.java.name

private fun exitStartedInCallback(stacks: Collection<Array<StackTraceElement>>): Boolean =
  stacks.any { stack ->
    // A stack trace lists the innermost frame first, so the callback's entry follows the exit.
    val exit = stack.indexOfFirst { it.className == "java.lang.Runtime" && it.methodName == "exit" }
    exit >= 0 && insideCallback(stack, from = exit + 1)
  }

private fun insideCallback(stack: Array<StackTraceElement>, from: Int = 0): Boolean =
  (from until stack.size).any { stack[it].className == UPCALL_ENTRY_CLASS }

/**
 * The render sessions to abandon at exit.
 *
 * Each entry holds a session's state, never its wrapper, so the leak cleaner still runs for an
 * unreachable session. Released entries are pruned as the list grows.
 */
internal class ExitSessions {
  private val lock = Any()
  private var entries = ArrayList<HandleState>()
  private var pruneAt = PRUNE_FLOOR
  private var exiting = false

  /** Holds [state] until [abandonAll], or abandons it now when [abandonAll] has already run. */
  fun enroll(state: HandleState) {
    synchronized(lock) {
      if (!exiting) {
        if (entries.size >= pruneAt) {
          entries.removeAll { it.isReleased }
          pruneAt = maxOf(entries.size * 2, PRUNE_FLOOR)
        }
        entries.add(state)
        return
      }
    }
    if (!insideCallback(Thread.currentThread().stackTrace)) abandonAtExit(state.handle)
  }

  /**
   * Abandons every session held that its owner has not released, and every session enrolled later.
   *
   * Abandon ends a core-worker session's graphics calls, waiting for a driver call in flight. A
   * session whose host thread is inside a caller-driven call stays as the host left it. A session
   * that the leak cleaner already disposed may still be detaching. Abandon from inside a MapLibre
   * callback is forbidden, so a callback thread abandons nothing.
   */
  fun abandonAll() {
    val pending =
      synchronized(lock) {
        exiting = true
        entries.also { entries = ArrayList() }
      }
    if (pending.isEmpty() || insideCallback(Thread.currentThread().stackTrace)) return
    for (state in pending) {
      if (!state.isReleased) abandonAtExit(state.handle)
    }
  }

  private companion object {
    const val PRUNE_FLOOR = 64
  }
}

// Abandons the render session [handle] for exit, whatever state the session is in.
//
// Every status is ignored: busy leaves a caller-driven session as its host left it, and an invalid
// state or argument means that the session already released its target or its handle.
private fun abandonAtExit(handle: Long) {
  NativeArena().use { call ->
    val out = call.sized(RenderAbandonResultLayout.SIZEOF, RenderAbandonResultLayout.ALIGNOF)
    C.mln_render_session_abandon(handle, out, NativeDiagnostics.buffer())
  }
}
