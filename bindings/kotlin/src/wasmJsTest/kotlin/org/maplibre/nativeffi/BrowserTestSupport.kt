package org.maplibre.nativeffi

import kotlin.coroutines.Continuation
import kotlin.coroutines.EmptyCoroutineContext
import kotlin.coroutines.startCoroutine
import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.awaitOrThrow
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.render.WebglContext
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
    // Before the module is even loaded, because reserving a canvas has to happen before the owner
    // thread starts and that thread starts on the first call any test makes. Which test runs first
    // is the framework's choice, so every test does this and the first one to arrive is the one it
    // takes effect for.
    PageCanvases.reserveAll()
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

// ---------------------------------------------------------------------------------------------
// Presenting to the page, and looking at what arrived there.
//
// A render test that reads the texture back proves the map drew something. It does not prove the
// frame ever reached the page, and in a browser those are different questions with different
// answers: pixels reach a page only when a canvas the page displays is composited, and a canvas is
// composited only when the task that drew into it ends. So the tests below assert on the `<canvas>`
// *element* — through `drawImage` into a scratch two-dimensional canvas and `getImageData`, which
// is what any page code would see — and never through a readback, which would pass just as well
// against a canvas nothing displays.
// ---------------------------------------------------------------------------------------------

/**
 * Creates a `<canvas>` element with this id if the document has none, and reports its presence.
 *
 * Idempotent, because every test calls it and only the first one can matter: control of a canvas is
 * given away once, and a second element with the same id would be a different canvas the owner
 * thread never received.
 */
@JsFun(
  """
  (id, width, height) => {
    let canvas = document.getElementById(id)
    if (!canvas) {
      canvas = document.createElement('canvas')
      canvas.id = id
      canvas.width = width
      canvas.height = height
      document.body.appendChild(canvas)
    }
    return true
  }
"""
)
private external fun ensureCanvasElement(id: String, width: Int, height: Int): Boolean

/**
 * Copies what the page's canvas element currently shows into the module's heap, as RGBA8.
 *
 * Read the way page code would: the element is drawn into a scratch two-dimensional canvas and
 * sampled with `getImageData`. That is the whole point — it reaches the placeholder element rather
 * than the render target, so it can only see what the browser actually composited.
 *
 * `getImageData` returns top-down rows and un-premultiplied colour, where GL's own origin is the
 * bottom row. The styles these tests render fill the viewport with one colour, so neither
 * difference shows; a test that drew something asymmetric would have to account for both.
 */
@JsFun(
  """
  (id, width, height, address) => {
    const source = document.getElementById(id)
    if (!source) return false
    const scratch = document.createElement('canvas')
    scratch.width = width
    scratch.height = height
    const context = scratch.getContext('2d', { willReadFrequently: true })
    // Cleared first, so a canvas that never received a frame reads as transparent rather than as
    // whatever the scratch happened to hold.
    context.clearRect(0, 0, width, height)
    context.drawImage(source, 0, 0, width, height)
    const data = context.getImageData(0, 0, width, height).data
    globalThis.__maplibreNativeC.HEAPU8.set(data, address)
    return true
  }
"""
)
private external fun readCanvasElement(id: String, width: Int, height: Int, address: Int): Boolean

/**
 * Parks this stack until the page has had a chance to composite.
 *
 * A suspending import, for the same reason every dispatched call is one: this runs inside a
 * `maplibreScope`, where the stack may unwind to the event loop and be resumed, and it has to yield
 * the page rather than spin on it — a page that spins never composites anything.
 *
 * An animation frame is what a compositor drives, and a timeout is the fallback, because a headless
 * browser that decides the page is not being painted would otherwise never resume this at all.
 */
@JsFun(
  """
  new WebAssembly.Suspending(async () => {
    await Promise.race([
      new Promise((resolve) => globalThis.requestAnimationFrame(() => resolve(null))),
      new Promise((resolve) => globalThis.setTimeout(() => resolve(null), 20)),
    ])
    return 0
  })
"""
)
private external fun awaitPageFrame(): Int

