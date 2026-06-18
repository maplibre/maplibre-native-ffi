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
import kotlin.test.fail
import kotlinx.cinterop.BetaInteropApi
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ObjCObject
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.objcPtr
import kotlinx.cinterop.ptr
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.staticCFunction
import kotlinx.cinterop.toKString
import kotlinx.cinterop.toLong
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import platform.CoreGraphics.CGSizeMake
import platform.Metal.MTLCreateSystemDefaultDevice
import platform.Metal.MTLDeviceProtocol
import platform.Metal.MTLPixelFormatBGRA8Unorm
import platform.Metal.MTLPixelFormatRGBA8Unorm
import platform.Metal.MTLTextureDescriptor
import platform.Metal.MTLTextureUsageRenderTarget
import platform.Metal.MTLTextureUsageShaderRead
import platform.QuartzCore.CAMetalLayer
import platform.posix.getenv
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

          map.setStyleJson(QUERY_STYLE_JSON)
          assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))
          session.renderUpdate()

          val sessionCallError = AtomicReference<Throwable?>(null)
          spawnSessionRenderOnNativeThread(session, sessionCallError)
          val sessionCallWrongThread = sessionCallError.load()
          if (sessionCallWrongThread !is WrongThreadException)
            throw sessionCallWrongThread
              ?: AssertionError("wrong-thread render session call succeeded")
          val sessionCallDiagnostic = sessionCallWrongThread.diagnostic
          assertEquals(MaplibreStatus.WRONG_THREAD, sessionCallWrongThread.status)
          assertTrue(sessionCallDiagnostic.isNotBlank())

          session.renderUpdate()

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

          val queryPoint = map.pixelForLatLng(LatLng(37.7749, -122.4194))
          val queryGeometry =
            RenderedQueryGeometry.Box(
              ScreenBox(
                ScreenPoint(queryPoint.x - 20.0, queryPoint.y - 20.0),
                ScreenPoint(queryPoint.x + 20.0, queryPoint.y + 20.0),
              )
            )
          val filter =
            JsonValue.Array(
              listOf(
                JsonValue.StringValue("=="),
                JsonValue.Array(
                  listOf(JsonValue.StringValue("get"), JsonValue.StringValue("kind"))
                ),
                JsonValue.StringValue("capital"),
              )
            )
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
          assertEquals(JsonValue.StringValue("capital"), member(rendered, "kind"))

          val source =
            waitForQueriedFeature(runtime, map, session) {
              session.querySourceFeatures(
                "point",
                SourceFeatureQueryOptions().apply { this.filter = filter },
              )
            }
          assertEquals("point", source.sourceId)
          assertEquals(JsonValue.StringValue("capital"), member(source, "kind"))

          assertFailsWith<InvalidArgumentException> {
            session.setFeatureState(featureStateSelector(), JsonValue.Array(emptyList()))
          }
          session.setFeatureState(featureStateSelector(), featureState())
          val copiedState = session.getFeatureState(featureStateSelector())
          assertEquals(JsonValue.Bool(true), member(copiedState, "hover"))
          assertEquals(JsonValue.UInt(20), member(copiedState, "radius"))

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
          assertEquals(JsonValue.Bool(true), member(renderedWithState.state, "hover"))
          assertEquals(JsonValue.UInt(20), member(renderedWithState.state, "radius"))

          session.removeFeatureState(
            FeatureStateSelector("point").apply {
              featureId = "feature-1"
              stateKey = "hover"
            }
          )
          renderIfAvailable(runtime, map, session)
          val afterRemove = session.getFeatureState(featureStateSelector())
          assertEquals(null, member(afterRemove, "hover"))
          assertEquals(JsonValue.UInt(20), member(afterRemove, "radius"))

          val frameHandle = session.acquireMetalOwnedTextureFrame()
          val frame = frameHandle.frame()
          try {
            assertEquals(32, frame.width())
            assertEquals(16, frame.height())
            assertNotEquals(0L, frame.texture().address)
            assertFalse(frameHandle.isClosed)
            assertFailsWith<InvalidStateException> { session.renderUpdate() }
            assertFailsWith<InvalidStateException> { session.resize(16, 8, 2.0) }
            assertFailsWith<InvalidStateException> { session.detach() }
            assertFailsWith<InvalidStateException> { session.reduceMemoryUse() }
            assertFailsWith<InvalidStateException> { session.clearData() }
            assertFailsWith<InvalidStateException> { session.dumpDebugLogs() }
            assertFailsWith<InvalidStateException> {
              session.setFeatureState(featureStateSelector(), JsonValue.Bool(true))
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

          session.resize(16, 8, 2.0)
          session.renderUpdate()
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
              texture = NativePointer.ofAddress(borrowedTextureAddress),
            )
          val session = borrowedMap.attachMetalBorrowedTexture(borrowedDescriptor)
          try {
            assertSame(borrowedMap, session.map())
            borrowedMap.setStyleJson(QUERY_STYLE_JSON)
            assertTrue(
              waitForMapEvent(runtime, borrowedMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            session.renderUpdate()
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
            surfaceMap.setStyleJson(QUERY_STYLE_JSON)
            assertTrue(
              waitForMapEvent(runtime, surfaceMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            session.renderUpdate()
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

  private fun metalSupportedOrInapplicable(): Boolean {
    if (RenderBackend.METAL in Maplibre.supportedRenderBackends()) return true
    if (getenv("MLN_FFI_RENDER_BACKEND")?.toKString() == "metal") {
      fail("MLN_FFI_RENDER_BACKEND=metal but native library reports no Metal support")
    }
    // Inapplicable on native library builds that do not include the Metal render backend.
    return false
  }

  private fun createMetalTexture(device: MTLDeviceProtocol, width: Int, height: Int): ObjCObject {
    val descriptor =
      MTLTextureDescriptor.texture2DDescriptorWithPixelFormat(
        MTLPixelFormatRGBA8Unorm,
        width.toULong(),
        height.toULong(),
        false,
      )
    descriptor.usage = MTLTextureUsageRenderTarget or MTLTextureUsageShaderRead
    return device.newTextureWithDescriptor(descriptor) ?: error("Metal texture creation failed")
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
      runtime.runOnce()
      while (true) {
        val event = runtime.pollEvent() ?: break
        if (event.type == eventType && event.mapSource == map) return true
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
      val features = query()
      if (features.isNotEmpty()) return features.first()
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
    runtime.runOnce()
    repeat(100) {
      val event = runtime.pollEvent() ?: return
      if (event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE && event.mapSource == map) {
        session.renderUpdate()
        return
      }
    }
  }

  private fun member(feature: QueriedFeature, key: String): JsonValue? =
    feature.feature.properties.firstOrNull { it.key == key }?.value

  private fun member(value: JsonValue?, key: String): JsonValue? =
    (value as? JsonValue.ObjectValue)?.members?.firstOrNull { it.key == key }?.value

  private fun featureStateSelector(): FeatureStateSelector =
    FeatureStateSelector("point").apply { featureId = "feature-1" }

  private fun featureState(): JsonValue.ObjectValue =
    JsonValue.ObjectValue(
      listOf(
        JsonValue.Member("hover", JsonValue.Bool(true)),
        JsonValue.Member("radius", JsonValue.UInt(20)),
      )
    )

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
