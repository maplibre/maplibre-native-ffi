package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.Status

@JsFun("() => globalThis.__maplibreNativeC._mln_browser_dispatcher_create()")
private external fun createDispatcher(): Int

@JsFun("(d) => globalThis.__maplibreNativeC._mln_browser_dispatcher_stop(d)")
private external fun stopDispatcher(dispatcher: Int)

@JsFun(
  "(d, index, slots, count, result, token) => " +
    "globalThis.__maplibreNativeC._mln_browser_dispatcher_submit(d, index, slots, count, result, token)"
)
private external fun submitCall(
  dispatcher: Int,
  index: Int,
  slots: Int,
  count: Int,
  result: Int,
  token: Int,
): Boolean

@JsFun(
  """
  (d, out) => {
    const module = globalThis.__maplibreNativeC
    return module._mln_browser_dispatcher_take_completion(d, out, out + 4)
  }
"""
)
private external fun takeCompletion(dispatcher: Int, out: Int): Boolean

/**
 * Registers the promise a caller will park on, before its call is submitted.
 *
 * Registered first because the answer can arrive before the caller reaches the suspension: the
 * worker may finish and the drain may resolve while this thread is still between submitting and
 * awaiting. Awaiting an already-resolved promise is fine; resolving one that does not exist yet is
 * not.
 */
@JsFun(
  """
  (token) => {
    const pending = (globalThis.__maplibreCalls ??= new Map())
    pending.set(token, null)
    pending.set(token, new Promise((resolve) => pending.set(token + 'r', resolve)))
  }
"""
)
private external fun beginWait(token: Int)

/**
 * Parks the calling stack until this call's answer arrives.
 *
 * A JSPI suspending import: it unwinds this Kotlin stack to the event loop and resumes it when the
 * promise settles, which is what lets the binding present the same synchronous API as every other
 * platform while the work happens on another thread. Legal only on a stack entered through
 * `maplibreScope`, which is what establishes the promising frame this suspension needs.
 */
@JsFun(
  """
  new WebAssembly.Suspending(async (token) => {
    const pending = globalThis.__maplibreCalls
    const promise = pending.get(token)
    try {
      return await promise
    } finally {
      pending.delete(token)
      pending.delete(token + 'r')
    }
  })
"""
)
private external fun awaitCall(token: Int): Int

@JsFun(
  """
  (token, ok) => {
    const pending = globalThis.__maplibreCalls
    const resolve = pending?.get(token + 'r')
    if (resolve) resolve(ok)
  }
"""
)
private external fun resolveCall(token: Int, ok: Int)

@JsFun("(drain) => globalThis.setTimeout(drain, 0)")
private external fun scheduleDrain(drain: () -> Unit)

/**
 * The thread that owns this binding's runtimes, and the calls placed on it.
 *
 * A browser host cannot own a thread, and MapLibre blocks, so owner-affine work runs on a pthread
 * the module owns. A caller submits, parks on a promise through [awaitCall], and resumes when the
 * completion for its token comes back — so the call looks synchronous while the page keeps
 * servicing its event loop, which is what the parked stack depends on to be resumed at all.
 *
 * There is one dispatcher for the whole module. The C API allows one runtime per owner thread, and
 * a second dispatcher would be a second owner thread with no way for a host to say which of them a
 * handle belonged to.
 */
internal object Dispatcher {
  private var handle = 0
  private var draining = false
  private var nextToken = 1

  /** Creates the owner thread on first use. */
  private fun require(): Int {
    if (handle != 0) return handle
    BrowserModule.require()
    val created = createDispatcher()
    if (created == 0) {
      throw Status.invalidState(
        "The MapLibre Native browser module could not start its owner thread"
      )
    }
    handle = created
    return handle
  }