/** The canvases this suite hands to the owner thread, one per test that presents to the page. */
internal object PageCanvases {
  /** The surface session's, which renders straight into this canvas's default framebuffer. */
  const val SURFACE: String = "mln-test-surface"

  /** The session-owned texture target's, which is blitted onto this canvas to present. */
  const val OWNED_TEXTURE: String = "mln-test-owned-texture"

  /** The caller-owned texture target's, which is blitted the same way. */
  const val BORROWED_TEXTURE: String = "mln-test-borrowed-texture"

  /** The custom geometry source test's, which presents tiles this page supplied. */
  const val CUSTOM_GEOMETRY: String = "mln-test-custom-geometry"

  /**
   * A valid HTML id that a CSS identifier cannot spell literally, for [HostileCanvasIdBrowserTest].
   *
   * An id may be anything without ASCII whitespace, and the module reaches a page canvas through
   * `document.querySelector`, so every character here is one that has to be escaped on the way or
   * the transfer selects the wrong element or none: a leading digit, a colon, a dot, and brackets.
   * Reserved alongside the others because a canvas reaches the owner thread only as that thread is
   * created.
   */
  const val HOSTILE: String = "9mln:test.hostile[canvas]"

  /**
   * The size every one of them is created at.
   *
   * Small, because a software rasteriser draws every pixel of every frame these tests render, and
   * an image that fills a viewport says everything a larger one would.
   */
  const val WIDTH: Int = 64
  const val HEIGHT: Int = 32

  /**
   * Puts each canvas in the document and claims it for the owner thread.
   *
   * One canvas per presenting test rather than one shared between them, which is also what a real
   * host does: a canvas belongs to the thing rendering onto it. It is not only tidiness — a
   * transferred canvas tolerates about two WebGL contexts over a page's lifetime, and a third
   * consumer of the same canvas makes destroying the runtime fail.
   */
  fun reserveAll() {
    for (id in listOf(SURFACE, OWNED_TEXTURE, BORROWED_TEXTURE, CUSTOM_GEOMETRY, HOSTILE)) {
      ensureCanvasElement(id, WIDTH, HEIGHT)
      WebglContext.reserveCanvas(id)
    }
  }
}

/**
 * Asserts that the page's canvas element comes to show one opaque colour.
 *
 * Presenting is not synchronous with the call that asks for it. The owner thread's task has to end
 * before the browser composites what it drew, and the page has to reach a frame before the element
 * it displays is updated — so this yields the page until the colour arrives, and only then makes
 * the assertion. Waiting for the frame to *appear* is not enough on its own: a canvas that already
 * holds a previous frame is opaque from the start, and a test that reads the first opaque image it
 * sees would keep asserting against the frame before the one it asked for.
 *
 * A colour that never arrives is not silently tolerated. The last image the page actually held is
 * what the assertion runs against, so the failure names the colour that was really there.
 */
internal fun assertPresentedColor(
  id: String,
  red: Int,
  green: Int,
  blue: Int,
  width: Int = PageCanvases.WIDTH,
  height: Int = PageCanvases.HEIGHT,
) {
  var pixels = ByteArray(0)
  repeat(PRESENT_ATTEMPTS) {
    awaitPageFrame()
    pixels = readPageCanvas(id, width, height)
    // One pixel decides whether to stop waiting, and the whole image is then checked below. A
    // partly arrived frame therefore fails on the pixels that are wrong rather than waiting for a
    // frame that is already as complete as it will get.
    if (isPresentedColor(pixels, 0, red, green, blue)) {
      assertUniformColor(pixels, red, green, blue, width, height, "presented")
      return
    }
  }
  assertUniformColor(pixels, red, green, blue, width, height, "presented")
}

