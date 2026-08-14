package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException

class RenderDriverContractTest {
  @Test
  fun defaultDemandIsRenderIfNeededWithoutPresentation() {
    val demand = FrameDemand()
    assertTrue(demand.ifNeeded)
    assertFalse(demand.present)
    assertEquals(0uL, demand.token)
    assertEquals(0L, demand.deadlineNanoseconds)
    assertFailsWith<InvalidArgumentException> { FrameDemand(deadlineNanoseconds = -1) }
  }

  @Test
  fun openResultDomainIncludesCoalescingAndDeadlineTerminals() {
    assertEquals(4, RenderResult.SUPERSEDED.nativeValue)
    assertEquals(5, RenderResult.DEADLINE_MISSED.nativeValue)
  }

  @Test
  fun cpuCompleteIsTheDefaultReleaseSynchronization() {
    assertEquals(GpuSyncKind.CPU_COMPLETE, GpuSync().kind)
    assertEquals(0uL, GpuSync().objectHandle)
  }

  @Test
  fun attachOptionsRepresentBothNativeDriverWorkflows() {
    assertEquals(RenderDriver.CORE_WORKER, RenderSessionAttachOptions().driver)
    assertEquals(
      RenderDriver.CALLER_GRAPHICS_THREAD,
      RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD).driver,
    )
  }

  @Test
  fun textureRingDepthRejectsValuesOutsideNativeRange() {
    assertFailsWith<InvalidArgumentException> {
      RenderSessionAttachOptions(requestedTextureRingDepth = -1)
    }
    assertFailsWith<InvalidArgumentException> {
      RenderSessionAttachOptions(requestedTextureRingDepth = 4)
    }
  }

  @Test
  fun abandonmentReportsQuarantineCount() {
    val result = RenderAbandonResult(RenderAbandonDisposition.QUARANTINED, 3)
    assertEquals(RenderAbandonDisposition.QUARANTINED, result.disposition)
    assertEquals(3, result.quarantinedResourceCount)
  }
}
