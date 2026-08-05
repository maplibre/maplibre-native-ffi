package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.internal.wasm.BrowserModule
import org.maplibre.nativeffi.internal.wasm.CallbackScope
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.generated.MlnAdapterLogRecord
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

@JsFun("(consume) => globalThis.__maplibreNativeC._mln_browser_log_install(consume)")
private external fun installQueue(consume: Int): Int

@JsFun("(mark) => globalThis.__maplibreNativeC._mln_browser_log_take_since(mark)")
private external fun takeRecord(mark: Long): Int

@JsFun("() => globalThis.__maplibreNativeC._mln_browser_log_mark()")
private external fun currentMark(): Long

// `uint64_t`, which the module exports as an i64 and so reaches JavaScript as a BigInt under
// `-sWASM_BIGINT`. Declared as Long for that reason: reading it as a Double would throw on the
// first drain, before any record had been delivered.
@JsFun("() => globalThis.__maplibreNativeC._mln_browser_log_take_dropped()")
private external fun takeDropped(): Long

@JsFun("(address) => globalThis.__maplibreNativeC._mln_adapter_log_record_destroy(address)")
private external fun destroyRecord(address: Int)

@JsFun("(address) => globalThis.__maplibreNativeC.UTF8ToString(address)")
private external fun readString(address: Int): String

@JsFun("(drain, delay) => globalThis.setTimeout(drain, delay)")
private external fun scheduleDrain(drain: () -> Unit, delayMillis: Int)

/**
 * One host callback's registration with the browser log queue.
 *
 * This holds only the Kotlin side. The queue is registered with native once, by [LogQueueDrain],
 * and stays registered while any callback is installed -- which is what makes replacement work.
 * `mln_adapter_log_record_listener` takes no user data while the adapter treats the state address
 * as registration identity, so a binding that re-registered per callback could not tell the adapter
 * anything had changed. Registering once sidesteps that: there is only ever one registration, and
 * which Kotlin callback receives its records is this binding's own business.
 */
internal class LogQueueBridge(private val callback: LogCallback) : AutoCloseable {
  private val gate = CallbackGate("LogCallback")

  /** Delivers [record] unless this registration has been replaced or cleared. */
  fun deliver(record: LogRecord) {
    val lease = gate.enter() ?: return
    try {
      // Contained: a callback failure must not stop the drain, and there is no native frame above
      // this to unwind into. Marked as callback scope so anything it calls that would dispatch to
      // the owner thread reports that rather than trapping on an illegal suspension.
      runCatching { CallbackScope.inside { callback.log(record) } }
    } finally {
      lease.close()
    }
  }

  fun checkCanClose() = gate.checkCanClose()

  override fun close() = gate.close()
}

/**
 * Drains the native log queue onto a browser task.
 *
 * Logging is process-global and has no runtime to pump, so nothing in the C API's model says when
 * this should run. It runs as its own task instead: draining inside a runtime pump would stop
 * logging exactly when a host most wants it -- during startup, and during teardown after the last
 * pump.
 *
 * A macrotask rather than a microtask, and bounded per turn, so a logging burst cannot starve
 * rendering. It never suspends and never takes the module-wide suspension gate, so it can run while
 * a `maplibreScope` is parked: the parked stack cannot resume until this task returns, which makes
 * any allocator scope opened here properly nested inside it.
 *
 * **Stated divergence.** [LogCallback.log] returns whether the callback consumed the record, but a
 * browser host cannot answer that: MapLibre needs the decision on the logging thread, before the
 * record has reached the page. The registration reports a fixed "not consumed", so native logging
 * behaves exactly as it would with no callback installed, and a Kotlin callback observes records
 * without being able to suppress them. Its return value is ignored. The fixed policy is stated on
 * [LogCallback.log] and on
 * [Maplibre.setLogCallback][org.maplibre.nativeffi.Maplibre.setLogCallback], so a host reads it
 * where it installs one, and it is recorded in the binding specification's browser divergences.
 */
