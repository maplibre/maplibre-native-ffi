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
   * Whether the owner thread has been stopped, which it can be only once.
   *
   * Stopping is final rather than a state a later call quietly starts over from. A handle is
   * destroyable only on the thread that created it, so a thread started after a stop could not
   * finish anything the previous one began: a lazy restart would take a handle the host still held,
   * destroy it somewhere it never belonged, and report the C API's wrong-thread status for a thread
   * the host was never told about. Refusing here is what turns every such use into one binding
   * error that names the shutdown, and it costs a host nothing a browser was going to allow anyway
   * -- a page gives a canvas away once, so the thread that presented can never be replaced by
   * another that does.
   */
  private var stopped = false

  /**
   * Owner-affine handles this module has created and not yet destroyed, by wrapper type name.
   *
   * This is what makes the module's destroy-then-drain-then-stop contract checkable rather than
   * advisory. Only the owner thread may destroy what it created, so stopping it while any of these
   * is open loses that handle for the life of the page; the count says so at the moment the mistake
   * is made, while the thread is still there to close them on.
   *
   * A wrapper counts itself unless something already counted is *refused* while it lives. A map and
   * an offline operation retain their runtime as a child, and a runtime with a live child refuses
   * its own destroy, so the runtime's entry covers them for as long as they exist; an owned texture
   * frame is covered the same way by the session that acquired it, which refuses to close while a
   * frame is out. Nothing covers the other three. A projection is a standalone snapshot of a map's
   * transform and retains nothing, a render session gives up both its retentions at detach while
   * staying live and still needing destruction, and a WebGL context has no runtime behind it. Each
   * of those counts itself, from construction until native has destroyed it. Without that, a
   * shutdown that should have been refused is accepted, and the handle it stranded reaches the
   * terminal failure instead -- which tells a host the same thing far too late to act on it.
   *
   * A plain list, because a page is one thread and every handle is created and released on it.
   */
  private val liveHandles = mutableListOf<String>()

  /**
   * What a shutdown would refuse to stop for.
   *
   * Read by the tests that say a closed handle stops holding the module open. Those run in the
   * shared suite, where the other way to observe it -- shutting down and seeing it accepted -- is
   * unavailable: a shutdown is final, and the page it happened on has no map in it again. The
   * accepted shutdown is covered by a run of its own instead; see the browser final-shutdown task.
   */
  val openHandles: List<String>
    get() = liveHandles.toList()

  /**
   * Calls that have been counted but whose answers have not been taken.
   *
   * This is what the drain's lifetime is decided by. A caller counts itself here before it submits,
   * so the count already covers the window in which a caller could park with nothing submitted yet
   * -- which is the window an empty-turn drain would stop in.
   *
   * A plain counter, with no synchronization, because everything that touches it runs on the page's
   * single agent. The owner thread reaches this binding only through the completion ring.
   */
  private var outstanding = 0

  /**
   * Whether a drain turn is scheduled.
   *
   * Read by the test that says the drain stops once nothing is outstanding, which is otherwise
   * unobservable from a page: an idle drain and a stopped one both do nothing visible.
   */
  val isDraining: Boolean
    get() = draining

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
    if (stopped) {
      throw Status.invalidState(
        "The canvas \"$id\" cannot be reserved because the MapLibre Native browser module has been " +
          "shut down. A reservation is only ever read as the owner thread starts, and that thread " +
          "cannot start again."
      )
    }
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
    requireDispatchable(name)
    val dispatcher = require()
    val entry = NativeCall.index(name)
    val bytes = (slotCount + 1) * SLOT_BYTES
    return Heap.withScratch(bytes) { scratch ->
      val result = scratch + slotCount * SLOT_BYTES
      fill(NativeCall.Slots(scratch))
      val token = nextToken++
      if (nextToken == TOKEN_WRAP) nextToken = 1
      beginWait(token)
      outstanding++
      try {
        startDraining()
        if (!submitCall(dispatcher, entry, scratch.address, slotCount, result.address, token)) {
          resolveCall(token, 0)
          park(token)
          throw Status.invalidState(
            "The MapLibre Native browser module refused a call to $name; too many calls are " +
              "already outstanding, or its owner thread is stopping."
          )
        }
        val invoked = park(token)
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
      } finally {
        outstanding--
      }
    }
  }

  /**
   * Refuses a call this module cannot place, before anything native is reached.
   *
   * A shut-down module is asked about first, because it is the one refusal that has nothing to do
   * with the calling stack: the owner thread is gone whoever asks and from wherever. This is the
   * error a host sees for a handle that outlived the shutdown, and it is deliberately the whole
   * story -- the alternative is starting a second thread that has never seen that handle, which
   * answers with the C API's wrong-thread status and leaves the handle destroyable nowhere.
   *
   * The two below are stacks that cannot park, and both would otherwise trap inside [awaitCall]
   * rather than report anything a host can act on.
   *
   * A callback scope is set only for the two families a MapLibre worker waits inside: the resource
   * provider and the URL transform. That worker is blocked in the module's synchronous proxy until
   * this page answers, so a call placed on the owner thread from here would be a page waiting for a
   * thread that is waiting for the page. The binding specification already requires those callbacks
   * to hand owner-thread work back rather than calling these APIs, so this reports it rather than
   * trying to serve it. It is checked first because it is the more specific of the two.
   *
   * A second call in flight is not itself the problem, and is ordinary now: a tile notification is
   * delivered on a promising stack of its own and may call the owner thread while a host scope is
   * parked. Only a blocked worker makes it a deadlock.
   *
   * Anything else outside a `maplibreScope` is a host that left the scope out, which is the easy
   * mistake to make: every other target's actuals are ordinary synchronous functions, so nothing in
   * shared host code carries a wrapper here.
   */
  private fun requireDispatchable(name: String) {
    if (stopped) {
      throw Status.invalidState(
        "$name cannot be called because the MapLibre Native browser module has been shut down. " +
          "The thread that owned its runtimes has been stopped, and a handle it created can only " +
          "ever be destroyed there, so this module starts no replacement for it."
      )
    }
    if (CallbackScope.isInside()) {
      throw Status.invalidState(
        "$name cannot be called from inside a MapLibre callback. Hand the work back to the " +
          "thread that owns the runtime and call it there."
      )
    }
    if (!PromisingStack.isInside()) {
      throw Status.invalidState(
        "$name must be called inside maplibreScope { }. This binding reaches the thread that " +
          "owns its runtimes by parking the calling stack on a promise, and WebAssembly allows " +
          "that only on a stack that maplibreScope established."
      )
    }
  }

  /**
   * Waits for [token]'s answer, with this stack away from the page while it waits.
   *
   * [awaitCall] unwinds to the event loop, so for as long as it has not returned, this stack is not
   * the one running. Surrendering the promising count says exactly that, which is what makes a call
   * placed from a page task that ran meanwhile report the missing scope rather than trap.
   */
  private fun park(token: Int): Int = PromisingStack.parked { awaitCall(token) }

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
    requireDispatchable(name)
    val dispatcher = require()
    val token = nextToken++
    if (nextToken == TOKEN_WRAP) nextToken = 1
    beginWait(token)
    outstanding++
    try {
      startDraining()
      if (!submit(dispatcher, token)) {
        resolveCall(token, 0)
        park(token)
        throw Status.invalidState(
          "The MapLibre Native browser module refused a call to $name; too many calls are " +
            "already outstanding, or its owner thread is stopping."
        )
      }
      // A task has no index and no slot count, so nothing about it can be rejected the way a table
      // call can be; the completion is only what says the owner thread has finished writing.
      park(token)
    } finally {
      outstanding--
    }
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
    // Stopped on the count rather than on an empty turn. A caller registers its wait and counts
    // itself before its call is submitted, so an empty turn says nothing about whether a park is
    // still coming -- a drain that stopped on one could leave a caller with nothing left to wake
    // it. The count closes that window, and stopping matters because the alternative is a
    // zero-delay task rescheduling itself for as long as the page is open, burning a browser task
    // per turn on a map that is doing nothing.
    if (outstanding == 0) {
      draining = false
      return
    }
    scheduleDrain(::drainTurn)
  }

  /** Counts a handle only the owner thread can destroy, so a stop can refuse while it is open. */
  fun retainHandle(typeName: String) {
    liveHandles.add(typeName)
  }

  /** Stops counting one, after native has destroyed it. */
  fun releaseHandle(typeName: String) {
    liveHandles.remove(typeName)
  }

  /**
   * Stops the owner thread, for good.
   *
   * The module's contract is destroy-then-drain-then-stop: a host closes its handles, lets their
   * calls complete, and only then stops. The first of those is checked here rather than assumed,
   * because it is the one a host can get wrong silently -- a handle that escaped its scope is still
   * an ordinary live object, and stopping the only thread that could destroy it loses it with no
   * complaint from anything. The second needs no check: `shutdownMaplibre` takes the same gate a
   * scope does, and a call is outstanding only while its scope holds that gate.
   *
   * Refusing leaves the thread running, so a host can close what it named and stop again.
   */
  fun stop() {
    if (liveHandles.isNotEmpty()) {
      throw Status.invalidState(
        "The MapLibre Native browser module cannot be shut down while ${liveHandles.size} handle(s) " +
          "created on its owner thread are still open: ${openHandleSummary()}. Only that thread " +
          "may destroy them, so stopping it now would lose them for the life of the page. Close " +
          "them inside a maplibreScope first."
      )
    }
    // Set whether or not a thread was ever started, because what this says is that the host is
    // finished with the module rather than that a particular thread has gone.
    stopped = true
    val dispatcher = handle
    if (dispatcher == 0) return
    handle = 0
    // `draining` is left as it is, and nothing will read it again: a turn may already be scheduled,
    // and the one thing that could set it is a call, which is refused from here on. The scheduled
    // turn clears the flag on the first turn that finds no dispatcher. `outstanding` and the
    // diagnostics are left for the same reason -- nothing can reach them, and scrubbing state that
    // no longer has a reader would only suggest it has one.
    //
    // The canvases stay in the reservation list for the same reason. They went with the thread and
    // cannot come back -- a page gives control of a canvas away once, and the element it gave away
    // is not drawable again -- and a host that reserves after this is refused before the list is
    // consulted at all.
    stopDispatcher(dispatcher)
  }

  /** Names the open handles the way a refused close names live children: sorted, with counts. */
  private fun openHandleSummary(): String =
    liveHandles
      .groupingBy { it }
      .eachCount()
      .entries
      .sortedBy { it.key }
      .joinToString(", ") { (name, count) -> if (count == 1) name else "$name x$count" }

  private const val SLOT_BYTES = 8
  private const val COMPLETION_BYTES = 8
  // What the module copies for a failure, terminator included; see MLN_BROWSER_DIAGNOSTIC_CAPACITY
  // in src/browser/dispatcher.c. The capacity travels with the call rather than being agreed on, so
  // this bounds only what this binding is willing to receive: the module truncates to whatever it
  // is given, on a UTF-8 boundary.
  private const val DIAGNOSTIC_BYTES = 512
  // Tokens must be unique among outstanding calls, and the counter alone is what makes them so:
  // every call takes the next value, and no two calls in flight can hold the same one. More than
  // one can now be in flight -- a tile notification is delivered on a promising stack of its own
  // and may call the owner thread while a host scope is parked -- so the counter is what this
  // rests on rather than one call at a time.
  //
  // The wrap keeps it from growing without bound. Reaching a token still outstanding would take
  // this many issuances between one call and its completion, which is not a number a page reaches
  // with at most a host scope and a delivery in flight.
  private const val TOKEN_WRAP = 1 shl 20
}
