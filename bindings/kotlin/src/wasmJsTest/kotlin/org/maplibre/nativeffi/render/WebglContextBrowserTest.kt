package org.maplibre.nativeffi.render

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import kotlin.test.fail
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.withMap

/**
 * Contexts to open while waiting for the module's allocator to hand a freed handle back.
 *
 * Bounded because a page holds only so many WebGL contexts, and the test asserts the refusal either
 * way -- reuse sharpens it into the ABA case rather than being what it rests on.
 */
private const val REUSE_ATTEMPTS = 8

/**
 * The lifetime of a WebGL context, which on this target the binding owns rather than the host.
 *
 * Everywhere else a render target's graphics context belongs to a host that made it with EGL,
 * Metal, or Vulkan, and keeping it valid for the target's borrow window is the host's job. Here the
 * context comes from this module, so keeping it valid is this binding's job — and the Emscripten
 * handle a target attaches by stays a positive integer long after the context behind it is gone,
 * and is handed to the next context created. So the checks below are the binding's own, and they
 * are what stands between a host and a render target working in a context that was destroyed
 * underneath it, or in one it never named.
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
              // rather than only at close. That is the order a host tearing a map down takes: the
              // detached session is live for its own destroy and nothing else, so releasing the
              // context it no longer touches must not wait for that destroy.
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
          error.diagnostic.contains("has been closed"),
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

  /**
   * The same refusal once another context has been given the closed one's handle.
   *
   * Emscripten allocates a context handle with `malloc` and frees it on destroy, so the number is
   * not an identity: destroy a context and the next one created takes the same number back. A
   * binding that resolved a descriptor by number would find the *new* context, retain it, and
   * render the map through a context the host never named — silently, because every check native
   * can make still passes.
   *
   * So this closes a context and opens others until the number comes back. The allocator is under
   * no obligation to return it, and what it returns depends on everything else this page has
   * allocated, so the refusal is asserted either way; reuse is what sharpens it into the case the
   * test above cannot reach.
   */
  @Test
  fun attachingWithAStaleDescriptorIsRefusedAfterItsHandleIsReused(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap(WIDTH, HEIGHT) { _, map ->
          val original = WebglContext.createOffscreen(WIDTH, HEIGHT)
          val stale = original.descriptor()
          original.close()

          // Reuse is what makes this the ABA case rather than merely a closed-context case, but the
          // module's allocator is under no obligation to hand the number straight back: what it
          // returns depends on whatever else has been allocated in this page. So the number is
          // hunted for rather than assumed, and every context opened on the way is kept open, since
          // closing one would free the very number being waited for.
          val opened = mutableListOf<WebglContext>()
          var replacement = WebglContext.createOffscreen(WIDTH, HEIGHT)
          opened.add(replacement)
          while (
            replacement.descriptor().context != stale.context && opened.size < REUSE_ATTEMPTS
          ) {
            replacement = WebglContext.createOffscreen(WIDTH, HEIGHT)
            opened.add(replacement)
          }
          val reused = replacement.descriptor().context == stale.context
          try {

            // Caught rather than left to assertFailsWith. A session that attached holds the map
            // open, so it would fail the map's own close on the way out, and that cleanup failure
            // is what the report would show instead of this one. Released first, reported second.
            val leaked =
              try {
                map.attachOpenGLOwnedTexture(
                  OpenGLOwnedTextureDescriptor(RenderTargetExtent(WIDTH, HEIGHT, 1.0), stale)
                )
              } catch (error: InvalidArgumentException) {
                assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
                assertTrue(
                  error.diagnostic.contains("has been closed"),
                  "the refusal reported ${error.diagnostic}",
                )
                null
              }
            if (leaked != null) {
              leaked.close()
              fail(
                "attaching with a descriptor from a closed context succeeded" +
                  if (reused) {
                    "; the handle ${stale.context} it carries now belongs to a different context, " +
                      "so the session would have rendered through one the host never named"
                  } else {
                    ", so a descriptor outliving its context is not refused at all"
                  }
              )
            }

            // And the context that really holds that number still attaches, so what was refused was
            // the stale descriptor rather than the handle it happens to carry. Only meaningful once
            // the number has actually been reused; otherwise this is an ordinary live context.
            if (reused) map.attachOpenGLOwnedTexture(descriptorFor(replacement)).close()
          } finally {
            opened.forEach { it.close() }
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

  /**
   * A readback whose pixel count no 32-bit pointer could address.
   *
   * The scratch a readback stages into is sized from two extents the caller chose, and the product
   * of two positive `Int`s is not one. Twenty-five by 42,949,673 is a gibibyte of pixels, and it
   * wraps to four bytes: the module would hand back a four-byte block, native would be told the
   * real extents, and what stopped it from reading past that block would be native's own extent cap
   * rather than anything this binding did. Refused here instead, before the allocator is asked.
   */
  @Test
  fun aReadbackTooLargeToAddressIsRefusedBeforeTheModuleIsAsked(): Promise<JsAny?> = browserTest {
    maplibreScope {
      val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
      try {
        val error =
          assertFailsWith<InvalidArgumentException> {
            context.readPixels(DEFAULT_FRAMEBUFFER, WRAPPING_WIDTH, WRAPPING_HEIGHT)
          }
        assertTrue(
          error.diagnostic.contains("pixels"),
          "the refusal did not name the pixel count: ${error.diagnostic}",
        )
        // And the context is untouched by the refusal, so a host that got the extent wrong once
        // still has the context it was reading from.
        assertFalse(context.isClosed)
        context.readPixels(DEFAULT_FRAMEBUFFER, WIDTH, HEIGHT)
      } finally {
        context.close()
      }
    }
  }

  private fun descriptorFor(context: WebglContext) =
    OpenGLOwnedTextureDescriptor(RenderTargetExtent(WIDTH, HEIGHT, 1.0), context.descriptor())

  private companion object {
    const val WIDTH = 64
    const val HEIGHT = 32

    /** Framebuffer zero, which is the canvas's own. */
    const val DEFAULT_FRAMEBUFFER = 0

    // 25 * 42_949_673 is 2^30 + 1 pixels, so four bytes each wraps an Int product to exactly four.
    // Both extents are positive and each fits an Int on its own, which is what makes the product
    // the only place this can be caught.
    const val WRAPPING_WIDTH = 25
    const val WRAPPING_HEIGHT = 42_949_673
  }
}