internal object LogQueueDrain {
  // The native registration is made once and never withdrawn, because the adapter's listener takes
  // no user data and so a withdrawal cannot be attributed to the registration that is retiring.
  // Every attempt to reconstruct that ordering had a race. Registering for the module's lifetime
  // removes the question: native never sees a change, and which Kotlin callback receives a record
  // is decided when the record is delivered, where it can be decided exactly.
  private const val NOT_CONSUMED = 0
  private const val BATCH = 64

  private var installed = false
  private var running = false

  /**
   * How long the next turn waits, in milliseconds, and so how idle the drain currently is.
   *
   * Zero while records are arriving, and stepped up towards [MAX_IDLE_DELAY_MILLIS] by each turn
   * that finds nothing. This is what stands in for the wake-up the queue does not have. Records are
   * produced by native threads and a pthread cannot call the page, so the only way a page learns of
   * one is by asking; the question is not whether to poll but how often, and the answer cannot be
   * "as fast as the browser will schedule a task" for a page that may be logging nothing at all for
   * minutes at a time.
   *
   * A backoff rather than a stop, which is where this parts company with the dispatcher's
   * completion drain. That one stops on a count it owns: a completion exists only because the page
   * submitted a call, so the page knows exactly when one can no longer arrive. Nothing here is that
   * count. MapLibre logs from whichever thread reaches the condition -- a tile worker answering a
   * response long after the call that asked for it returned -- so a drain that stopped on an empty
   * turn would need a page-side event to restart it that no page-side event corresponds to, and the
   * records would sit in the ring until the host happened to make another call.
   *
   * A turn that finds nothing costs two calls into the module, each taking the queue's lock and
   * finding it empty, so the cap is chosen for the page's task queue rather than for the work: four
   * wake-ups a second is what an idle page does anyway, where a zero-delay chain is a task every
   * four milliseconds forever, competing with rendering and input for the whole session.
   *
   * Read by the test that says an idle drain stops running a task per turn, which is otherwise
   * unobservable from a page: a drain that is sleeping and one that is spinning both deliver the
   * same records.
   */
  var nextTurnDelayMillis: Int = 0
    private set

  // Records enqueued before this are not this callback's. Taken when a callback is installed and
  // compared inside the same lock that enqueues, so there is no window in which a record produced
  // while nobody was listening can be delivered to whoever listens next -- which flushing at the
  // right moments could only ever narrow, not close.
  private var mark = 0L

  // Where the current callback comes from. The registry already owns which one is installed --
  // it swaps on replacement without re-installing -- so holding a second copy here would go stale
  // the first time a host replaced its callback, and every record after that would reach a closed
  // bridge instead of the new one.
  private var registry: (() -> LogQueueBridge?)? = null

  /**
   * How many records the queue dropped because the host did not drain them.
   *
   * Reported as its own count rather than as a synthesized log record: a fabricated
   * `WARNING`/`GENERAL` would be indistinguishable from one MapLibre produced, and a host that
   * persists its telemetry would file this binding's delivery loss as native output.
   */
  var droppedRecords: Long = 0L
    private set

  /**
   * Begins a callback's era: registers with native on first use, takes a fresh mark, and starts the
   * drain.
   *
   * Called for **every** installation, not only the first. The native registration is for the
   * module's lifetime -- see the note on this object -- but the mark is not: it is what says which
   * records belong to the callback being installed, so a replacement that reused the previous mark
   * would inherit its predecessor's queued records.
   *
   * The mark is taken before the caller publishes the new callback, so a record enqueued during the
   * swap belongs to the callback being installed rather than to the one it replaces.
   *
   * Which callback receives records is [source]'s answer, read afresh on every record, so replacing
   * or clearing takes effect immediately and without telling native anything.
   */
  fun beginEra(source: () -> LogQueueBridge?): Int {
    BrowserModule.require()
    registry = source
    if (!installed) {
      val status = installQueue(NOT_CONSUMED)
      if (status != 0) return status
      installed = true
    }
    // Everything already queued belongs to a period this callback was not listening for.
    mark = currentMark()
    // A host that has just installed a callback is about to want records, so the ramp starts over
    // rather than inheriting whatever the previous era backed off to. A turn already scheduled
    // keeps its own delay -- there is no cancelling a browser timeout from here, and scheduling a
    // second turn would run two drains against one queue -- so the first turn of a new era arrives
    // no later than the last era's delay, and every turn after it is prompt.
    nextTurnDelayMillis = 0
    if (!running) {
      running = true
      scheduleDrain(::drainTurn, 0)
    }
    return 0
  }

