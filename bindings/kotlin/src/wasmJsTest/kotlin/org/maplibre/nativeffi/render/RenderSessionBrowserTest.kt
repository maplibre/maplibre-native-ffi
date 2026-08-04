package org.maplibre.nativeffi.render

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.BACKGROUND_STYLE_JSON
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.withMap

/**
 * A live render session on the browser's only backend.
 *
 * A WebGL context belongs to the thread that created it, and the thread that renders is the one the
 * module owns, so the context comes from [WebglContext] rather than from the page. Everything else
 * is the common render-session API: the session is attached to a map, renders on the owner thread,
 * and hands frames back either as an explicit frame handle or through CPU readback.
 */
class RenderSessionBrowserTest {
  // Spec coverage: BND-161, BND-162, BND-163, BND-164, BND-165, BND-166, BND-167, BND-168,
  // BND-170, BND-173, BND-175, BND-176.

  @Test
  fun aDescriptorMaterializesAnExtentAndABorrowedContextItDoesNotOwn(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap(WIDTH, HEIGHT) { _, map ->
          val context = WebglContext.create(WIDTH, HEIGHT)
          try {
            val descriptor = context.descriptor()
            assertNotEquals(0, descriptor.context)
            // A fresh descriptor each time, so a caller that mutates one cannot redirect another.
            assertNotEquals(descriptor, context.descriptor())

            val extent = RenderTargetExtent(WIDTH, HEIGHT, 2.0)
            val physical = extent.physicalSize()
            assertEquals(WIDTH * 2, physical.width)
            assertEquals(HEIGHT * 2, physical.height)

            val session =
              map.attachOpenGLOwnedTexture(OpenGLOwnedTextureDescriptor(extent, descriptor))
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
    }

  @Test
  fun eachAttachFamilyProducesTheSameSessionShape(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap(WIDTH, HEIGHT) { _, map ->
        val context = WebglContext.create(WIDTH, HEIGHT)
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
  }

  @Test
  fun renderUpdateReportsNoUpdateWithoutClosingTheSession(): Promise<JsAny?> = browserTest {
    maplibreScope {
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
  }

  @Test
  fun resizeAndSetTargetChangeTheExtentTheSessionRendersAt(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap(WIDTH, HEIGHT) { runtime, map ->
        val context = WebglContext.create(WIDTH, HEIGHT)
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
  }

  @Test
  fun readbackCopiesMetadataAndRefusesABufferTooSmallToHoldTheImage(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
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
    }

  @Test
  fun anOwnedTextureFrameExposesItsBackendHandleOnlyWhileItIsActive(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
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
            // The frame values report through the frame's own scope, which the release closed.
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
    }

  @Test
  fun closingAMapWithASessionAttachedIsRefusedUntilTheSessionGoes(): Promise<JsAny?> = browserTest {
    maplibreScope {
      org.maplibre.nativeffi.withRuntime { runtime ->
        val map =
          MapHandle.create(
            runtime,
            org.maplibre.nativeffi.map.MapOptions().apply {
              width = WIDTH
              height = HEIGHT
            },
          )
        val context = WebglContext.create(WIDTH, HEIGHT)
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
          assertEquals(
            "MapHandle has 1 live child handle(s): RenderSessionHandle",
            error.diagnostic,
          )
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
  }

  /** Runs [body] with a session on a context sized for it, closing both afterwards. */
  private fun <T> withSession(map: MapHandle, body: (WebglContext, RenderSessionHandle) -> T): T {
    val context = WebglContext.create(WIDTH, HEIGHT)
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
