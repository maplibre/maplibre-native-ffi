package org.maplibre.nativeffi

import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Heap
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
// **No second thread this binding can reach.** Kotlin/Wasm runs on the Emscripten pthread the
// module's `main()` imported it into, and a Kotlin/Wasm module cannot create a thread of its own:
// there is no `pthread_create` it can call and no worker that would share its memory. Every call
// this binding makes into the C API is therefore made from the same native thread, and every
// callback body reaches it by being drained from the module's record ring on that same thread. So
// no test here can make a call from the wrong thread, race a release against a use, or hold a
// handle from a thread that does not own it:
// - BND-046 — concurrent releases.
// - BND-049, BND-190, BND-191 — wrong-thread calls.
// - BND-145 — completing a handled request from another thread.
// - BND-153 — a release waiting for an in-flight use from elsewhere.
// - BND-193, BND-194, BND-195, BND-196 — render sessions on a second thread.
// - BND-197 — a release racing a use of the same handle.
// - BND-174 — closing a map whose session was attached on another thread.
//
// **No cleanup outside explicit release.** Kotlin/Wasm has no finalization, no reference queue and
// no weak reference, so there is no non-deterministic cleanup to hang a leak report on:
// - BND-044 — cleanup hooks reporting leaked thread-affine handles.
// - BND-048 — best-effort cleanup failure reported through a leak channel.
//
// **Answered by a native rule table rather than by host code.** MapLibre raises a resource
// transform on worker threads, each of which is a separate JavaScript agent, so this binding
// registers `mln_adapter_resource_transform_rewrite_callback` and reports the host-callback form
// as unsupported, which is what the specification's `#### The browser` clause sanctions. No host
// code receives a transform request, so there is nothing for a copy to protect:
// - BND-141 — transform request data copied into language-owned values.
// BND-140 is covered by `ResourceProviderBrowserTest`, through the rule table.
//
// **Decided by route rather than by a callback return.** Requests reach host code through
// `mln_adapter_queued_resource_provider`, which claims a request by matching a route declared at
// registration. The specification states that BND-150 does not apply to such a binding, because
// there is no callback return path left to override.
//
// **Absent by design in this module.**
// - BND-158, BND-159 — outgoing HTTP header transforms. The browser's fetch transport follows
//   redirects itself, so a transformed header cannot be kept out of a cross-origin hop. Asserted
//   as permanently unsupported by `RuntimeHandleBrowserTest` instead.
// - BND-160's Metal and Vulkan halves — the browser module is built with OpenGL only. The
//   unsupported-backend errors those attach paths report are covered.
//
// **Injected through an internal seam.** The module will not produce these on request, so
// `InjectedFaults` produces them instead, which the binding specification's test-seam rules allow.
// Each of these tests asserts the state the failure left behind rather than only the error it
// reported:
// - BND-066's copy-failure half — `StyleBrowserTest` and `QueryBrowserTest` fail the copy after a
//   native list, snapshot, or result handle is acquired, and replay the handle to prove native
//   destroyed it.
// - BND-122's failed-replacement half — `ResourceProviderBrowserTest` has native refuse the
//   provider and rewrite-rule installs. The custom geometry family needs no seam: native refuses a
//   duplicate source id by itself, which `CustomGeometrySourceBrowserTest` uses.
// - BND-169 — `RenderSessionBrowserTest` has native refuse a frame release, and the frame stays
//   retryable.
// - BND-172 — `RenderSessionBrowserTest` fails the wrapper construction that follows a successful
//   frame acquire, and the session goes on rendering, resizing, and handing out frames, which it
//   could not do if the frame it acquired had been stranded.
// ---------------------------------------------------------------------------------------------

/**
 * The origin this module was served from.
 *
 * Used where a test needs a URL the module's HTTP transport can really fetch. This thread is a
 * worker of the host page, so its location is the module's own URL; the page is cross-origin
 * isolated, and a request anywhere else would be refused before it left the browser.
 */
@JsFun("() => globalThis.location.origin") internal external fun pageOrigin(): String

@JsFun("() => Date.now()") private external fun nowMillis(): Double

/** Reports how long [body] took, for the waits whose whole claim is that they ended early. */
internal fun elapsedMillis(body: () -> Unit): Long {
  val started = nowMillis()
  body()
  return (nowMillis() - started).toLong()
}

/**
 * Asserts that native no longer holds the snapshot, list, or result handle [handle] named.
 *
 * Asking native is the only way to tell a destroyed result handle from a leaked one. A leaked one
 * sits in the module's handle table doing nothing until the page is gone. A destroyed one names a
 * retired generation of its slot, and native answers that with an invalid argument saying so. So
 * the handle is replayed through [replay], which is any entry point taking a handle of this [kind]
 * and one output pointer.
 *
 * The replay is what a released handle costs a caller anyway, and the binding's own wrappers cannot
 * express it: a result handle never leaves the call that reads it.
 */
internal fun assertResultHandleDestroyed(handle: Long, kind: String, replay: (Long, Int) -> Int) {
  assertTrue(handle != 0L, "no native $kind was acquired, so nothing was there to destroy")
  val error =
    assertFailsWith<InvalidArgumentException>(
      "native still holds $kind $handle, so the failed copy leaked it"
    ) {
      Heap.withScratch(RESULT_HANDLE_OUT_BYTES) { out -> Status.check(replay(handle, out.address)) }
    }
  assertTrue(error.diagnostic.contains(kind), error.diagnostic)
  assertTrue(error.diagnostic.contains("stale"), error.diagnostic)
}

/** Room for the widest output any of the replayed entry points above writes. */
private const val RESULT_HANDLE_OUT_BYTES = 8

