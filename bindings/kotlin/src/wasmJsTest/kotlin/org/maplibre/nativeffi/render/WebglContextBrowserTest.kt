package org.maplibre.nativeffi.render

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.withMap

/**
 * The lifetime of a WebGL context, which on this target the binding owns rather than the host.
 *
 * Everywhere else a render target's graphics context belongs to a host that made it with EGL,
 * Metal, or Vulkan, and keeping it valid for the target's borrow window is the host's job. Here the
 * context comes from this module, so keeping it valid is this binding's job — and the descriptor a
 * target attaches with carries only the Emscripten handle, which stays a positive integer long
 * after the context behind it is gone. So the checks below are the binding's own, and they are what
 * stands between a host and a render target working in a context that was destroyed underneath it.
 */
class WebglContextBrowserTest {
  // Spec coverage: BND-041, BND-042.

  @Test
  fun closingAContextUnderALiveRenderTargetIsRefusedUntilTheTargetGoes(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap(WIDTH, HEIGHT) { _, map ->
          val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
          try {
            val session = map.attachOpenGLOwnedTexture(descriptorFor(context))
            try {
              // The backend makes this context current on every frame and again while it releases
              // the GL objects the session built, so destroying it here would leave native working
              // in a context that is gone — and the destructor that would find out swallows it.
              val error = assertFailsWith<InvalidStateException> { context.close() }
              assertEquals(MaplibreStatus.INVALID_STATE, error.status)
              assertEquals(
                "WebglContext has 1 live child handle(s): RenderSessionHandle",
                error.diagnostic,
              )
              assertFalse(context.isClosed)

              // A refused close leaves a working context rather than a half-released one: this
              // does real GL work on the owner thread, in the context the session renders in.
              val texture = context.createTexture(WIDTH, HEIGHT)
              assertNotEquals(0, texture)
              context.destroyTexture(texture)

              // Detaching is what releases the backend, so the context becomes closeable there
              // rather than only at close — which is what a host that keeps the session handle to
              // attach a new target with depends on.
              session.detach()
              context.close()
              assertTrue(context.isClosed)
            } finally {
              // Closed here rather than at the end, so an assertion that fails above still leaves
              // the map closeable and the failure this test reports is its own.
              session.close()
            }
          } finally {
            if (!context.isClosed) context.close()
          }
        }
      }
    }

  @Test
  fun attachingWithADescriptorFromAClosedContextIsRefused(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap(WIDTH, HEIGHT) { _, map ->
        val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
        val stale = context.descriptor()
        context.close()

        // The handle in it is still positive, which is every check native makes and every check a
        // descriptor can make on its own. Only the binding knows the context it named is gone.
        assertTrue(stale.context > 0)
        val error =
          assertFailsWith<InvalidArgumentException> {
            map.attachOpenGLOwnedTexture(
              OpenGLOwnedTextureDescriptor(RenderTargetExtent(WIDTH, HEIGHT, 1.0), stale)
            )
          }
        assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
        assertTrue(
          error.diagnostic.contains("no open WebglContext"),
          "the refusal reported ${error.diagnostic}",
        )

        // The map is where it was, so a target attaches with a context that is open.
        val replacement = WebglContext.createOffscreen(WIDTH, HEIGHT)
        try {
          map.attachOpenGLOwnedTexture(descriptorFor(replacement)).close()
        } finally {
          replacement.close()
        }
      }
    }
  }

  @Test
  fun aContextCloseTheModuleRefusesLeavesItOpenForARetry(): Promise<JsAny?> = browserTest {
    val context = maplibreScope { WebglContext.createOffscreen(WIDTH, HEIGHT) }

    // Destroying a context is owner-thread work, and outside a scope there is no promising stack to
    // park it on, so the module refuses the submission. A browser host has no finalizer and cannot
    // restart a process, so a wrapper that marked itself closed here would strand the native
    // context for the page's whole life with nothing left able to name it.
    val error = assertFailsWith<InvalidStateException> { context.close() }
    assertTrue(
      error.diagnostic.contains("maplibreScope"),
      "the refusal reported ${error.diagnostic}",
    )
    assertFalse(context.isClosed)

    maplibreScope {
      // Still a context that works, and still one that closes, which is what restoring the live
      // state is for.
      context.destroyTexture(context.createTexture(WIDTH, HEIGHT))
      context.close()
    }
    assertTrue(context.isClosed)
  }

  private fun descriptorFor(context: WebglContext) =
    OpenGLOwnedTextureDescriptor(RenderTargetExtent(WIDTH, HEIGHT, 1.0), context.descriptor())

  private companion object {
    const val WIDTH = 64
    const val HEIGHT = 32
  }
}
