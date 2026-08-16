package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotEquals
import kotlin.test.assertSame
import kotlin.test.assertTrue
import kotlinx.cinterop.BetaInteropApi
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ObjCObject
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.objcPtr
import kotlinx.cinterop.ptr
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.staticCFunction
import kotlinx.cinterop.toLong
import kotlinx.cinterop.usePinned
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.GeoJsonSourceDataHandle
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import platform.CoreGraphics.CGSizeMake
import platform.Metal.MTLCreateSystemDefaultDevice
import platform.Metal.MTLDeviceProtocol
import platform.Metal.MTLPixelFormatBGRA8Unorm
import platform.Metal.MTLPixelFormatRGBA8Unorm
import platform.Metal.MTLRegionMake2D
import platform.Metal.MTLTextureDescriptor
import platform.Metal.MTLTextureProtocol
import platform.Metal.MTLTextureUsageRenderTarget
import platform.Metal.MTLTextureUsageShaderRead
import platform.QuartzCore.CAMetalLayer
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar
import platform.posix.usleep

@OptIn(BetaInteropApi::class, ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class RenderSessionHandleTest {
  // BND-160, BND-161, BND-163, BND-164, BND-165, BND-166, BND-167, BND-168,
  // BND-169, BND-170: owned-texture rendering, readback, queries, frames, and
  // owner-thread checks.

  @Test
  fun renderUpdateWithoutPendingUpdateReportsNoUpdateAndKeepsSessionLive() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
            mapMode = MapMode.STATIC
          },
        )
      try {
        val session =
          map.attachMetalOwnedTexture(
            MetalOwnedTextureDescriptor(
              extent = RenderTargetExtent(32, 16, 1.0),
              context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
            )
          )
        try {
          assertEquals(RenderResult.NO_UPDATE, session.renderUpdate().result)
          session.resize(32, 16, 1.0)
        } finally {
          session.close()
        }
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun renderUpdateReportsNeedsRepaintDuringCameraTransition() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
      try {
        val session =
          map.attachMetalOwnedTexture(
            MetalOwnedTextureDescriptor(
              extent = RenderTargetExtent(32, 16, 1.0),
              context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
            )
          )
        try {
          map.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
          assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))

          // Render until the map settles: the last frame asks for no repaint.
          var update = session.renderUpdate()
          for (attempt in 0 until 500) {
            if (update.result == RenderResult.RENDERED && !update.needsRepaint) break
            runtime.pump(0)
            update = session.renderUpdate()
            usleep(1_000U)
          }
          assertEquals(RenderResult.RENDERED, update.result)
          assertFalse(update.needsRepaint)

          map.easeTo(
            CameraOptions().apply { zoom = 4.0 },
            AnimationOptions().apply { durationMs = 60_000.0 },
          )

          var sawRepaintRequest = false
          for (attempt in 0 until 500) {
            runtime.pump(0)
            update = session.renderUpdate()
            if (update.result == RenderResult.RENDERED && update.needsRepaint) {
              sawRepaintRequest = true
              break
            }
            usleep(1_000U)
          }
          assertTrue(sawRepaintRequest)
        } finally {
          session.close()
        }
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun metalOwnedTextureSessionRendersReadsBackAcquiresFrameAndDetaches() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
      try {
        val session =
          map.attachMetalOwnedTexture(
            MetalOwnedTextureDescriptor(
              extent = RenderTargetExtent(32, 16, 1.0),
              context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
            )
          )
        try {
          val featureCoordinate = LatLng(37.7749, -122.4194)
          // Rendered box queries clip to the viewport, so put the fixture feature on screen.
          map.jumpTo(CameraOptions().apply { center = featureCoordinate })
          assertSame(map, session.map())
          assertFailsWith<InvalidStateException> { session.textureImageInfo() }
          assertFailsWith<InvalidStateException> {
            session.setFeatureState(featureStateSelector(), featureState())
          }
          assertFailsWith<InvalidStateException> { map.close() }
          assertFailsWith<InvalidStateException> {
            map
              .attachMetalOwnedTexture(
                MetalOwnedTextureDescriptor(
                  extent = RenderTargetExtent(16, 8, 1.0),
                  context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
                )
              )
              .close()
          }

          map.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
          assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))
          assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

          val sessionCallError = AtomicReference<Throwable?>(null)
          spawnSessionRenderOnNativeThread(session, sessionCallError)
          val sessionCallWrongThread = sessionCallError.load()
          if (sessionCallWrongThread !is WrongThreadException)
            throw sessionCallWrongThread
              ?: AssertionError("wrong-thread render session call succeeded")
          val sessionCallDiagnostic = sessionCallWrongThread.diagnostic
          assertEquals(MaplibreStatus.WRONG_THREAD, sessionCallWrongThread.status)
          assertTrue(sessionCallDiagnostic.isNotBlank())

          assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

          assertEquals(sessionCallDiagnostic, sessionCallWrongThread.diagnostic)

          val sessionCloseError = AtomicReference<Throwable?>(null)
          spawnSessionCloseOnNativeThread(session, sessionCloseError)
          val sessionCloseWrongThread = sessionCloseError.load()
          if (sessionCloseWrongThread !is WrongThreadException)
            throw sessionCloseWrongThread
              ?: AssertionError("wrong-thread render session close succeeded")
          assertEquals(MaplibreStatus.WRONG_THREAD, sessionCloseWrongThread.status)
          assertFalse(session.isClosed)

          val info = session.textureImageInfo()
          assertEquals(32, info.width)
          assertEquals(16, info.height)
          assertEquals(32 * 4, info.stride)
          assertEquals(info.stride.toLong() * info.height.toLong(), info.byteLength)

          NativeBuffer.allocate(4).use { small ->
            assertFailsWith<InvalidArgumentException> { session.readPremultipliedRgba8(small) }
          }
          NativeBuffer.allocate(info.byteLength).use { buffer ->
            assertEquals(info, session.readPremultipliedRgba8(buffer))
            assertEquals(info.byteLength.toInt(), buffer.toByteArray().size)
          }

          val queryPoint = map.pixelForLatLng(featureCoordinate)
          val queryGeometry =
            RenderedQueryGeometry.Box(
              ScreenBox(
                ScreenPoint(queryPoint.x - 20.0, queryPoint.y - 20.0),
                ScreenPoint(queryPoint.x + 20.0, queryPoint.y + 20.0),
              )
            )
          val filter = jsonBytes("""["==",["get","kind"],"capital"]""")
          val rendered =
            waitForQueriedFeature(runtime, map, session) {
              session.queryRenderedFeatures(
                queryGeometry,
                RenderedFeatureQueryOptions().apply {
                  layerIds = listOf("point-circle")
                  this.filter = filter
                },
              )
            }
          assertEquals("point", rendered.sourceId)
          assertEquals("capital", featureStringProperty(rendered.feature, "kind"))

          val source =
            waitForQueriedFeature(runtime, map, session) {
              session.querySourceFeatures(
                "point",
                SourceFeatureQueryOptions().apply { this.filter = filter },
              )
            }
          assertEquals("point", source.sourceId)
          assertEquals("capital", featureStringProperty(source.feature, "kind"))

          assertFailsWith<InvalidArgumentException> {
            session.setFeatureState(featureStateSelector(), jsonBytes("[]"))
          }
          session.setFeatureState(featureStateSelector(), featureState())
          val copiedState = session.getFeatureState(featureStateSelector())
          assertEquals("true", rawMember(copiedState, "hover")?.decodeToString())
          assertEquals(20.0, numberMember(copiedState, "radius"))

          renderIfAvailable(runtime, map, session)
          val renderedWithState =
            waitForQueriedFeature(runtime, map, session) {
              session.queryRenderedFeatures(
                queryGeometry,
                RenderedFeatureQueryOptions().apply {
                  layerIds = listOf("point-circle")
                  this.filter = filter
                },
              )
            }
          val renderedState = renderedWithState.state ?: error("missing state")
          assertEquals("true", rawMember(renderedState, "hover")?.decodeToString())
          assertEquals(20.0, numberMember(renderedState, "radius"))

          session.removeFeatureState(
            FeatureStateSelector("point").apply {
              featureId = "feature-1"
              stateKey = "hover"
            }
          )
          renderIfAvailable(runtime, map, session)
          val afterRemove = session.getFeatureState(featureStateSelector())
          assertEquals(null, rawMember(afterRemove, "hover"))
          assertEquals(20.0, numberMember(afterRemove, "radius"))

          val frameHandle = session.acquireMetalOwnedTextureFrame()
          val frame = frameHandle.frame()
          try {
            assertEquals(32, frame.width())
            assertEquals(16, frame.height())
            assertNotEquals(0L, frame.texture().address)
            assertFalse(frameHandle.isClosed)
            assertFailsWith<InvalidStateException> { session.renderUpdate() }
            assertFailsWith<InvalidStateException> { session.resize(16, 8, 2.0) }
            assertFailsWith<InvalidStateException> {
              session.setMetalBorrowedTextureTarget(
                MetalBorrowedTextureDescriptor(
                  extent = RenderTargetExtent(16, 8, 1.0),
                  physicalWidth = 16,
                  physicalHeight = 8,
                  texture = NativePointer.ofAddress(frame.texture().address),
                )
              )
            }
            assertFailsWith<InvalidStateException> { session.detach() }
            assertFailsWith<InvalidStateException> { session.reduceMemoryUse() }
            assertFailsWith<InvalidStateException> { session.clearData() }
            assertFailsWith<InvalidStateException> { session.dumpDebugLogs() }
            assertFailsWith<InvalidStateException> {
              session.setFeatureState(featureStateSelector(), jsonBytes("true"))
            }
            assertFailsWith<InvalidStateException> {
              session.getFeatureState(featureStateSelector())
            }
            assertFailsWith<InvalidStateException> {
              session.removeFeatureState(featureStateSelector())
            }
            assertFailsWith<InvalidStateException> {
              session.queryRenderedFeatures(queryGeometry, null)
            }
            assertFailsWith<InvalidStateException> { session.querySourceFeatures("point", null) }
            assertFailsWith<InvalidStateException> {
              session.queryFeatureExtension(
                "point",
                rendered.feature,
                "supercluster",
                "children",
                null,
              )
            }
            assertFailsWith<InvalidStateException> { session.textureImageInfo() }
            NativeBuffer.allocate(1).use { buffer ->
              assertFailsWith<InvalidStateException> { session.readPremultipliedRgba8(buffer) }
            }
            assertFailsWith<InvalidStateException> { session.acquireMetalOwnedTextureFrame() }
            assertFailsWith<InvalidStateException> { session.close() }

            val closeError = AtomicReference<Throwable?>(null)
            closeFrameOnNativeThread(frameHandle, closeError)
            assertTrue(closeError.load() is WrongThreadException)
            assertFalse(frameHandle.isClosed)
            assertNotEquals(0L, frame.texture().address)
            assertFailsWith<InvalidStateException> { session.renderUpdate() }
          } finally {
            frameHandle.close()
          }
          assertTrue(frameHandle.isClosed)
          assertFailsWith<IllegalStateException> { frame.width() }

          // Resizing hands the new logical size to the map's owner thread, so
          // the map publishes an update matching the new target only once
          // pumped.
          session.resize(16, 8, 2.0)
          assertEquals(RenderResult.SIZE_PENDING, session.renderUpdate().result)
          runtime.pump(0)
          assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
          session.detach()
          assertFailsWith<InvalidStateException> { session.renderUpdate() }
          assertFalse(session.isClosed)
        } finally {
          session.close()
        }
      } finally {
        map.close()
        runtime.close()
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  // BND-162, BND-171: borrowed texture and surface attach paths preserve caller-owned backend
  // handles.

  @Test
  fun metalBorrowedTextureAndSurfaceAttachThroughPublicBinding() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
      try {
        val borrowedTexture = createMetalTexture(device, 32, 16)
        val borrowedMap =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          val borrowedTextureAddress = borrowedTexture.address()
          val borrowedDescriptor =
            MetalBorrowedTextureDescriptor(
              extent = RenderTargetExtent(32, 16, 1.0),
              physicalWidth = 32,
              physicalHeight = 16,
              texture = NativePointer.ofAddress(borrowedTextureAddress),
            )
          val session = borrowedMap.attachMetalBorrowedTexture(borrowedDescriptor)
          try {
            assertSame(borrowedMap, session.map())
            borrowedMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
            assertTrue(
              waitForMapEvent(runtime, borrowedMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
            assertFailsWith<UnsupportedFeatureException> { session.acquireMetalOwnedTextureFrame() }
            assertFailsWith<UnsupportedFeatureException> { session.textureImageInfo() }
          } finally {
            session.close()
          }
          assertEquals(borrowedTextureAddress, borrowedDescriptor.texture.address)
        } finally {
          borrowedMap.close()
        }

        val layer = createMetalLayer(device, 32, 16)
        val surfaceMap =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          val session =
            surfaceMap.attachMetalSurface(
              MetalSurfaceDescriptor(
                extent = RenderTargetExtent(32, 16, 1.0),
                context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
                layer = NativePointer.ofAddress(layer.address()),
              )
            )
          try {
            assertSame(surfaceMap, session.map())
            surfaceMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
            assertTrue(
              waitForMapEvent(runtime, surfaceMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
            assertFailsWith<UnsupportedFeatureException> { session.acquireMetalOwnedTextureFrame() }
            assertFailsWith<UnsupportedFeatureException> { session.textureImageInfo() }
          } finally {
            session.close()
          }
        } finally {
          surfaceMap.close()
        }
      } finally {
        runtime.close()
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  // BND-175: replacing a host-owned target keeps the session and hands it the
  // replacement's extent, which the map catches up to on its next pump.

  @Test
  fun metalSetTargetReplacesBorrowedTextureAndSurfaceTargets() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
      try {
        val borrowedTexture = createMetalTexture(device, 32, 16)
        val borrowedMap =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          val borrowedTextureAddress = borrowedTexture.address()
          val session =
            borrowedMap.attachMetalBorrowedTexture(
              MetalBorrowedTextureDescriptor(
                extent = RenderTargetExtent(32, 16, 1.0),
                physicalWidth = 32,
                physicalHeight = 16,
                texture = NativePointer.ofAddress(borrowedTextureAddress),
              )
            )
          try {
            borrowedMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
            assertTrue(
              waitForMapEvent(runtime, borrowedMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

            // A caller-owned texture is sized by its owner, so the host
            // reallocates and hands the replacement over instead of resizing.
            assertFailsWith<UnsupportedFeatureException> { session.resize(16, 8, 1.0) }
            val replacementTexture = createMetalTexture(device, 16, 8)
            val replacement =
              MetalBorrowedTextureDescriptor(
                extent = RenderTargetExtent(16, 8, 1.0),
                physicalWidth = 16,
                physicalHeight = 8,
                texture = NativePointer.ofAddress(replacementTexture.address()),
              )
            // Freshly allocated, so anything non-zero later came from this
            // session rendering into it after the handoff.
            assertTrue(
              readMetalTextureRgba(device, replacementTexture, 16, 8).all { it == 0.toByte() },
              "a freshly allocated replacement should start blank",
            )
            session.setMetalBorrowedTextureTarget(replacement)
            assertSame(borrowedMap, session.map())
            assertEquals(RenderResult.SIZE_PENDING, session.renderUpdate().result)
            runtime.pump(0)
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
            // The session paints the texture it was handed, not the one it had.
            assertTrue(
              readMetalTextureRgba(device, replacementTexture, 16, 8).any { it != 0.toByte() },
              "the replacement texture should have been rendered into",
            )
            assertEquals(replacementTexture.address(), replacement.texture.address)
            // The session never retained the outgoing texture, so the host
            // still owns the one it attached with.
            assertEquals(borrowedTextureAddress, borrowedTexture.address())
          } finally {
            session.close()
          }
        } finally {
          borrowedMap.close()
        }

        val layer = createMetalLayer(device, 32, 16)
        val replacementLayer = createMetalLayer(device, 16, 8)
        val surfaceMap =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          val session =
            surfaceMap.attachMetalSurface(
              MetalSurfaceDescriptor(
                extent = RenderTargetExtent(32, 16, 1.0),
                context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
                layer = NativePointer.ofAddress(layer.address()),
              )
            )
          try {
            surfaceMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
            assertTrue(
              waitForMapEvent(runtime, surfaceMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

            // A recreated host surface replaces the presentation target while
            // the session and its renderer go on living.
            session.setMetalSurfaceTarget(
              MetalSurfaceDescriptor(
                extent = RenderTargetExtent(16, 8, 1.0),
                context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
                layer = NativePointer.ofAddress(replacementLayer.address()),
              )
            )
            assertSame(surfaceMap, session.map())
            assertEquals(RenderResult.SIZE_PENDING, session.renderUpdate().result)
            runtime.pump(0)
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
          } finally {
            session.close()
          }
        } finally {
          surfaceMap.close()
        }
      } finally {
        runtime.close()
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  // BND-176: set_target reports unsupported for a target kind the session does
  // not have, covering a session-owned texture and a swapped surface/texture
  // pairing.

  @Test
  fun metalSetTargetReportsUnsupportedForOtherTargetKinds() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    val metalContext = MetalContextDescriptor(NativePointer.ofAddress(device.address()))
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      val texture = createMetalTexture(device, 32, 16)
      val textureTarget =
        MetalBorrowedTextureDescriptor(
          extent = RenderTargetExtent(32, 16, 1.0),
          physicalWidth = 32,
          physicalHeight = 16,
          texture = NativePointer.ofAddress(texture.address()),
        )
      val layer = createMetalLayer(device, 32, 16)
      val surfaceTarget =
        MetalSurfaceDescriptor(
          extent = RenderTargetExtent(32, 16, 1.0),
          context = metalContext,
          layer = NativePointer.ofAddress(layer.address()),
        )

      val ownedMap = createStaticMap(runtime)
      try {
        val session =
          ownedMap.attachMetalOwnedTexture(
            MetalOwnedTextureDescriptor(
              extent = RenderTargetExtent(32, 16, 1.0),
              context = metalContext,
            )
          )
        try {
          val error =
            assertFailsWith<UnsupportedFeatureException> {
              session.setMetalBorrowedTextureTarget(textureTarget)
            }
          assertEquals(MaplibreStatus.UNSUPPORTED, error.status)
          assertFailsWith<UnsupportedFeatureException> {
            session.setMetalSurfaceTarget(surfaceTarget)
          }
        } finally {
          session.close()
        }
      } finally {
        ownedMap.close()
      }

      val borrowedMap = createStaticMap(runtime)
      try {
        val session = borrowedMap.attachMetalBorrowedTexture(textureTarget)
        try {
          assertFailsWith<UnsupportedFeatureException> {
            session.setMetalSurfaceTarget(surfaceTarget)
          }
        } finally {
          session.close()
        }
      } finally {
        borrowedMap.close()
      }

      val surfaceMap = createStaticMap(runtime)
      try {
        val session = surfaceMap.attachMetalSurface(surfaceTarget)
        try {
          assertFailsWith<UnsupportedFeatureException> {
            session.setMetalBorrowedTextureTarget(textureTarget)
          }
        } finally {
          session.close()
        }
      } finally {
        surfaceMap.close()
      }
    } finally {
      runtime.close()
    }
  }

  // BND-107: an unsigned cluster_id survives the query round trip, and an
  // unsigned leaves limit bounds the returned features.
  @Test
  fun clusterFeatureExtensionQueriesResolveUnsignedClusterIdAndLimit() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
      try {
        val session =
          map.attachMetalOwnedTexture(
            MetalOwnedTextureDescriptor(
              extent = RenderTargetExtent(64, 64, 1.0),
              context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
            )
          )
        try {
          map.jumpTo(
            CameraOptions().apply {
              center = LatLng(0.0, 0.0)
              zoom = 0.0
            }
          )
          map.setStyleJson(CLUSTER_STYLE_JSON.encodeToByteArray())
          GeoJsonSourceDataHandle.create(clusterPoints(), clusterSourceOptions()).use { clusterData
            ->
            map.addGeoJsonSourceData("cluster-source", clusterData)
          }
          map.addStyleLayerJson(clusterCircleLayer(), "")
          assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))
          assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

          val queryPoint = map.pixelForLatLng(LatLng(0.0, 0.0))
          val queryGeometry =
            RenderedQueryGeometry.Box(
              ScreenBox(
                ScreenPoint(queryPoint.x - 30.0, queryPoint.y - 30.0),
                ScreenPoint(queryPoint.x + 30.0, queryPoint.y + 30.0),
              )
            )
          val cluster =
            waitForQueriedFeature(runtime, map, session) {
              session.queryRenderedFeatures(
                queryGeometry,
                RenderedFeatureQueryOptions().apply { layerIds = listOf("cluster-circle") },
              )
            }
          val clusterProperties =
            rawMember(cluster.feature, "properties") ?: error("feature has no properties")
          // The serialized feature must keep cluster_id as an integral value so
          // MapLibre can resolve it when the bytes are passed back in.
          assertTrue(numberMember(clusterProperties, "cluster_id") != null)

          // The rendered cluster exists because GeoJsonSourceOptions enables
          // clustering, and weightSum comes from the byte-encoded aggregation.
          assertEquals(3.0, numberMember(clusterProperties, "point_count"))
          assertEquals(6.0, numberMember(clusterProperties, "weightSum"))

          val children =
            session.queryFeatureExtension(
              "cluster-source",
              cluster.feature,
              "supercluster",
              "children",
              null,
            )
          assertTrue(firstFeature(children) != null)

          val expansionZoom =
            session.queryFeatureExtension(
              "cluster-source",
              cluster.feature,
              "supercluster",
              "expansion-zoom",
              null,
            )
          assertTrue(expansionZoom.decodeToString().toULongOrNull() != null)

          // An unsigned limit bounds the collection, and an unsigned offset
          // selects a later leaf. Native ignores arguments of another type and
          // falls back to ten leaves at offset zero, so both bounds must move
          // the observed result.
          val feature = cluster.feature
          val first = singleClusterLeaf(session, feature, 0)
          val second = singleClusterLeaf(session, feature, 1)
          assertNotEquals(
            featureStringProperty(first, "name"),
            featureStringProperty(second, "name"),
          )
        } finally {
          session.close()
        }
      } finally {
        map.close()
        runtime.close()
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  private fun metalSupportedOrInapplicable(): Boolean {
    return RenderBackend.METAL in Maplibre.supportedRenderBackends()
  }

  private fun createStaticMap(runtime: RuntimeHandle): MapHandle =
    MapHandle.create(
      runtime,
      MapOptions().apply {
        width = 64
        height = 64
        mapMode = MapMode.STATIC
      },
    )

  private fun createMetalTexture(device: MTLDeviceProtocol, width: Int, height: Int): ObjCObject {
    val descriptor =
      MTLTextureDescriptor.texture2DDescriptorWithPixelFormat(
        MTLPixelFormatRGBA8Unorm,
        width.toULong(),
        height.toULong(),
        false,
      )
    descriptor.usage = MTLTextureUsageRenderTarget or MTLTextureUsageShaderRead
    val texture =
      device.newTextureWithDescriptor(descriptor) ?: error("Metal texture creation failed")
    // A new MTLTexture's contents are undefined, so clear it: a test that reads
    // this back to prove a session rendered into it needs a known start value.
    val blank = ByteArray(width * height * 4)
    blank.usePinned { pinned ->
      texture.replaceRegion(
        MTLRegionMake2D(0u, 0u, width.toULong(), height.toULong()),
        0u,
        pinned.addressOf(0),
        (width * 4).toULong(),
      )
    }
    return texture
  }

  // Reads a borrowed texture back to the CPU. A texture created without an
  // explicit storage mode is managed on macOS, so its CPU copy stays stale
  // until a blit synchronizes it.
  private fun readMetalTextureRgba(
    device: MTLDeviceProtocol,
    texture: ObjCObject,
    width: Int,
    height: Int,
  ): ByteArray {
    val metalTexture = texture as MTLTextureProtocol
    val queue = device.newCommandQueue() ?: error("MTLDevice.newCommandQueue returned nil")
    val commandBuffer = queue.commandBuffer() ?: error("MTLCommandQueue.commandBuffer returned nil")
    val encoder =
      commandBuffer.blitCommandEncoder()
        ?: error("MTLCommandBuffer.blitCommandEncoder returned nil")
    encoder.synchronizeTextureForCpu(metalTexture)
    encoder.endEncoding()
    commandBuffer.commit()
    commandBuffer.waitUntilCompleted()

    val pixels = ByteArray(width * height * 4)
    pixels.usePinned { pinned ->
      metalTexture.getBytes(
        pinned.addressOf(0),
        (width * 4).toULong(),
        MTLRegionMake2D(0u, 0u, width.toULong(), height.toULong()),
        0u,
      )
    }
    return pixels
  }

  private fun createMetalLayer(device: MTLDeviceProtocol, width: Int, height: Int): CAMetalLayer {
    val layer = CAMetalLayer()
    layer.device = device as objcnames.protocols.MTLDeviceProtocol
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm
    layer.framebufferOnly = true
    layer.drawableSize = CGSizeMake(width.toDouble(), height.toDouble())
    return layer
  }

  private fun waitForMapEvent(
    runtime: RuntimeHandle,
    map: MapHandle,
    eventType: RuntimeEventType,
  ): Boolean {
    repeat(10_000) {
      runtime.pump(0)
      if (runtime.drainEvents().events.any { it.type == eventType && it.mapSource == map }) {
        return true
      }
      usleep(1_000U)
    }
    return false
  }

  private fun waitForQueriedFeature(
    runtime: RuntimeHandle,
    map: MapHandle,
    session: RenderSessionHandle,
    query: () -> List<QueriedFeature>,
  ): QueriedFeature {
    repeat(100) {
      val feature = query().firstOrNull()
      if (feature != null) return feature
      renderIfAvailable(runtime, map, session)
      usleep(1_000U)
    }
    error("query returned no features")
  }

  private fun renderIfAvailable(
    runtime: RuntimeHandle,
    map: MapHandle,
    session: RenderSessionHandle,
  ) {
    runtime.pump(0)
    for (event in runtime.drainEvents().events) {
      if (event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE && event.mapSource == map) {
        assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
        return
      }
    }
  }

  /** Returns the one leaf at [offset] through a bounded supercluster query. */
  private fun singleClusterLeaf(
    session: RenderSessionHandle,
    feature: ByteArray,
    offset: Long,
  ): ByteArray {
    val leaves =
      session.queryFeatureExtension(
        "cluster-source",
        feature,
        "supercluster",
        "leaves",
        jsonBytes("""{"limit":1,"offset":$offset}"""),
      )
    return firstFeature(leaves) ?: error("expected one leaf")
  }

  /** Point features close enough together to collapse into one cluster at zoom 0. */
  private fun clusterPoints(): ByteArray =
    jsonBytes(
      """
      {
        "type": "FeatureCollection",
        "features": [
          ${clusterPoint("one", 0.0)},
          ${clusterPoint("two", 0.001)},
          ${clusterPoint("three", 0.002)}
        ]
      }
      """
    )

  private fun clusterPoint(name: String, offset: Double): String =
    """{"type":"Feature","geometry":{"type":"Point","coordinates":[$offset,$offset]},"properties":{"name":"$name","weight":2}}"""

  private fun clusterSourceOptions(): GeoJsonSourceOptions =
    GeoJsonSourceOptions().apply {
      cluster = true
      clusterRadius = 50
      clusterMaxZoom = 14.0
      clusterMinPoints = 2
      clusterProperties = jsonBytes("""{"weightSum":["+",["get","weight"]]}""")
    }

  private fun clusterCircleLayer(): ByteArray =
    jsonBytes(
      """
      {
        "id": "cluster-circle",
        "type": "circle",
        "source": "cluster-source",
        "filter": ["has", "point_count"],
        "paint": {"circle-color": "#2563eb", "circle-radius": 20}
      }
      """
    )

  private fun featureStateSelector(): FeatureStateSelector =
    FeatureStateSelector("point").apply { featureId = "feature-1" }

  private fun featureState(): ByteArray = jsonBytes("""{"hover":true,"radius":20}""")

  private fun jsonBytes(value: String): ByteArray = value.trimIndent().encodeToByteArray()

  private fun featureStringProperty(feature: ByteArray, key: String): String? =
    rawMember(feature, "properties")?.let { stringMember(it, key) }

  private fun firstFeature(collection: ByteArray): ByteArray? =
    rawMember(collection, "features")?.let(::firstArrayElement)

  private fun numberMember(value: ByteArray, key: String): Double? =
    rawMember(value, key)?.decodeToString()?.toDoubleOrNull()

  private fun stringMember(value: ByteArray, key: String): String? {
    val encoded = rawMember(value, key)?.decodeToString() ?: return null
    if (encoded.length < 2 || encoded.first() != '"' || encoded.last() != '"') return null
    return encoded.substring(1, encoded.lastIndex)
  }

  /** Extracts a top-level object member without introducing a JSON model into transit tests. */
  private fun rawMember(value: ByteArray, key: String): ByteArray? {
    val json = value.decodeToString()
    var cursor = skipWhitespace(json, 0)
    if (cursor >= json.length || json[cursor] != '{') return null
    cursor++
    while (true) {
      cursor = skipWhitespace(json, cursor)
      if (cursor >= json.length || json[cursor] == '}') return null
      if (json[cursor] != '"') return null
      val keyEnd = jsonStringEnd(json, cursor)
      val memberName = json.substring(cursor + 1, keyEnd - 1)
      cursor = skipWhitespace(json, keyEnd)
      if (cursor >= json.length || json[cursor] != ':') return null
      val valueStart = skipWhitespace(json, cursor + 1)
      val valueEnd = jsonValueEnd(json, valueStart)
      if (memberName == key) return json.substring(valueStart, valueEnd).encodeToByteArray()
      cursor = skipWhitespace(json, valueEnd)
      if (cursor >= json.length || json[cursor] != ',') return null
      cursor++
    }
  }

  private fun firstArrayElement(value: ByteArray): ByteArray? {
    val json = value.decodeToString()
    var cursor = skipWhitespace(json, 0)
    if (cursor >= json.length || json[cursor] != '[') return null
    cursor = skipWhitespace(json, cursor + 1)
    if (cursor >= json.length || json[cursor] == ']') return null
    return json.substring(cursor, jsonValueEnd(json, cursor)).encodeToByteArray()
  }

  private fun skipWhitespace(json: String, start: Int): Int {
    var cursor = start
    while (cursor < json.length && json[cursor].isWhitespace()) cursor++
    return cursor
  }

  private fun jsonStringEnd(json: String, start: Int): Int {
    var escaped = false
    for (cursor in start + 1 until json.length) {
      val character = json[cursor]
      if (escaped) {
        escaped = false
      } else if (character == '\\') {
        escaped = true
      } else if (character == '"') {
        return cursor + 1
      }
    }
    error("unterminated JSON string")
  }

  private fun jsonValueEnd(json: String, start: Int): Int {
    if (json[start] == '"') return jsonStringEnd(json, start)
    if (json[start] != '{' && json[start] != '[') {
      var cursor = start
      while (
        cursor < json.length &&
          !json[cursor].isWhitespace() &&
          json[cursor] != ',' &&
          json[cursor] != '}' &&
          json[cursor] != ']'
      ) {
        cursor++
      }
      return cursor
    }

    var depth = 0
    var cursor = start
    while (cursor < json.length) {
      when (json[cursor]) {
        '"' -> cursor = jsonStringEnd(json, cursor) - 1
        '{',
        '[' -> depth++
        '}',
        ']' -> {
          depth--
          if (depth == 0) return cursor + 1
        }
      }
      cursor++
    }
    error("unterminated JSON value")
  }

  private fun spawnSessionRenderOnNativeThread(
    session: RenderSessionHandle,
    callError: AtomicReference<Throwable?>,
  ) {
    memScoped {
      val call = BackgroundSessionCall(session, callError)
      val selfRef = StableRef.create(call)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::renderSessionOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      pthread_join(thread.ptr[0], null)
    }
  }

  private fun spawnSessionCloseOnNativeThread(
    session: RenderSessionHandle,
    closeError: AtomicReference<Throwable?>,
  ) {
    memScoped {
      val close = BackgroundSessionClose(session, closeError)
      val selfRef = StableRef.create(close)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::closeSessionOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      pthread_join(thread.ptr[0], null)
    }
  }

  private fun closeFrameOnNativeThread(
    handle: MetalOwnedTextureFrameHandle,
    closeError: AtomicReference<Throwable?>,
  ) {
    memScoped {
      val close = BackgroundFrameClose(handle, closeError)
      val selfRef = StableRef.create(close)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::closeMetalFrameOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      pthread_join(thread.ptr[0], null)
    }
  }

  private fun COpaquePointer.address(): Long = rawValue.toLong()

  private fun ObjCObject.address(): Long = objcPtr().toLong()

  private companion object {
    private const val QUERY_STYLE_JSON =
      """
      {
        "version": 8,
        "name": "kotlin-query-test",
        "sources": {
          "point": {
            "type": "geojson",
            "data": {
              "type": "FeatureCollection",
              "features": [
                {
                  "type": "Feature",
                  "id": "feature-1",
                  "geometry": {"type": "Point", "coordinates": [-122.4194, 37.7749]},
                  "properties": {"kind": "capital", "visible": true}
                }
              ]
            }
          }
        },
        "layers": [
          {"id": "background", "type": "background", "paint": {"background-color": "#d8f1ff"}},
          {"id": "point-circle", "type": "circle", "source": "point", "paint": {"circle-color": "#f97316", "circle-radius": 12}}
        ]
      }
      """

    /**
     * The clustered source and its layer are added afterwards through the typed GeoJSON adder, so
     * clustering comes from [GeoJsonSourceOptions] rather than from style JSON.
     */
    private const val CLUSTER_STYLE_JSON =
      """
      {
        "version": 8,
        "name": "kotlin-cluster-query-test",
        "sources": {},
        "layers": [
          {"id": "background", "type": "background", "paint": {"background-color": "#ffffff"}}
        ]
      }
      """
  }
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundFrameClose(
  private val handle: MetalOwnedTextureFrameHandle,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      handle.close()
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun closeMetalFrameOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundFrameClose>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundSessionCall(
  private val session: RenderSessionHandle,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      session.renderUpdate()
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun renderSessionOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundSessionCall>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundSessionClose(
  private val session: RenderSessionHandle,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      session.close()
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun closeSessionOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundSessionClose>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}
