package org.maplibre.nativeffi

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.internal.wasm.BrowserModule
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.render.NativeBuffer
import org.maplibre.nativeffi.render.WebglContext
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceRequestHandle
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/**
 * The shutdown a host is supposed to perform, and what the page is like afterwards.
 *
 * This is the only test in this suite that stops the module's owner thread, and stopping it is
 * final: the thread that owned every runtime is gone, the canvases that went with it cannot come
 * back, and every later call is refused. So it cannot share a page with anything, and it does not —
 * it has a browser run of its own, which is what the `wasmJsFinalShutdownTest` task exists for.
 * Everything else about a shutdown, including the refusals that keep a host from reaching this by
 * mistake, is in [ShutdownMaplibreBrowserTest] and runs with the rest of the suite.
 *
 * Without this run, `mln_browser_dispatcher_stop` is never called successfully by anything: not by
 * this suite, and not by the C tests, which join their dispatcher instead because they may block.
 * The detach, the final wake, the keepalive the owner thread drops and the free it does on the way
 * out would all be code no test had executed.
 *
 * It is also the only place the state a shutdown does *not* count can be observed after the module
 * has gone. A buffer, a resource request handle, and the process-global log callback are none of
 * them owner-affine, so each is still the host's when the module is released, and each reaches the
 * module by a route the handle registry does not describe.
 */
class ShutdownMaplibreFinalBrowserTest {
  @Test
  fun shutdownStopsTheOwnerThreadAndRefusesEverythingAfterwards(): Promise<JsAny?> = browserTest {
    // The three pieces of state a shutdown does not count as an open handle, each reaching the
    // module by a different route: page memory, a native object a provider still holds, and a
    // Kotlin reference that native never sees. Taken before the shutdown, so the assertions below
    // describe a page that really held all three when the module went.
    val buffer = NativeBuffer.allocate(BUFFER_BYTES)
    Maplibre.setLogCallback { false }
    var handled: ResourceRequestHandle? = null

    // Real owner-thread work first, so the thread this stops is one that was started, used, and
    // drained rather than one that never ran. A stop of an unused module would exercise none of
    // the drain-and-release path the thread takes on its way out.
    maplibreScope {
      withMap { runtime, map ->
        runtime.setResourceProvider { request, handle ->
          if (request.requestedUrl != STYLE_URL) {
            return@setResourceProvider ResourceProviderDecision.PASS_THROUGH
          }
          handled = handle
          ResourceProviderDecision.HANDLE
        }
        map.setStyleUrl(STYLE_URL)
        assertTrue(pumpUntil(runtime) { handled != null }, "the provider was never asked")

        // Abandoned so that native stops waiting for an answer, which is what lets the map and the
        // runtime close under a request the host still holds. The handle stays the host's, and its
        // native request is the object the release below would have reached.
        map.setStyleJson(EMPTY_STYLE_JSON)
        assertTrue(
          pumpUntil(runtime) { handled?.isCancelled() == true },
          "the abandoned request was never reported as cancelled",
        )
      }
    }
    val request = assertNotNull(handled, "no request handle survived the map that produced it")
    assertEquals(
      emptyList(),
      Dispatcher.openHandles,
      "a handle from this test's own map is still counted, so the shutdown below would be refused",
    )

    shutdownMaplibre()

    // The buffer's bytes went with the heap they lived in, and this is the binding saying so
    // rather than a JavaScript type error naming `HEAPU8`. Read before the close below, because a
    // closed buffer refuses for a different reason.
    assertFailsWith<InvalidStateException> { buffer.toByteArray() }

    // Closing the other two succeeds and does nothing, and the callback is gone without being
    // closed at all. Neither a buffer nor a request handle is one of the handles the shutdown
    // counts, so a host arrives here with them open by following the documented order rather than
    // by making a mistake. A close that threw would break the `finally` it belongs in, and would
    // throw after the close core had already marked the resource released.
    buffer.close()
    request.close()
    assertFalse(
      Maplibre.hasLogCallback(),
      "the log callback is still installed, so it and everything it closes over stay reachable " +
        "for the life of the document with no call left able to drop them",
    )

    // The refusal every owner-affine call reports from here on. It names the shutdown rather than
    // starting a replacement thread, because a thread started now has never seen the handles a
    // host still holds and could only answer them with the C API's wrong-thread status.
    val refused =
      assertFailsWith<InvalidStateException> {
        maplibreScope { RuntimeHandle.create(RuntimeOptions()) }
      }
    // Either wording is the right answer, and which one arrives depends only on what the call
    // touches first: a shutdown both stops the thread and releases the module, so a call that
    // allocates before it dispatches is turned away by the release and one that dispatches first by
    // the stop. What matters is that it is the binding saying so, rather than a JavaScript type
    // error from a module reference that is now null.
    assertTrue(
      refused.message.orEmpty().let { it.contains("shut down") || it.contains("released") },
      "the failure says \"${refused.message}\", which says neither that the module was shut down " +
        "nor that it was released",
    )

    // Reserving is refused by its own path rather than by the one above, because a reservation is
    // read only as the owner thread starts and never reaches a dispatched call at all.
    val reserveRefused =
      assertFailsWith<InvalidStateException> { WebglContext.reserveCanvas("mln-test-after-stop") }
    assertTrue(
      reserveRefused.message.orEmpty().let { it.contains("shut down") || it.contains("released") },
      "the failure says \"${reserveRefused.message}\", which says neither that the module was shut " +
        "down nor that it was released",
    )

    // Shutting down again is accepted and does nothing. A host tearing down along more than one
    // path should not have to remember which of them got there first, and there is no thread left
    // for a second stop to reach.
    shutdownMaplibre()

    // And the page is left idle. The drain reschedules itself for as long as a call is outstanding,
    // so a stop that left it running would burn a browser task per turn for the life of the page,
    // polling a dispatcher that is gone.
    nextPageTask()
    assertFalse(
      Dispatcher.isDraining,
      "the completion drain is still scheduling itself after the owner thread was stopped",
    )

    // The module itself is gone, not merely idle. Stopping the thread alone would leave a sixteen
    // worker pool and a 512 MiB heap reachable for the life of the document, which is most of what
    // a host that shuts down wanted back.
    assertFalse(
      BrowserModule.isLoaded(),
      "the module is still on the page after a shutdown, so its worker pool and heap were never " +
        "released",
    )

    // Loading again is refused rather than answered. Both wrong answers are worse than an error: a
    // memo resolved into a no-op would hand back a module that is gone, and a cleared memo would
    // instantiate a second sixteen-worker module beside handles the host still holds.
    val reloadRefused = assertFailsWith<InvalidStateException> { Maplibre.loadNativeLibraryAsync() }
    assertTrue(
      reloadRefused.message.orEmpty().contains("released"),
      "the failure says \"${reloadRefused.message}\", which does not say the module was released",
    )

    // A page task still runs afterwards. The log drain reaches the module directly and reschedules
    // itself, so a turn already queued when the module went would fail on a reference that is gone,
    // in a task with no caller to report it to.
    nextPageTask()
    nextPageTask()
  }

  private companion object {
    /** A url no transport serves, so the provider is the only thing that could answer it. */
    const val STYLE_URL = "custom://shutdown-style.json"

    /**
     * Non-empty, so the buffer holds a real allocation rather than the zero-length special case.
     */
    const val BUFFER_BYTES = 64L
  }
}