  /**
   * Stops delivering to a Kotlin callback.
   *
   * Native stays registered, so the drain keeps running and keeps releasing records; they simply
   * reach no one. Unregistering would be the tidier thing and is not safe to do -- see the note on
   * this object.
   */
  fun clear(): Int {
    BrowserModule.require()
    // The drain stops on its next turn once it finds no callback. The ring is bounded and native
    // keeps evicting into its dropped count, so a stopped drain cannot grow memory.
    return 0
  }

  private fun drainTurn() {
    // A final shutdown discards the module, and this turn was already scheduled by then. Every call
    // below reaches the module, so a turn that ran afterwards would fail on a reference that is
    // gone -- in a page task, where there is no caller to report it to. The backlog goes with the
    // heap it lived in, so there is nothing left to drain either.
    if (!BrowserModule.isLoaded()) {
      running = false
      return
    }
    if (registry?.invoke() == null) {
      // Nothing is listening, so the task parks however busy native happens to be -- a condition
      // that also counted records would keep waking the page forever under a steady log rate. The
      // backlog is left where it is: the next install takes a mark past it, so it can neither be
      // delivered late nor grow beyond the bounded ring.
      droppedRecords += takeDropped()
      running = false
      return
    }
    // Rescheduled from a finally for the reason the dispatcher's drain is: `running` is set by an
    // install and cleared only by a turn, so a turn that threw between here and the tail would stop
    // log delivery for the life of the page, and `install`'s `if (!running)` would never start it
    // again. Nothing here strands a caller the way a stranded completion drain would -- the cost is
    // that the host stops hearing records -- but it is silent, and a page has no other way back.
    var delivered = 0
    try {
      var address = takeRecord(mark)
      while (address != 0) {
        deliver(HeapPointer(address))
        delivered++
        if (delivered == BATCH) break
        address = takeRecord(mark)
      }
      val dropped = takeDropped()
      if (dropped > 0L) droppedRecords += dropped
    } finally {
      // A turn that delivered anything is followed at once, because a queue that had one record has
      // every chance of having the next; a turn that found nothing steps the wait up. A turn that
      // threw is counted by whatever it managed to deliver first, so a failure that repeats backs
      // off with the idle case rather than becoming a task the page runs as fast as it can schedule
      // one. Either way a successor is scheduled, which is the property this finally exists for.
      nextTurnDelayMillis = if (delivered > 0) 0 else nextIdleDelay()
      scheduleDrain(::drainTurn, nextTurnDelayMillis)
    }
  }

  /**
   * Doubles the idle wait, from one millisecond up to the cap.
   *
   * Geometric rather than a single idle constant so that the wait tracks how quiet the queue has
   * actually been: a gap between two records in a burst is covered by a wait of a few milliseconds,
   * and only a page that has logged nothing for a quarter of a second reaches the cap. The browser
   * clamps a nested timeout to about four milliseconds anyway, so the first few steps cost no more
   * than the zero-delay chain they replace.
   */
  private fun nextIdleDelay(): Int =
    if (nextTurnDelayMillis == 0) FIRST_IDLE_DELAY_MILLIS
    else minOf(nextTurnDelayMillis * 2, MAX_IDLE_DELAY_MILLIS)

  private fun deliver(record: HeapPointer) {
    try {
      val bridge = registry?.invoke()
      if (bridge == null) {
        // Nothing is listening, so the record is released without being decoded: reading the
        // message and building a LogRecord for no one is the cost this avoids.
        return
      }
      // Copied before the record is released, which is the only window it is valid in.
      val decoded =
        LogRecord(
          LogSeverity.fromNative(MlnAdapterLogRecord.severity(record)),
          LogEvent.fromNative(MlnAdapterLogRecord.event(record)),
          MlnAdapterLogRecord.code(record),
          readString(MlnAdapterLogRecord.message(record).address),
        )
      bridge.deliver(decoded)
    } finally {
      destroyRecord(record.address)
    }
  }
}
