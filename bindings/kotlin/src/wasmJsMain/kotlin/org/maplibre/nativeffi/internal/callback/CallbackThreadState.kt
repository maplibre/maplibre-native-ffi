package org.maplibre.nativeffi.internal.callback

/**
 * The callback bodies the stack that is *running* has entered.
 *
 * Every other platform answers this with a thread local, because native can invoke a callback on
 * any thread and the thread is what tells one caller from another. A Kotlin/Wasm module runs on one
 * thread, so there is no thread local to hang it on -- and a plain counter is not the answer
 * either, because one thread here carries more than one stack. A body that parks unwinds to the
 * event loop with its entry still recorded, and whatever the page runs next arrives on a stack of
 * its own; a counter would report that stack as being inside a callback it never entered.
 *
 * So this is a thread local in the only sense this target has one: the record belongs to whichever
 * stack is running, and a stack that parks takes its record with it. [whileParked] is that handoff,
 * and `PromisingStack.parked` is the single place a stack parks, so every suspension in the binding
 * passes through it.
 *
 * A list rather than a count, because the question is asked per gate. A close asks whether the
 * running stack is inside *its* callback, which is what decides whether it may wait for that body
 * or is standing on it; and a body may legally reach native code that dispatches another family's
 * callback before it returns.
 */
internal object RunningCallbacks {
  private var entered = mutableListOf<CallbackThreadState>()

  fun enter(state: CallbackThreadState) {
    entered.add(state)
  }

  fun exit(state: CallbackThreadState) {
    // The innermost entry, because a re-entered gate is recorded once per entry and the one leaving
    // is the one this stack entered last.
    val innermost = entered.lastIndexOf(state)
    if (innermost >= 0) entered.removeAt(innermost)
  }

  fun isInside(state: CallbackThreadState): Boolean = entered.contains(state)

  /**
   * Runs [body], which parks the calling stack, with that stack's callbacks set aside.
   *
   * Whatever runs while this stack is away is a stack of its own, inside the callbacks it entered
   * itself and no others. Restoring on the way back is what makes the resumed body count as being
   * inside its callback again for the rest of its life.
   */
  fun <T> whileParked(body: () -> T): T {
    val away = entered
    entered = mutableListOf()
    try {
      return body()
    } finally {
      entered = away
    }
  }
}

/**
 * Tracks whether the caller is inside one of this binding's callbacks.
 *
 * The state lives in [RunningCallbacks] rather than in this instance, because on this target the
 * answer belongs to a stack and an instance cannot be one. What is per-instance is the question: a
 * gate asks about its own callbacks, which is what a close needs in order to tell a body it is
 * standing on -- and must not wait for -- from one that is suspended on another stack, which it
 * must wait for and can.
 */
internal actual class CallbackThreadState actual constructor() {
  actual fun enter() {
    RunningCallbacks.enter(this)
  }

  actual fun exit() {
    RunningCallbacks.exit(this)
  }

  actual fun isInCallback(): Boolean = RunningCallbacks.isInside(this)
}