  /**
   * Performs [name] on the owner thread and returns once its answer has come back.
   *
   * Ordinary and non-suspending as far as Kotlin is concerned; the suspension happens inside
   * [awaitCall], below this frame. The scratch outlives the call because this frame does: the
   * worker writes the result into it, and nothing here returns until it has.
   */
  fun <T> call(
    name: String,
    slotCount: Int,
    fill: (NativeCall.Slots) -> Unit,
    read: (HeapPointer) -> T,
  ): T {
    // A callback frame was entered from native and is not a promising stack, so parking here would
    // trap; and it runs while a scope may be parked, so dispatching would put a second call in
    // flight that the gate exists to prevent. The binding specification already requires callback
    // code to hand owner-thread work back rather than calling these APIs, so this reports that
    // rather than trying to serve it.
    if (CallbackScope.isInside()) {
      throw Status.invalidState(
        "$name cannot be called from inside a MapLibre callback. Hand the work back to the " +
          "thread that owns the runtime and call it there."
      )
    }
    val dispatcher = require()
    val entry = NativeCall.index(name)
    val bytes = (slotCount + 1) * SLOT_BYTES
    return Heap.withScratch(bytes) { scratch ->
      val result = scratch + slotCount * SLOT_BYTES
      fill(NativeCall.Slots(scratch))
      val token = nextToken++
      if (nextToken == TOKEN_WRAP) nextToken = 1
      beginWait(token)
      startDraining()
      if (!submitCall(dispatcher, entry, scratch.address, slotCount, result.address, token)) {
        resolveCall(token, 0)
        awaitCall(token)
        throw Status.invalidState(
          "The MapLibre Native browser module refused a call to $name; too many calls are " +
            "already outstanding, or its owner thread is stopping."
        )
      }
      if (awaitCall(token) == 0) {
        throw Status.invalidState(
          "The MapLibre Native browser module could not invoke $name with $slotCount slots."
        )
      }
      read(result)
    }
  }

  /**
   * Places one of the module's own entry points on the owner thread.
   *
   * [call] goes through the generated table, and that table carries the C API and nothing else. A
   * browser module entry point is not in it and still has to run where the runtime lives — a WebGL
   * context belongs to the thread that created it, and this is that thread. So [submit] is handed
   * the dispatcher and a token and calls the module export itself, while everything around it is
   * what [call] already does: one outstanding call, a promise parked on until the token comes back,
   * and the drain that resolves it.
   *
   * Whatever storage [submit] hands native is the caller's, and the rule [call] states applies to
   * it unchanged: the owner thread writes it, so nothing may read or release it until this returns.
   */
  fun submitTask(name: String, submit: (dispatcher: Int, token: Int) -> Boolean) {
    // Same reasoning as in [call]: a callback frame was entered from native, so it is not a
    // promising stack and cannot park, and it runs while a scope may already be parked.
    if (CallbackScope.isInside()) {
      throw Status.invalidState(
        "$name cannot be called from inside a MapLibre callback. Hand the work back to the " +
          "thread that owns the runtime and call it there."
      )
    }
    val dispatcher = require()
    val token = nextToken++
    if (nextToken == TOKEN_WRAP) nextToken = 1
    beginWait(token)
    startDraining()
    if (!submit(dispatcher, token)) {
      resolveCall(token, 0)
      awaitCall(token)
      throw Status.invalidState(
        "The MapLibre Native browser module refused a call to $name; too many calls are " +
          "already outstanding, or its owner thread is stopping."
      )
    }
    // A task has no index and no slot count, so nothing about it can be rejected the way a table
    // call can be; the completion is only what says the owner thread has finished writing.
    awaitCall(token)
  }

  private fun startDraining() {
    if (draining) return
    draining = true
    scheduleDrain(::drainTurn)
  }

  private fun drainTurn() {
    val dispatcher = handle
    if (dispatcher == 0) {
      draining = false
      return
    }
    Heap.withScratch(COMPLETION_BYTES) { out ->
      while (takeCompletion(dispatcher, out.address)) {
        resolveCall(Heap.loadInt(out), Heap.loadInt(out + 4))
      }
    }
    // Kept running for the dispatcher's life. A caller parks before its call is submitted, so a
    // drain that stopped on an empty turn could park a caller with nothing left to wake it.
    scheduleDrain(::drainTurn)
  }

  /**
   * Stops the owner thread.
   *
   * The module's contract is destroy-then-drain-then-stop: a host closes its handles, lets their
   * calls complete, and only then stops. Nothing here enforces that, because by this point the
   * handles are gone and there is nothing left to ask.
   */
  fun stop() {
    val dispatcher = handle
    if (dispatcher == 0) return
    handle = 0
    draining = false
    stopDispatcher(dispatcher)
  }

  private const val SLOT_BYTES = 8
  private const val COMPLETION_BYTES = 8
  // Tokens must be unique among outstanding calls. What guarantees that is the module-wide
  // suspension gate: one scope runs at a time, so one dispatched call is outstanding at a time.
  // The wrap is only to keep the counter from growing without bound -- it is not what makes a
  // token unique, and would not be enough on its own if more than one call could be in flight.
  private const val TOKEN_WRAP = 1 shl 20
}
