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
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.CameraUpdateMode
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.OperationHandle
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import platform.Metal.MTLCreateSystemDefaultDevice

@OptIn(BetaInteropApi::class, ExperimentalForeignApi::class)
class RenderSessionHandleTest {
  @Test
  fun callerDriverAttachDemandFrameLeaseAndDetachUseOperations(): Unit =
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
        try {
          map.setStyleJson(EMPTY_STYLE.encodeToByteArray())
          runtime.barrier()
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
            completeOnDriver(session, attachment.operation)
            assertEquals(RenderDriver.CALLER_GRAPHICS_THREAD, session.capabilities().driver)
            assertEquals(2, session.capabilities().textureRingDepth)

            session.requestFrame(
              FrameDemand(ifNeeded = false, token = 7u, presentationTimeNanoseconds = 1)
            )
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
              completeOnDriver(session, frame.release(GpuSync()))
              assertTrue(frame.isReleased)
            }

            completeOnDriver(session, session.startBarrier(result.mapUpdateGeneration))
            completeOnDriver(session, session.startDetach())
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
        try {
          map.setStyleJson(EMPTY_STYLE.encodeToByteArray())
          runtime.barrier()
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
            completeOnDriver(session, attachment.operation)

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

            map.updateCamera(
              CameraUpdate(
                mode = CameraUpdateMode.EASE,
                camera = CameraOptions().apply { zoom = 4.0 },
                animation = AnimationOptions().apply { durationMs = 60_000.0 },
              )
            )
            runtime.barrier()

            var sawRepaintRequest = false
            for (attempt in 0 until 500) {
              val result = renderOneFrame(session)
              if (result.disposition == RenderResult.RENDERED && result.needsRepaint) {
                sawRepaintRequest = true
                break
              }
            }
            assertTrue(sawRepaintRequest)

            completeOnDriver(session, session.startDetach())
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
        try {
          map.setStyleJson(EMPTY_STYLE.encodeToByteArray())
          runtime.barrier()
          val attachment =
            map.attachMetalOwnedTexture(
              MetalOwnedTextureDescriptor(
                RenderTargetExtent(8, 8, 1.0),
                MetalContextDescriptor(NativePointer.ofAddress(device.address())),
              ),
              RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD),
            )
          val session = attachment.session
          completeOnDriver(session, attachment.operation)
          try {

            session.requestFrame(FrameDemand(ifNeeded = false))
            session.serviceDriverWork()
            assertEquals(RenderResult.RENDERED, session.drainFrameResults().single().disposition)
            val frame = assertNotNull(session.acquireFrame())
            val pending = session.startReduceMemoryUse()
            val abandoned = session.abandon()
            assertTrue(
              abandoned.disposition == RenderAbandonDisposition.CLEAN ||
                abandoned.disposition == RenderAbandonDisposition.QUARANTINED
            )
            pending.use {
              assertTrue(it.waitForCompletion(-1))
              assertEquals(MaplibreStatus.TARGET_LOST, it.terminalStatus())
            }
            frame.release().use {
              assertTrue(it.waitForCompletion(-1))
              assertEquals(MaplibreStatus.TARGET_LOST, it.terminalStatus())
            }
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

  private fun completeOnDriver(session: RenderSessionHandle, operation: OperationHandle<*>) {
    operation.use {
      while (!it.poll()) session.serviceDriverWork()
      assertEquals(MaplibreStatus.OK, it.terminalStatus(), it.diagnostic())
    }
  }

  private fun ObjCObject.address(): Long = objcPtr().toLong()

  private companion object {
    const val EMPTY_STYLE: String =
      """{"version":8,"sources":{},"layers":[{"id":"background","type":"background"}]}"""
  }
}
