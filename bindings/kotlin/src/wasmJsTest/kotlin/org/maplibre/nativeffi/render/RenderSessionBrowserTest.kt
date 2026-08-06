package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.BACKGROUND_STYLE_JSON
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.withMap

/**
 * A live render session on the browser's only backend.
 *
 * A WebGL context belongs to the agent that created it, and no host agent can hand one to this
 * module, so the context comes from [WebglContext] rather than from a host. Everything else is the
 * common render-session API: the session is attached to a map, renders, and hands frames back
 * either as an explicit frame handle or through CPU readback.
 */
class RenderSessionBrowserTest {
  // Spec coverage: BND-161, BND-162, BND-163, BND-164, BND-165, BND-166, BND-167, BND-168,
  // BND-169, BND-170, BND-172, BND-173, BND-175, BND-176.

  @Test
  fun aDescriptorMaterializesAnExtentAndABorrowedContextItDoesNotOwn() {
    withMap(WIDTH, HEIGHT) { _, map ->
      val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
      try {
        val descriptor = context.descriptor()
        assertNotEquals(0, descriptor.context)
        // A fresh descriptor each time, so a caller that mutates one cannot redirect another.
        assertNotEquals(descriptor, context.descriptor())

        val extent = RenderTargetExtent(WIDTH, HEIGHT, 2.0)
        val physical = extent.physicalSize()
        assertEquals(WIDTH * 2, physical.width)
        assertEquals(HEIGHT * 2, physical.height)

        val session = map.attachOpenGLOwnedTexture(OpenGLOwnedTextureDescriptor(extent, descriptor))
        session.close()

        // The session borrowed the context and did not take it: it is still usable for the
        // next target after the session that held it has gone.
        val second =
          map.attachOpenGLOwnedTexture(
            OpenGLOwnedTextureDescriptor(
              RenderTargetExtent(WIDTH, HEIGHT, 1.0),
              context.descriptor(),
            )
          )
        second.close()
      } finally {
        context.close()
      }
    }
  }

  @Test
  fun eachAttachFamilyProducesTheSameSessionShape() {
    withMap(WIDTH, HEIGHT) { _, map ->
      val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
      try {
        // A session-owned texture target: the session allocates the texture and hands frames
        // back through its own accessors.
        val owned =
          map.attachOpenGLOwnedTexture(
            OpenGLOwnedTextureDescriptor(
              RenderTargetExtent(WIDTH, HEIGHT, 1.0),
              context.descriptor(),
            )
          )
        assertEquals(map, owned.map())
        assertFalse(owned.isClosed)

        // A second session on the same map is refused by native, and the refusal leaves the
        // first one alone.
        val second =
          assertFailsWith<InvalidStateException> {
            map.attachOpenGLOwnedTexture(
              OpenGLOwnedTextureDescriptor(
                RenderTargetExtent(WIDTH, HEIGHT, 1.0),
                context.descriptor(),
              )
            )
          }
        assertEquals(MaplibreStatus.INVALID_STATE, second.status)
        assertFalse(owned.isClosed)
        owned.close()
        assertTrue(owned.isClosed)

        // A surface target, which presents through the canvas the context is bound to. There is
        // no drawable to name, so the surface pointer is null.
        val surface =
          map.attachOpenGLSurface(
            OpenGLSurfaceDescriptor(
              RenderTargetExtent(WIDTH, HEIGHT, 1.0),
              context.descriptor(),
              NativePointer.NULL,
            )
          )
        assertEquals(map, surface.map())
        // A surface session has no texture of its own, so the texture accessors report that
        // rather than reading a target that is not there.
        assertFailsWith<UnsupportedFeatureException> { surface.textureImageInfo() }
        surface.close()
      } finally {
        context.close()
      }
    }
  }

