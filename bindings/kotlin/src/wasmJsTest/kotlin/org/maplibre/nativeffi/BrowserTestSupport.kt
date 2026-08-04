package org.maplibre.nativeffi

import kotlin.coroutines.Continuation
import kotlin.coroutines.EmptyCoroutineContext
import kotlin.coroutines.startCoroutine
import kotlin.js.JsAny
import kotlin.js.Promise
import org.maplibre.nativeffi.internal.wasm.awaitOrThrow
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeEvent
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

// ---------------------------------------------------------------------------------------------
// Requirements from the binding specification's test table that this target does not have.
//
// Each is recorded here rather than dropped, so the gap between this suite and the table is a
// stated one. Everything else in the table is covered by a test marked `Spec coverage: BND-xxx`.
//
// **No second host thread.** A page is one thread, and every owner-affine call is placed on the
// module's thread by the binding rather than by the host. Host code therefore cannot make a call
// from the wrong thread, race a release against a use, or attach a render session from a thread
// that does not own the map:
// - BND-046 — concurrent releases: a page cannot release the same handle twice at once.
// - BND-049, BND-190, BND-191 — wrong-thread calls: the binding never makes one.
// - BND-145 — completing a handled request from another thread: there is no other thread.
// - BND-153 — a release waiting for an in-flight use: no use can be in flight from elsewhere.
// - BND-193, BND-194, BND-195, BND-196 — render sessions on a second thread.
// - BND-197 — a release racing a use of the same handle.
// - BND-174 — closing a map whose session was attached on another thread.
//
// **No cleanup outside explicit release.** Kotlin/Wasm has no finalization, no reference queue and
// no weak reference, so there is no non-deterministic cleanup to hang a leak report on:
// - BND-044 — cleanup hooks reporting leaked thread-affine handles.
// - BND-048 — best-effort cleanup failure reported through a leak channel.
//
// **Absent by design in this module.**
// - BND-158, BND-159 — outgoing HTTP header transforms. The browser's fetch transport follows
//   redirects itself, so a transformed header cannot be kept out of a cross-origin hop. Asserted
//   as permanently unsupported by `RuntimeHandleBrowserTest` instead.
// - BND-124 — custom geometry source callback teardown. MapLibre invokes those callbacks
//   synchronously on worker threads, which cannot enter the page's WebAssembly instance, so the
//   source cannot be created at all. Asserted as refused by `StyleBrowserTest` instead.
// - BND-156, BND-157 — queued provider routes. This binding does not route provider requests
//   through `mln_adapter_queued_resource_provider`.
// - BND-160's Metal and Vulkan halves — the browser module is built with OpenGL only. The
//   unsupported-backend errors those attach paths report are covered.
//
// **No seam for the injected failure.** The C API cannot be made to produce these here, and this
// binding has no internal hook that would:
// - BND-066's copy-failure half — an allocation failure after a native result handle is acquired.
//   The success half, which releases the same handles, is covered.
// - BND-122's failed-replacement half — a callback install that native refuses. The half that
//   preserves the previous callback is covered.
// - BND-169 — a native frame release that fails. Every reachable release succeeds.
// - BND-171 — caller-owned texture descriptors. A texture native can look up belongs to the
//   module's own context table, which page code cannot reach, so a host cannot supply one.
// - BND-172 — fallible owned-frame wrapper construction. Nothing between this binding's native
//   frame acquire and the handle it returns can fail.
//
// ---------------------------------------------------------------------------------------------

/**
 * Where the Karma harness serves the prelinked Emscripten module.
 *
 * Paired with the proxy in `karma.config.d/maplibre-browser-module.js`. The wasm and the ABI
 * manifest sit beside it, which is what the loader expects of any deployment.
 */
private const val MODULE_URL: String = "/maplibre/maplibre_native_c.mjs"

@JsFun("(message) => new Error(message)") private external fun jsError(message: String): JsAny

/**
 * Runs [block] as one test, with the browser module loaded.
 *
 * The browser binding's entry points are suspending, because loading a module and parking a stack
 * on native work are both promise-shaped. A test therefore returns the promise its body settles,
 * which is what the test framework waits on; a test that called the body and returned would report
 * success before the body had run.
 *
 * Loading is memoized by the loader itself, so every test names the module and the first one to
 * arrive is the one that fetches it.
 */