/**
 * The one `<canvas>` element of the host page this thread can render into.
 *
 * A canvas reaches a pthread by being transferred as that thread is created, and the module's link
 * names this element id in `-sOFFSCREENCANVASES_TO_PTHREAD`. So there is exactly one, it is the
 * page's own element rather than a private surface, and no test can make a second: a page hosting
 * more than one on-screen map is a documented limitation of this binding rather than something a
 * test could reach.
 *
 * The context is created once and never destroyed, because a WebGL context belongs to its canvas —
 * asking the same canvas for a second one hands back the first. That is also what a page host does:
 * one canvas, one context, for as long as the page lives.
 */
internal object PageCanvas {
  /**
   * The size the canvas is used at.
   *
   * Small, because a software rasteriser draws every pixel of every frame these tests render, and
   * an image that fills a viewport says everything a larger one would.
   */
  const val WIDTH: Int = 64
  const val HEIGHT: Int = 32

  private var shared: WebglContext? = null

  /** The context every presenting test renders through, sized back to [WIDTH] by [HEIGHT]. */
  fun context(): WebglContext {
    val context = shared ?: WebglContext.createForPageCanvas(WIDTH, HEIGHT).also { shared = it }
    context.resizeCanvas(WIDTH, HEIGHT)
    return context
  }
}

/**
 * Asserts that the page canvas's own drawing buffer holds one opaque colour.
 *
 * Read with `glReadPixels` against framebuffer zero, which for a transferred canvas is the element
 * the page displays rather than a surface of this binding's own. That is what makes this a claim
 * about presenting: a surface session draws straight into this framebuffer, and a texture session's
 * frame reaches it only by being blitted there.
 *
 * What it does not claim is that the browser composited what it read. Compositing happens when the
 * task that drew ends, and the page's `<canvas>` element can only be sampled from the page's own
 * agent, which this thread is not.
 */
internal fun assertPresentedColor(
  context: WebglContext,
  red: Int,
  green: Int,
  blue: Int,
  width: Int = PageCanvas.WIDTH,
  height: Int = PageCanvas.HEIGHT,
) {
  assertUniformColor(context.readPixels(0, width, height), red, green, blue, width, height, "page")
}

/**
 * Asserts that a frame read back out of a render target is one opaque colour.
 *
 * Read back rather than presented, which is the point of having both: a canvas showing the wrong
 * colour says nothing about whether the map drew the right one, and these two assertions together
 * say which half failed.
 */
internal fun assertRenderedColor(
  pixels: ByteArray,
  red: Int,
  green: Int,
  blue: Int,
  width: Int = PageCanvas.WIDTH,
  height: Int = PageCanvas.HEIGHT,
) {
  assertUniformColor(pixels, red, green, blue, width, height, "rendered")
}

/**
 * Asserts every pixel of an image is one opaque colour.
 *
 * Checked as a whole image rather than a sample: these styles paint one background over the whole
 * viewport, so a frame that arrived only in part shows up here where a single sample would miss it.
 * The tolerance is for the round trip through a float shader and an eight-bit target.
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
    actual in (expected - COLOR_TOLERANCE)..(expected + COLOR_TOLERANCE),
    "the $where pixel ($x, $y) has $channel $actual, but the frame's is $expected",
  )
}

private const val COLOR_TOLERANCE = 3

/** A style painting one opaque background colour and nothing else: no network, no tiles. */
internal fun backgroundStyle(color: String): String =
  """{"version":8,"sources":{},"layers":[""" +
    """{"id":"background","type":"background","paint":{"background-color":"$color"}}]}"""

/** A style with a background layer and nothing else: no network, no tiles, no glyphs. */
internal const val BACKGROUND_STYLE_JSON: String =
  """{"version":8,"sources":{},"layers":[{"id":"background","type":"background"}]}"""

/** The smallest style MapLibre parses, for tests that only need a loaded style. */
internal const val EMPTY_STYLE_JSON: String = """{"version":8,"sources":{},"layers":[]}"""

/**
 * How long each wait pumps for, and how many times.
 *
 * A pump blocks this thread on the runtime's own condition variable, which is legal here and is
 * what gives MapLibre's workers a chance to run. The product bounds a wait at a few seconds, far
 * longer than any style this suite loads takes.
 */
private const val PUMP_MILLIS = 2L
private const val PUMP_ATTEMPTS = 2_000

/**
 * Runs [body] with a runtime, closing it afterwards.
 *
 * The provider is cleared and the ring drained before the close, because a queued provider's
 * registration is only released when its retirement marker comes out of the ring — and a runtime
 * that is gone can no longer drain one. A registration left waiting for its marker is the module's
 * state rather than this runtime's, so it would be the *next* test that found its provider
 * unreachable.
 */
internal fun <T> withRuntime(
  options: RuntimeOptions = RuntimeOptions(),
  body: (RuntimeHandle) -> T,
): T {
  val runtime = RuntimeHandle.create(options)
  try {
    return body(runtime)
  } finally {
    if (!runtime.isClosed) {
      runCatching { runtime.clearResourceProvider() }
      runCatching { pumpTurns(runtime, RETIREMENT_PUMPS) }
    }
    runtime.close()
  }
}

/**
 * Enough turns for the retirement markers of anything a test registered to come out of the ring.
 */
private const val RETIREMENT_PUMPS = 8

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

/**
 * Pumps [turns] times without waiting for anything.
 *
 * Used where the claim is that nothing arrives. A record native produced reaches host code by being
 * drained inside a pump, so the runtime has to be pumped that many times before its absence means
 * anything.
 */
internal fun pumpTurns(runtime: RuntimeHandle, turns: Int) {
  repeat(turns) {
    runtime.pump(PUMP_MILLIS)
    while (runtime.pollEvent() != null) {}
  }
}
