package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import org.maplibre.nativeffi.internal.status.Status

@JsFun("(ids) => globalThis.__maplibreNativeC._mln_browser_dispatcher_create_with_canvases(ids)")
private external fun createDispatcher(canvasIds: Int): Int

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
  (d, out, diagnostic, capacity) => {
    const module = globalThis.__maplibreNativeC
    return module._mln_browser_dispatcher_take_completion(
      d, out, out + 4, diagnostic, capacity)
  }
"""
)
private external fun takeCompletion(
  dispatcher: Int,
  out: Int,
  diagnostic: Int,
  capacity: Int,
): Boolean

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

  /**
   * Diagnostics the drain has collected, by the token of the call that produced each.
   *
   * A message reaches the page only here, in the completion, because the C API's own is
   * thread-local to the owner thread and the next call there replaces it. It waits in this map for
   * the caller that is parked on the token to resume and take it; a call that left no message puts
   * nothing here at all.
   */
  private val diagnostics = mutableMapOf<Int, String>()

  /**
   * Page canvases to hand the owner thread as it starts, in the order they were reserved.
   *
   * A browser moves a canvas between agents only at `pthread_create`, so this list is read once and
   * never again. It is what [reserveCanvas] fills.
   */
  private val reservedCanvases = mutableListOf<String>()

  /**
   * Claims a page canvas for the owner thread, before that thread exists.
   *
   * A `<canvas>` element with this `id` is transferred to the owner thread when the thread is
   * created, and from then on the page's element is a placeholder that displays what the owner
   * thread draws. That is the whole of zero-copy presentation in a browser: a render target's
   * default framebuffer *is* the canvas the page shows, so a frame reaches the page without being
   * read back, copied, or passed through JavaScript.
   *
   * **Reserve before the first call that reaches native.** The owner thread starts lazily, on the
   * first call placed on it, and a browser will not transfer a canvas to a thread that is already
   * running — so this reports a failure rather than silently reserving something that can never
   * arrive. The element must be in the document by then too, and control of it must not already
   * have been transferred; either of those makes creating the thread fail instead.
   *
   * Reserving the same id twice does nothing, so a host may reserve on a path it takes more than
   * once.
   */
  fun reserveCanvas(id: String) {
    Status.requireArgument(id.isNotEmpty()) { "a canvas id must not be empty" }
    // The list crosses to native as one comma-separated string, which is the shape Emscripten's
    // transfer attribute takes, so a comma inside an id would split it into two that name nothing.
    Status.requireArgument(!id.contains(',')) { "a canvas id must not contain a comma: $id" }
    Heap.requireCString(id, "canvas id")
    if (reservedCanvases.contains(id)) return
    if (handle != 0) {
      throw Status.invalidState(
        "The canvas \"$id\" cannot be reserved because the MapLibre Native browser module's owner " +
          "thread has already started. A browser hands a canvas to a thread only as that thread " +
          "is created, so every canvas is reserved before the first call that reaches native."
      )
    }
    reservedCanvases.add(id)
  }

  /** Creates the owner thread on first use, transferring whatever canvases were reserved. */
  private fun require(): Int {
    if (handle != 0) return handle
    BrowserModule.require()
    val ids = reservedCanvases.joinToString(",")
    val created =
      if (ids.isEmpty()) createDispatcher(0)
      else
        Heap.withScratch(Heap.utf8Size(ids)) { block ->
          Heap.storeUtf8(block, ids)
          createDispatcher(block.address)
        }
    if (created == 0) {
      throw Status.invalidState(
        if (ids.isEmpty()) "The MapLibre Native browser module could not start its owner thread"
        else
          "The MapLibre Native browser module could not start its owner thread with the canvases " +
            "$ids. Each must be the id of a <canvas> element in the document whose control has " +
            "not already been transferred."
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
      val invoked = awaitCall(token)
      // Taken whatever the outcome, so a message never outlives the call it belongs to, and
      // published before the status is read: [read] is where the status becomes an exception, and
      // the diagnostic that exception copies has to be this call's.
      NativeDiagnostics.setProxiedDiagnostic(diagnostics.remove(token) ?: "")
      if (invoked == 0) {
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
    Heap.withScratch(COMPLETION_BYTES + DIAGNOSTIC_BYTES) { out ->
      val diagnostic = out + COMPLETION_BYTES
      while (takeCompletion(dispatcher, out.address, diagnostic.address, DIAGNOSTIC_BYTES)) {
        val token = Heap.loadInt(out)
        // Only a failing call leaves one, so the common case stores nothing. Recorded before the
        // token is resolved for order rather than for safety: the parked caller resumes on a
        // microtask, which cannot run while this loop holds the stack.
        val message = Heap.loadUtf8(diagnostic)
        if (message.isNotEmpty()) diagnostics[token] = message
        resolveCall(token, Heap.loadInt(out + 4))
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
    // Nothing is outstanding by now, so anything still here belongs to a caller that never
    // resumed; it would otherwise be handed to whichever token matched it after a restart.
    diagnostics.clear()
    // The canvases went with the thread and cannot come back: a page gives control of a canvas
    // away once, and the element it gave away is not drawable again. Carrying the reservations
    // into a restart would only make that restart fail, so a host that starts over supplies fresh
    // elements.
    reservedCanvases.clear()
    stopDispatcher(dispatcher)
  }

  private const val SLOT_BYTES = 8
  private const val COMPLETION_BYTES = 8
  // What the module copies for a failure, terminator included; see MLN_BROWSER_DIAGNOSTIC_CAPACITY
  // in src/browser/dispatcher.c. The capacity travels with the call rather than being agreed on, so
  // this bounds only what this binding is willing to receive: the module truncates to whatever it
  // is given, on a UTF-8 boundary.
  private const val DIAGNOSTIC_BYTES = 512
  // Tokens must be unique among outstanding calls. What guarantees that is the module-wide
  // suspension gate: one scope runs at a time, so one dispatched call is outstanding at a time.
  // The wrap is only to keep the counter from growing without bound -- it is not what makes a
  // token unique, and would not be enough on its own if more than one call could be in flight.
  private const val TOKEN_WRAP = 1 shl 20
}