  @Test
  fun renderUpdateReportsNoUpdateWithoutClosingTheSession() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      withSession(map) { context, session ->
        // A map with no style has nothing to draw. That is a false result rather than a failure,
        // and it is the C API's own answer rather than something the binding decided.
        assertFalse(session.renderUpdate())
        assertFalse(session.isClosed)

        // The session was not closed or spoiled by it: the same session renders once there is
        // something to render.
        map.setStyleJson(BACKGROUND_STYLE_JSON)
        assertTrue(renderOneFrame(runtime, session), "the session never rendered a frame")
        assertFalse(session.isClosed)
        session.textureImageInfo()
      }
    }
  }

  @Test
  fun resizeAndSetTargetChangeTheExtentTheSessionRendersAt() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
      try {
        val session =
          map.attachOpenGLOwnedTexture(
            OpenGLOwnedTextureDescriptor(
              RenderTargetExtent(WIDTH, HEIGHT, 1.0),
              context.descriptor(),
            )
          )
        try {
          map.setStyleJson(BACKGROUND_STYLE_JSON)
          assertTrue(renderOneFrame(runtime, session))
          assertEquals(WIDTH, session.textureImageInfo().width)

          session.resize(HALF_WIDTH, HALF_HEIGHT, 1.0)
          assertTrue(renderOneFrame(runtime, session))
          val resized = session.textureImageInfo()
          assertEquals(HALF_WIDTH, resized.width)
          assertEquals(HALF_HEIGHT, resized.height)
          assertEquals(HALF_WIDTH * 4, resized.stride)

          // A session-owned texture has no host-owned target to replace, so `set_target` for a
          // target kind this session does not have is refused rather than silently accepted.
          val mismatched =
            assertFailsWith<UnsupportedFeatureException> {
              session.setOpenGLSurfaceTarget(
                OpenGLSurfaceDescriptor(
                  RenderTargetExtent(WIDTH, HEIGHT, 1.0),
                  context.descriptor(),
                  NativePointer.NULL,
                )
              )
            }
          assertEquals(MaplibreStatus.UNSUPPORTED, mismatched.status)
          // Refused without disturbing the session, which still reports the extent it had.
          assertEquals(HALF_WIDTH, session.textureImageInfo().width)
        } finally {
          session.close()
        }

        // The host-owned half of the pair: a surface session takes a new extent through
        // `set_target`, which is the one thing a browser surface target can be given.
        val surface =
          map.attachOpenGLSurface(
            OpenGLSurfaceDescriptor(
              RenderTargetExtent(WIDTH, HEIGHT, 1.0),
              context.descriptor(),
              NativePointer.NULL,
            )
          )
        try {
          surface.setOpenGLSurfaceTarget(
            OpenGLSurfaceDescriptor(
              RenderTargetExtent(HALF_WIDTH, HALF_HEIGHT, 1.0),
              context.descriptor(),
              NativePointer.NULL,
            )
          )
          assertTrue(renderOneFrame(runtime, surface))
        } finally {
          surface.close()
        }
      } finally {
        context.close()
      }
    }
  }

  @Test
  fun readbackCopiesMetadataAndRefusesABufferTooSmallToHoldTheImage() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      withSession(map) { context, session ->
        map.setStyleJson(BACKGROUND_STYLE_JSON)
        assertTrue(renderOneFrame(runtime, session))

        val info = session.textureImageInfo()
        assertEquals(WIDTH, info.width)
        assertEquals(HEIGHT, info.height)
        assertEquals(WIDTH * 4, info.stride)
        assertEquals((WIDTH * 4).toLong() * HEIGHT, info.byteLength)

        // A buffer one byte short: the read fails and the caller still owns the buffer.
        NativeBuffer.allocate(info.byteLength - 1).use { small ->
          val error =
            assertFailsWith<InvalidArgumentException> { session.readPremultipliedRgba8(small) }
          assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
          assertEquals(info.byteLength - 1, small.byteLength())
        }

        // A buffer that fits receives the image, and the same buffer is reusable for the next
        // read rather than being consumed by the first.
        NativeBuffer.allocate(info.byteLength).use { buffer ->
          assertEquals(info, session.readPremultipliedRgba8(buffer))
          val first = buffer.toByteArray()
          assertEquals(info.byteLength.toInt(), first.size)
          assertTrue(first.any { it != ZERO }, "the readback was entirely zero")

          assertEquals(info, session.readPremultipliedRgba8(buffer))
          assertContentEquals(first, buffer.toByteArray())
        }
      }
    }
  }

  @Test
  fun anOwnedTextureFrameExposesItsBackendHandleOnlyWhileItIsActive() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      withSession(map) { context, session ->
        map.setStyleJson(BACKGROUND_STYLE_JSON)
        assertTrue(renderOneFrame(runtime, session))

        val frame = session.acquireOpenGLOwnedTextureFrame()
        val copied = frame.frame()
        assertEquals(WIDTH, copied.width())
        assertEquals(HEIGHT, copied.height())
        assertEquals(1.0, copied.scaleFactor())
        assertNotEquals(0, copied.texture())
        assertNotEquals(0, copied.target())
        assertFalse(frame.isClosed)

        // While a frame is active the session may not render, resize, be given a new target,
        // or hand out a second frame.
        assertFailsWith<InvalidStateException> { session.renderUpdate() }
        assertFailsWith<InvalidStateException> { session.resize(WIDTH, HEIGHT, 1.0) }
        assertFailsWith<InvalidStateException> { session.acquireOpenGLOwnedTextureFrame() }
        assertFailsWith<InvalidStateException> {
          session.setOpenGLBorrowedTextureTarget(
            OpenGLBorrowedTextureDescriptor(
              RenderTargetExtent(WIDTH, HEIGHT, 1.0),
              WIDTH,
              HEIGHT,
              context.descriptor(),
              copied.texture(),
              copied.target(),
            )
          )
        }

        frame.close()
        assertTrue(frame.isClosed)
        // Released twice is a no-op, and the backend handles are gone with it.
        frame.close()
        assertFailsWith<InvalidStateException> { frame.frame() }
        // The frame values report through the frame's own borrow, which the release ended.
        assertFailsWith<IllegalStateException> { copied.texture() }

        // The session is usable again, and a second frame is a handle of its own. The first
        // one stays closed even though the storage behind it has been reused.
        assertTrue(renderOneFrame(runtime, session))
        val next = session.acquireOpenGLOwnedTextureFrame()
        try {
          assertFailsWith<InvalidStateException> { frame.frame() }
          assertNotEquals(0, next.frame().texture())
        } finally {
          next.close()
        }
      }
    }
  }

  /**
   * A release native refuses leaves the frame exactly as it was, so the caller can ask again.
   *
   * The alternative is worse than the failure: a frame marked closed by a release that did not
   * happen is one native still holds, and the session it was borrowed from refuses to render,
   * resize, detach, or close for as long as that borrow stands — with nothing left that could give
   * it back. So the assertions here are about the state rather than the error. The frame is still
   * open, its backend handles still read, the session still refuses to render, and the retry both
   * succeeds and gives the session back.
   *
   * The refusal is injected, and injected in place of the call rather than after it: native still
   * holds the frame afterwards, which is what makes the retry below a real release rather than a
   * second attempt at one that already happened.
   */
  // Spec coverage: BND-169.
  @Test
  fun aFrameReleaseNativeRefusesLeavesTheFrameOpenForAnotherAttempt() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      withSession(map) { _, session ->
        map.setStyleJson(BACKGROUND_STYLE_JSON)
        assertTrue(renderOneFrame(runtime, session))

        val frame = session.acquireOpenGLOwnedTextureFrame()
        val texture = frame.frame().texture()
        try {
          InjectedFaults.failNextCall(
            "mln_opengl_owned_texture_release_frame",
            MaplibreStatus.INVALID_STATE,
            "render session has no frame acquired",
          )
          val error = assertFailsWith<InvalidStateException> { frame.close() }
          assertEquals(MaplibreStatus.INVALID_STATE, error.status)
          assertEquals("render session has no frame acquired", error.diagnostic)
        } finally {
          InjectedFaults.reset()
        }

        // Nothing was retired: the handle is open, its frame still reads, and the session still
        // counts the borrow the release did not end.
        assertFalse(frame.isClosed, "the frame was retired by a release that did not happen")
        assertEquals(texture, frame.frame().texture())
        assertFailsWith<InvalidStateException> { session.renderUpdate() }

        // The retry is the release that never happened, so it closes the frame and hands the
        // session back.
        frame.close()
        assertTrue(frame.isClosed)
        assertFailsWith<InvalidStateException> { frame.frame() }
        assertTrue(renderOneFrame(runtime, session))
      }
    }
  }

  /**
   * A frame native handed over and the page could not wrap goes back, rather than being stranded.
   *
   * The window is the one between a successful acquire and the handle the caller is given: the
   * descriptor is copied into a Kotlin value and that value wrapped, both of which are object
   * construction and so both of which fail when there is no memory to construct into. A page that
   * only ended its own borrow there would leave native holding a frame with nothing left that could
   * release it — and a session with a frame acquired refuses to render, resize, detach, and close,
   * so the map would be lost for the life of the page.
   *
   * So the assertions are about what the session can do afterwards rather than about the error. The
   * failure is injected because a page cannot be made to run out of Kotlin heap on request, and it
   * is injected before the wrap rather than in place of the acquire, which is the point: native
   * really has a frame at that moment, so the release that follows is a real one.
   */
  // Spec coverage: BND-172.
  @Test
  fun aFrameTheWrapperCouldNotBeBuiltForIsGivenBackToNative() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      withSession(map) { _, session ->
        map.setStyleJson(BACKGROUND_STYLE_JSON)
        assertTrue(renderOneFrame(runtime, session))

        try {
          InjectedFaults.failNextFrameWrap()
          val error =
            assertFailsWith<InvalidStateException> { session.acquireOpenGLOwnedTextureFrame() }
          assertEquals(MaplibreStatus.INVALID_STATE, error.status)
        } finally {
          InjectedFaults.reset()
        }

        // Each of these is refused while a frame is acquired, so all three together say the
        // frame went back: the binding's own borrow ended, and native's did too. The resize
        // comes first because it retires whatever generation was rendered, which is what the
        // acquire below is asking for.
        session.resize(WIDTH, HEIGHT, 1.0)
        assertTrue(renderOneFrame(runtime, session))
        val frame = session.acquireOpenGLOwnedTextureFrame()
        try {
          assertNotEquals(0, frame.frame().texture())
        } finally {
          frame.close()
        }
      }
    }
  }

  @Test
  fun closingAMapWithASessionAttachedIsRefusedUntilTheSessionGoes() {
    org.maplibre.nativeffi.withRuntime { runtime ->
      val map =
        MapHandle.create(
          runtime,
          org.maplibre.nativeffi.map.MapOptions().apply {
            width = WIDTH
            height = HEIGHT
          },
        )
      val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
      try {
        val session =
          map.attachOpenGLOwnedTexture(
            OpenGLOwnedTextureDescriptor(
              RenderTargetExtent(WIDTH, HEIGHT, 1.0),
              context.descriptor(),
            )
          )
        val error = assertFailsWith<InvalidStateException> { map.close() }
        assertEquals(MaplibreStatus.INVALID_STATE, error.status)
        assertEquals("MapHandle has 1 live child handle(s): RenderSessionHandle", error.diagnostic)
        assertFalse(map.isClosed)

        session.close()
        map.close()
        assertTrue(map.isClosed)
      } finally {
        context.close()
        if (!map.isClosed) map.close()
      }
    }
  }

  /** Runs [body] with a session on a context sized for it, closing both afterwards. */
  private fun <T> withSession(map: MapHandle, body: (WebglContext, RenderSessionHandle) -> T): T {
    val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
    try {
      val session =
        map.attachOpenGLOwnedTexture(
          OpenGLOwnedTextureDescriptor(RenderTargetExtent(WIDTH, HEIGHT, 1.0), context.descriptor())
        )
      try {
        return body(context, session)
      } finally {
        session.close()
      }
    } finally {
      context.close()
    }
  }

  /**
   * Renders until the session reports a frame.
   *
   * The first render has nothing to draw yet: the style is still parsing on a MapLibre worker, and
   * the map only becomes renderable once the update it produced has been pumped through.
   */
  private fun renderOneFrame(runtime: RuntimeHandle, session: RenderSessionHandle): Boolean {
    repeat(ATTEMPTS) {
      if (session.renderUpdate()) return true
      runtime.pump(PUMP_MILLIS)
      while (runtime.pollEvent() != null) {}
    }
    return false
  }

  private companion object {
    const val WIDTH = 64
    const val HEIGHT = 32
    const val HALF_WIDTH = 32
    const val HALF_HEIGHT = 16
    const val ATTEMPTS = 200
    const val PUMP_MILLIS = 2L
    const val ZERO: Byte = 0
  }
}
