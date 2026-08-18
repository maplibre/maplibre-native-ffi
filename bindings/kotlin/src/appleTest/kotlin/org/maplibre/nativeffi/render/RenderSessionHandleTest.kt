package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import kotlinx.cinterop.BetaInteropApi
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ObjCObject
import kotlinx.cinterop.objcPtr
import kotlinx.cinterop.toLong
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.CameraUpdateMode
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import platform.Metal.MTLCreateSystemDefaultDevice

@OptIn(BetaInteropApi::class, ExperimentalForeignApi::class)
class RenderSessionHandleTest {
  @Test
  fun callerDriverAttachDemandFrameLeaseAndDetachUseFutures(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val device = MTLCreateSystemDefaultDevice() ?: return@runSuspendTest
      val runtime = RuntimeHandle.create(RuntimeOptions())
      try {
        val map =
          MapHandle.create(
              runtime,
              MapOptions().apply {
                width = 32
                height = 16
                mapMode = MapMode.CONTINUOUS
              },
            )
            .await()
        try {
          map.setStyleJson(EMPTY_STYLE.encodeToByteArray()).await()
          runtime.barrier().await()
          val attachment =
            map.attachMetalOwnedTexture(
              MetalOwnedTextureDescriptor(
                RenderTargetExtent(32, 16, 1.0),
                MetalContextDescriptor(NativePointer.ofAddress(device.address())),
              ),
              RenderSessionAttachOptions(
                driver = RenderDriver.CALLER_GRAPHICS_THREAD,
                requestedTextureRingDepth = 2,
              ),
            )
          val session = attachment.session
          try {
            completeOnDriver(session, attachment.completed)
            assertEquals(RenderDriver.CALLER_GRAPHICS_THREAD, session.capabilities().driver)
            assertEquals(2, session.capabilities().textureRingDepth)

            session.requestFrame(FrameDemand(ifNeeded = false, token = 7u))
            session.serviceDriverWork()
            val result = session.drainFrameResults().single()
            assertEquals(7uL, result.token)
            assertTrue(
              result.disposition == RenderResult.RENDERED ||
                result.disposition == RenderResult.SIZE_PENDING
            )

            if (result.disposition == RenderResult.RENDERED) {
              val frame = assertNotNull(session.acquireFrame())
              assertEquals(result.frameGeneration, frame.result().frameGeneration)
              assertEquals(GpuSyncKind.CPU_COMPLETE, frame.producerSync().kind)
              frame.release(GpuSync())
              assertTrue(frame.isReleased)
            }

            completeOnDriver(session, session.barrier())
            completeOnDriver(session, session.detach())
            assertEquals(RenderSessionState.DETACHED, session.snapshot().state)
          } finally {
            if (session.snapshot().state != RenderSessionState.DETACHED) {
              runCatching { session.abandon() }
            }
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
  fun renderedFrameResultsReportNeedsRepaintDuringCameraTransition(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val device = MTLCreateSystemDefaultDevice() ?: return@runSuspendTest
      val runtime = RuntimeHandle.create(RuntimeOptions())
      try {
        val map =
          MapHandle.create(
              runtime,
              MapOptions().apply {
                width = 32
                height = 16
                mapMode = MapMode.CONTINUOUS
              },
            )
            .await()
        try {
          map.setStyleJson(EMPTY_STYLE.encodeToByteArray()).await()
          runtime.barrier().await()
          val attachment =
            map.attachMetalOwnedTexture(
              MetalOwnedTextureDescriptor(
                RenderTargetExtent(32, 16, 1.0),
                MetalContextDescriptor(NativePointer.ofAddress(device.address())),
              ),
              RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD),
            )
          val session = attachment.session
          try {
            completeOnDriver(session, attachment.completed)

            // Render until the map settles: the last frame asks for no repaint.
            var settled: RenderFrameResult? = null
            for (attempt in 0 until 500) {
              val result = renderOneFrame(session)
              if (result.disposition == RenderResult.RENDERED && !result.needsRepaint) {
                settled = result
                break
              }
            }
            assertEquals(RenderResult.RENDERED, assertNotNull(settled).disposition)

            map
              .updateCamera(
                CameraUpdate(
                  mode = CameraUpdateMode.EASE,
                  camera = CameraOptions().apply { zoom = 4.0 },
                  animation = AnimationOptions().apply { durationMs = 60_000.0 },
                )
              )
              .await()
            runtime.barrier().await()

            var sawRepaintRequest = false
            for (attempt in 0 until 500) {
              val result = renderOneFrame(session)
              if (result.disposition == RenderResult.RENDERED && result.needsRepaint) {
                sawRepaintRequest = true
                break
              }
            }
            assertTrue(sawRepaintRequest)

            completeOnDriver(session, session.detach())
          } finally {
            if (session.snapshot().state != RenderSessionState.DETACHED) {
              runCatching { session.abandon() }
            }
            session.close()
          }
        } finally {
          map.close()
        }
      } finally {
        runtime.close()
      }
    }

  private fun renderOneFrame(session: RenderSessionHandle): RenderFrameResult {
    session.requestFrame(FrameDemand(ifNeeded = false))
    while (true) {
      session.serviceDriverWork()
      val results = session.drainFrameResults()
      if (results.isNotEmpty()) return results.last()
    }
  }

  @Test
  fun abandonReportsQuarantineAndCompletesPendingDriverWorkAsTargetLost(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val device = MTLCreateSystemDefaultDevice() ?: return@runSuspendTest
      val runtime = RuntimeHandle.create(RuntimeOptions())
      try {
        val map =
          MapHandle.create(
              runtime,
              MapOptions().apply {
                width = 8
                height = 8
                mapMode = MapMode.CONTINUOUS
              },
            )
            .await()
        try {
          map.setStyleJson(EMPTY_STYLE.encodeToByteArray()).await()
          runtime.barrier().await()
          val attachment =
            map.attachMetalOwnedTexture(
              MetalOwnedTextureDescriptor(
                RenderTargetExtent(8, 8, 1.0),
                MetalContextDescriptor(NativePointer.ofAddress(device.address())),
              ),
              RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD),
            )
          val session = attachment.session
          completeOnDriver(session, attachment.completed)
          try {

            session.requestFrame(FrameDemand(ifNeeded = false))
            session.serviceDriverWork()
            assertEquals(RenderResult.RENDERED, session.drainFrameResults().single().disposition)
            val frame = assertNotNull(session.acquireFrame())
            val pending = session.reduceMemoryUse()
            val abandoned = session.abandon()
            assertTrue(
              abandoned.disposition == RenderAbandonDisposition.CLEAN ||
                abandoned.disposition == RenderAbandonDisposition.QUARANTINED
            )
            val failure = runCatching { pending.await() }.exceptionOrNull()
            assertTrue(failure is MaplibreException)
            assertEquals(MaplibreStatus.TARGET_LOST, failure.status)
            frame.release()
            assertTrue(frame.isReleased)
            assertEquals(RenderSessionState.ABANDONED, session.snapshot().state)
          } finally {
            if (session.snapshot().state != RenderSessionState.ABANDONED) {
              runCatching { session.abandon() }
            }
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
  fun featureQueriesReturnTypedQueriedFeatureLists(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val device = MTLCreateSystemDefaultDevice() ?: return@runSuspendTest
      val runtime = RuntimeHandle.create(RuntimeOptions())
      try {
        val map =
          MapHandle.create(
              runtime,
              MapOptions().apply {
                width = 32
                height = 16
                mapMode = MapMode.CONTINUOUS
              },
            )
            .await()
        try {
          map.setStyleJson(QUERY_STYLE.encodeToByteArray()).await()
          runtime.barrier().await()
          val attachment =
            map.attachMetalOwnedTexture(
              MetalOwnedTextureDescriptor(
                RenderTargetExtent(32, 16, 1.0),
                MetalContextDescriptor(NativePointer.ofAddress(device.address())),
              ),
              RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD),
            )
          val session = attachment.session
          try {
            completeOnDriver(session, attachment.completed)

            // An over-covering box queries everything visible in the viewport.
            val geometry =
              RenderedQueryGeometry.Box(ScreenBox(ScreenPoint(0.0, 0.0), ScreenPoint(32.0, 16.0)))
            val options =
              RenderedFeatureQueryOptions().apply {
                layerIds = listOf("point-circle")
                filter = """["==",["get","kind"],"capital"]""".encodeToByteArray()
              }

            var rendered: QueriedFeature? = null
            for (attempt in 0 until 500) {
              renderOneFrame(session)
              rendered =
                takeFeaturesOnDriver(session, session.queryRenderedFeatures(geometry, options))
                  .firstOrNull()
              if (rendered != null) break
            }
            val renderedHit = assertNotNull(rendered)
            assertEquals("point", renderedHit.sourceId)
            assertTrue(renderedHit.feature.decodeToString().contains("\"kind\":\"capital\""))
            assertEquals(null, renderedHit.state)

            val source =
              takeFeaturesOnDriver(
                  session,
                  session.querySourceFeatures(
                    "point",
                    SourceFeatureQueryOptions().apply {
                      filter = """["==",["get","kind"],"capital"]""".encodeToByteArray()
                    },
                  ),
                )
                .single()
            assertEquals("point", source.sourceId)
            assertTrue(source.feature.decodeToString().contains("\"kind\":\"capital\""))

            completeOnDriver(session, session.detach())
          } finally {
            if (session.snapshot().state != RenderSessionState.DETACHED) {
              runCatching { session.abandon() }
            }
            session.close()
          }
        } finally {
          map.close()
        }
      } finally {
        runtime.close()
      }
    }

  private suspend fun takeFeaturesOnDriver(
    session: RenderSessionHandle,
    completion: Deferred<List<QueriedFeature>>,
  ): List<QueriedFeature> = completeOnDriver(session, completion)

  private suspend fun <T> completeOnDriver(
    session: RenderSessionHandle,
    completion: Deferred<T>,
  ): T {
    while (!completion.isCompleted) session.serviceDriverWork()
    return completion.await()
  }

  private fun ObjCObject.address(): Long = objcPtr().toLong()

  private companion object {
    const val EMPTY_STYLE: String =
      """{"version":8,"sources":{},"layers":[{"id":"background","type":"background"}]}"""

    const val QUERY_STYLE: String =
      """
      {
        "version": 8,
        "sources": {
          "point": {
            "type": "geojson",
            "data": {
              "type": "FeatureCollection",
              "features": [
                {
                  "type": "Feature",
                  "id": "feature-1",
                  "geometry": {"type": "Point", "coordinates": [0.0, 0.0]},
                  "properties": {"kind": "capital"}
                }
              ]
            }
          }
        },
        "layers": [
          {"id": "background", "type": "background"},
          {"id": "point-circle", "type": "circle", "source": "point", "paint": {"circle-radius": 6}}
        ]
      }
      """
  }
}