internal fun browserTest(block: suspend () -> Unit): Promise<JsAny?> = Promise { resolve, reject ->
  val body: suspend () -> Unit = {
    Maplibre.loadNativeLibraryAsync(MODULE_URL)
    block()
  }
  body.startCoroutine(
    Continuation(EmptyCoroutineContext) { result ->
      result.fold(
        onSuccess = { resolve(null) },
        // Carried across as a JavaScript error, which is the only failure shape a promise has.
        // The Kotlin type and stack are written into the message so the report still names the
        // line that failed.
        onFailure = { reject(jsError("$it\n${it.stackTraceToString()}")) },
      )
    }
  )
}

/**
 * Runs [task] on the page's next browser task.
 *
 * Used to observe the page while a scope is parked. A parked scope resumes from the event loop, so
 * a task that never ran would say the page had stopped servicing it.
 */
@JsFun("(task) => { globalThis.setTimeout(task, 0) }")
internal external fun runOnNextPageTask(task: () -> Unit)

/** Milliseconds on the page's clock, for measuring how long a parked call actually waited. */
@JsFun("() => Date.now()") internal external fun pageTimeMillis(): Double

/**
 * The origin the test page was served from.
 *
 * Used where a test needs a URL the module's HTTP transport can really fetch. The harness's own
 * origin is the only one available: the page is cross-origin isolated, so a request anywhere else
 * would be refused before it left the browser.
 */
@JsFun("() => globalThis.location.origin") internal external fun pageOrigin(): String

/** Yields the page for one browser task, so anything queued on it runs before this returns. */
internal suspend fun nextPageTask() {
  Promise<JsAny?> { resolve, _ -> runOnNextPageTask { resolve(null) } }.awaitOrThrow()
}

/** A style with a background layer and nothing else: no network, no tiles, no glyphs. */
internal const val BACKGROUND_STYLE_JSON: String =
  """{"version":8,"sources":{},"layers":[{"id":"background","type":"background"}]}"""

/** The smallest style MapLibre parses, for tests that only need a loaded style. */
internal const val EMPTY_STYLE_JSON: String = """{"version":8,"sources":{},"layers":[]}"""

/**
 * How long each wait pumps for, and how many times.
 *
 * A pump with a timeout parks the module's owner thread rather than the page, so the page keeps
 * servicing the event loop that resumes this stack. The product bounds a wait at a few seconds,
 * which is far longer than any style this suite loads takes.
 */
private const val PUMP_MILLIS = 2L
private const val PUMP_ATTEMPTS = 2_000

/** Runs [body] with a runtime, closing it afterwards. Owner-affine: call inside a scope. */
internal fun <T> withRuntime(
  options: RuntimeOptions = RuntimeOptions(),
  body: (RuntimeHandle) -> T,
): T {
  val runtime = RuntimeHandle.create(options)
  try {
    return body(runtime)
  } finally {
    runtime.close()
  }
}

/** Runs [body] with a runtime and a map of [width] by [height], closing both afterwards. */
internal fun <T> withMap(
  width: Int = 128,
  height: Int = 128,
  options: RuntimeOptions = RuntimeOptions(),
  body: (RuntimeHandle, MapHandle) -> T,
): T =
  withRuntime(options) { runtime ->
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          this.width = width
          this.height = height
        },
      )
    try {
      body(runtime, map)
    } finally {
      map.close()
    }
  }

/** Pumps until [predicate] holds, draining events into [onEvent] as they arrive. */
internal fun pumpUntil(
  runtime: RuntimeHandle,
  onEvent: (RuntimeEvent) -> Unit = {},
  predicate: () -> Boolean,
): Boolean {
  repeat(PUMP_ATTEMPTS) {
    if (predicate()) return true
    runtime.pump(PUMP_MILLIS)
    while (true) {
      val event = runtime.pollEvent() ?: break
      onEvent(event)
    }
  }
  return predicate()
}

/** Pumps until the map raises [type], returning the copied event. */
internal fun waitForMapEvent(
  runtime: RuntimeHandle,
  map: MapHandle,
  type: RuntimeEventType,
): RuntimeEvent {
  var found: RuntimeEvent? = null
  pumpUntil(runtime, onEvent = { if (it.type == type && it.mapSource == map) found = it }) {
    found != null
  }
  return found ?: error("the map raised no $type event")
}

/** Pumps until the runtime's queue is empty, so a later wait observes only new events. */
internal fun drain(runtime: RuntimeHandle) {
  repeat(64) {
    runtime.pump(PUMP_MILLIS)
    var drained = false
    while (runtime.pollEvent() != null) {
      drained = true
    }
    if (!drained) return
  }
}