/**
 * Asserts that a frame read back out of a render target is one opaque colour.
 *
 * Read back rather than presented, which is the point of having both: a page canvas showing the
 * wrong colour says nothing about whether the map drew the right one, and these two assertions
 * together say which half failed. The page assertion is the one that matters; this one is what
 * makes its failure diagnosable.
 *
 * Row zero is the bottom row here, where the presented image has it at the top, so a uniform colour
 * is all these two can be compared on directly.
 */
internal fun assertRenderedColor(
  pixels: ByteArray,
  red: Int,
  green: Int,
  blue: Int,
  width: Int = PageCanvases.WIDTH,
  height: Int = PageCanvases.HEIGHT,
) {
  assertUniformColor(pixels, red, green, blue, width, height, "rendered")
}

private fun isPresentedColor(
  pixels: ByteArray,
  offset: Int,
  red: Int,
  green: Int,
  blue: Int,
): Boolean =
  pixels.size > offset + 3 &&
    channelMatches(pixels, offset, red) &&
    channelMatches(pixels, offset + 1, green) &&
    channelMatches(pixels, offset + 2, blue) &&
    channelMatches(pixels, offset + 3, 255)

private fun channelMatches(pixels: ByteArray, offset: Int, expected: Int): Boolean =
  (pixels[offset].toInt() and 0xFF) in
    (expected - PRESENT_TOLERANCE)..(expected + PRESENT_TOLERANCE)

private fun readPageCanvas(id: String, width: Int, height: Int): ByteArray {
  val bytes = width * height * 4
  return Heap.withScratch(bytes) { pixels ->
    if (!readCanvasElement(id, width, height, pixels.address)) {
      error("the document has no canvas element with the id \"$id\"")
    }
    Heap.loadBytes(pixels, bytes)
  }
}

/**
 * Asserts every pixel of a presented image is one opaque colour.
 *
 * Checked as a whole image rather than a sample: these styles paint one background over the whole
 * viewport, so a frame that reached the page only in part shows up here where a single sample would
 * miss it. The tolerance is for the round trip through a float shader, an eight-bit target, a
 * compositor, and `getImageData`'s un-premultiplication.
 */
private fun assertUniformColor(
  pixels: ByteArray,
  red: Int,
  green: Int,
  blue: Int,
  width: Int,
  height: Int,
  where: String,
) {
  assertEquals(width * height * 4, pixels.size, "the $where image is the wrong size")
  assertTrue(
    pixels.any { it != 0.toByte() },
    "the $where image is entirely zero, so no frame ever reached it",
  )
  for (y in 0 until height) {
    for (x in 0 until width) {
      val offset = (y * width + x) * 4
      assertChannel(pixels, offset, red, "red", x, y, where)
      assertChannel(pixels, offset + 1, green, "green", x, y, where)
      assertChannel(pixels, offset + 2, blue, "blue", x, y, where)
      // Exactly opaque. A frame with the wrong alpha would darken or lighten the channels above
      // without changing which colour they name.
      assertChannel(pixels, offset + 3, 255, "alpha", x, y, where)
    }
  }
}

private fun assertChannel(
  pixels: ByteArray,
  offset: Int,
  expected: Int,
  channel: String,
  x: Int,
  y: Int,
  where: String,
) {
  val actual = pixels[offset].toInt() and 0xFF
  assertTrue(
    actual in (expected - PRESENT_TOLERANCE)..(expected + PRESENT_TOLERANCE),
    "the $where pixel ($x, $y) has $channel $actual, but the frame's is $expected",
  )
}

/** A style painting one opaque background colour and nothing else: no network, no tiles. */
internal fun backgroundStyle(color: String): String =
  """{"version":8,"sources":{},"layers":[""" +
    """{"id":"background","type":"background","paint":{"background-color":"$color"}}]}"""

/**
 * How many page frames a present is given to arrive.
 *
 * Each is an animation frame or twenty milliseconds, whichever comes first, so this bounds a wait
 * at a couple of seconds — far longer than a compositor takes, and short enough that a frame that
 * never arrives fails the run rather than hanging it.
 */
private const val PRESENT_ATTEMPTS = 100

private const val PRESENT_TOLERANCE = 3

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
