package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import kotlin.test.fail
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
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
  fun closingAContextUnderALiveRenderTargetIsRefusedUntilTheTargetGoes() {
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
          // does real GL work in the context the session renders in.
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

  @Test
  fun attachingWithADescriptorFromAClosedContextIsRefused() {
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
   *
   * Retargeting is asked the same question, at both entry points that take a context. It is the
   * half with no second line of defence: an attach at least reaches a session that has no context
   * yet, while `set_target` reaches one that does, and all native compares there is the handle — so
   * a stale descriptor whose number has come back matches the context the session is really
   * rendering in and is accepted.
   */
  @Test
  fun attachingWithAStaleDescriptorIsRefusedAfterItsHandleIsReused() {
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
      while (replacement.descriptor().context != stale.context && opened.size < REUSE_ATTEMPTS) {
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

        // The other way in. Each session below is attached with the live context and then
        // offered the stale descriptor for the same target kind, so the only thing wrong with
        // the retarget is the context it names.
        val texture = replacement.createTexture(WIDTH, HEIGHT)
        try {
          val borrowed =
            map.attachOpenGLBorrowedTexture(borrowedDescriptorFor(replacement, texture))
          try {
            assertStaleContextRefused(reused, stale.context, "a borrowed texture target") {
              borrowed.setOpenGLBorrowedTextureTarget(
                OpenGLBorrowedTextureDescriptor(
                  RenderTargetExtent(WIDTH, HEIGHT, 1.0),
                  WIDTH,
                  HEIGHT,
                  stale,
                  texture,
                  TEXTURE_2D,
                )
              )
            }

            // The same retarget with a descriptor from the context that is open goes through,
            // so what was refused was the descriptor and not the call.
            borrowed.setOpenGLBorrowedTextureTarget(borrowedDescriptorFor(replacement, texture))
          } finally {
            borrowed.close()
          }
        } finally {
          replacement.destroyTexture(texture)
        }

        val surface = map.attachOpenGLSurface(surfaceDescriptorFor(replacement))
        try {
          assertStaleContextRefused(reused, stale.context, "a surface target") {
            surface.setOpenGLSurfaceTarget(
              OpenGLSurfaceDescriptor(
                RenderTargetExtent(WIDTH, HEIGHT, 1.0),
                stale,
                NativePointer.NULL,
              )
            )
          }

          surface.setOpenGLSurfaceTarget(surfaceDescriptorFor(replacement))
        } finally {
          surface.close()
        }
      } finally {
        opened.forEach { it.close() }
      }
    }
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
  fun aReadbackTooLargeToAddressIsRefusedBeforeTheModuleIsAsked() {
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

  /**
   * Asserts that [retarget] refuses a descriptor whose context is gone, and says what it cost.
   *
   * The refusal has to be the binding's own. Native refuses a *mismatched* handle by itself, so a
   * retarget with a stale descriptor whose number was never reused already fails there — with a
   * message about the context this session attached with, which says nothing about the descriptor
   * having outlived what it named. Only the diagnostic tells the two apart, which is why this reads
   * it rather than settling for the exception type.
   */
  private fun assertStaleContextRefused(
    reused: Boolean,
    handle: Int,
    what: String,
    retarget: () -> Unit,
  ) {
    val refusal =
      try {
        retarget()
        null
      } catch (error: InvalidArgumentException) {
        error
      }
    if (refusal == null) {
      fail(
        "retargeting to $what with a descriptor from a closed context succeeded" +
          if (reused) {
            "; the handle $handle it carries now belongs to a different context, so the session " +
              "would have rendered through one the host never named"
          } else {
            ", so a descriptor outliving its context is not refused at retarget at all"
          }
      )
    }
    assertEquals(MaplibreStatus.INVALID_ARGUMENT, refusal.status)
    assertTrue(
      refusal.diagnostic.contains("has been closed"),
      "retargeting to $what reported ${refusal.diagnostic}, which is not the binding's own refusal",
    )
  }

  private fun descriptorFor(context: WebglContext) =
    OpenGLOwnedTextureDescriptor(RenderTargetExtent(WIDTH, HEIGHT, 1.0), context.descriptor())

  private fun borrowedDescriptorFor(context: WebglContext, texture: Int) =
    OpenGLBorrowedTextureDescriptor(
      RenderTargetExtent(WIDTH, HEIGHT, 1.0),
      WIDTH,
      HEIGHT,
      context.descriptor(),
      texture,
      TEXTURE_2D,
    )

  private fun surfaceDescriptorFor(context: WebglContext) =
    OpenGLSurfaceDescriptor(
      RenderTargetExtent(WIDTH, HEIGHT, 1.0),
      context.descriptor(),
      // A WebGL context is already bound to its canvas, so there is no drawable to name where every
      // other OpenGL provider names one, and native refuses anything else here.
      NativePointer.NULL,
    )

  private companion object {
    const val WIDTH = 64
    const val HEIGHT = 32

    // GL_TEXTURE_2D. The C API takes the GL enum unchanged, and this is the only target a render
    // target can be attached to.
    const val TEXTURE_2D = 3553

    /** Framebuffer zero, which is the canvas's own. */
    const val DEFAULT_FRAMEBUFFER = 0

    // 25 * 42_949_673 is 2^30 + 1 pixels, so four bytes each wraps an Int product to exactly four.
    // Both extents are positive and each fits an Int on its own, which is what makes the product
    // the only place this can be caught.
    const val WRAPPING_WIDTH = 25
    const val WRAPPING_HEIGHT = 42_949_673
  }
}
