package org.maplibre.nativeffi

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/**
 * What a shutdown is allowed to do to the handles a host still holds.
 *
 * The module owns the thread its runtimes live on, and only that thread may destroy what it
 * created. So a shutdown is the one call in this binding that can lose a live handle rather than
 * fail: the handle stays an ordinary Kotlin object with an ordinary `close`, and the thread that
 * `close` needed is gone.
 *
 * The accepted shutdown is not tested here, and cannot be. It is final and it ends the owner
 * thread, so a suite that performed one would leave every later test with no module to call and no
 * canvas to render on -- a page gives control of a canvas away once. What is tested is the refusal,
 * which is the half a host can be wrong about, and the bookkeeping the refusal reads.
 */
class ShutdownMaplibreBrowserTest {
  @Test
  fun shutdownIsRefusedWhileAHandleOutlivesTheScopeThatMadeIt(): Promise<JsAny?> = browserTest {
    // Whatever earlier tests left open, so this measures its own handle rather than the page's.
    val alreadyOpen = Dispatcher.openHandles.size

    // The mistake this exists for: a handle that escaped the scope that created it. Nothing about
    // it afterwards says which thread may destroy it, so a host that tears down in the order it
    // wrote its code in reaches the shutdown with this still live.
    val runtime = maplibreScope { RuntimeHandle.create(RuntimeOptions()) }

    val failure = assertFailsWith<InvalidStateException> { shutdownMaplibre() }

    // Named, rather than reported as a bare refusal, because the host's next move is to find and
    // close it, and the module is the only thing that knows what "it" is.
    assertTrue(
      failure.message.orEmpty().contains("RuntimeHandle"),
      "the failure says \"${failure.message}\", which does not name the handle that is open",
    )

    // A refused shutdown stops nothing, so the escaped handle is still closable on the thread that
    // created it. That is the whole point of refusing: this call is what would report the C API's
    // wrong-thread status against a thread the host never asked for, if the shutdown it followed
    // had been accepted.
    maplibreScope { runtime.close() }

    // And the module is open again for a shutdown, which is otherwise unobservable: bookkeeping
    // that counted a handle in and never out would refuse every shutdown for the rest of the page,
    // and no accepted shutdown can be performed here to find that out.
    assertEquals(
      alreadyOpen,
      Dispatcher.openHandles.size,
      "a destroyed runtime still holds the module open, so nothing could ever shut it down",
    )
  }
}
