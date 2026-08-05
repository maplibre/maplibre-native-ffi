package org.maplibre.nativeffi

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.internal.wasm.Dispatcher
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.WebglContext
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
 * The accepted shutdown is not tested here. It is final and it ends the owner thread, so a suite
 * that performed one would leave every later test with no module to call and no canvas to render on
 * -- a page gives control of a canvas away once. It has a run of its own instead, in
 * [ShutdownMaplibreFinalBrowserTest]. What is tested here is the refusal, which is the half a host
 * can be wrong about, and the bookkeeping the refusal reads.
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

  /**
   * The refusal for a handle no runtime stands behind.
   *
   * A projection is a snapshot of a map's transform that owns nothing else, so it retains neither
   * the map it came from nor the runtime behind that. Both can therefore be closed while it is
   * live, and a shutdown that inferred a projection from the runtime holding it open would find
   * nothing to refuse for -- and stop the one thread that may destroy it.
   */
  @Test
  fun shutdownIsRefusedWhileAProjectionOutlivesTheMapItWasTakenFrom(): Promise<JsAny?> =
    browserTest {
      val alreadyOpen = Dispatcher.openHandles.size

      // withMap closes the map and its runtime on the way out, so what escapes the scope is the
      // projection alone, with nothing else of this test's still counted.
      val projection = maplibreScope { withMap { _, map -> map.createProjection() } }

      val failure = assertFailsWith<InvalidStateException> { shutdownMaplibre() }
      assertTrue(
        failure.message.orEmpty().contains("MapProjectionHandle"),
        "the failure says \"${failure.message}\", which does not name the handle that is open",
      )

      maplibreScope { projection.close() }
      assertEquals(
        alreadyOpen,
        Dispatcher.openHandles.size,
        "a destroyed projection still holds the module open, so nothing could ever shut it down",
      )
    }

  /**
   * The refusal for a session that has given its retentions back.
   *
   * Detaching releases the map and the WebGL context, which is what lets a host tear a map down in
   * that order, and it leaves the session live and still needing to be destroyed on the owner
   * thread. So at this moment nothing else is holding the module open on the session's account, and
   * a shutdown that read only the retention graph would be accepted and strand it.
   */
  @Test
  fun shutdownIsRefusedWhileADetachedSessionOutlivesItsMap(): Promise<JsAny?> = browserTest {
    val alreadyOpen = Dispatcher.openHandles.size

    val session = maplibreScope {
      withMap(WIDTH, HEIGHT) { _, map ->
        val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
        val session =
          map.attachOpenGLOwnedTexture(
            OpenGLOwnedTextureDescriptor(
              RenderTargetExtent(WIDTH, HEIGHT, 1.0),
              context.descriptor(),
            )
          )
        // The context is closed here rather than left for the end, because a context still open
        // would be counted itself and would refuse the shutdown below whatever the session did.
        session.detach()
        context.close()
        session
      }
    }

    val failure = assertFailsWith<InvalidStateException> { shutdownMaplibre() }
    assertTrue(
      failure.message.orEmpty().contains("RenderSessionHandle"),
      "the failure says \"${failure.message}\", which does not name the handle that is open",
    )

    maplibreScope { session.close() }
    assertEquals(
      alreadyOpen,
      Dispatcher.openHandles.size,
      "a destroyed session still holds the module open, so nothing could ever shut it down",
    )
  }

  private companion object {
    const val WIDTH = 64
    const val HEIGHT = 32
  }
}
