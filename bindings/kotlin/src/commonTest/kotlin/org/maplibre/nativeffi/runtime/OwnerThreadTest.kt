package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.failureFromBackgroundThread
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

class OwnerThreadTest {
  @Test
  fun runtimeAndMapCallsFromAnotherHostThreadCopyTheirDiagnostics() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
        },
      )
    try {
      val runtimeFailure = failureFromBackgroundThread { runtime.pump(0) }
      val runtimeWrongThread = runtimeFailure as? WrongThreadException ?: throw runtimeFailure
      assertEquals(MaplibreStatus.WRONG_THREAD, runtimeWrongThread.status)
      val runtimeDiagnostic = runtimeWrongThread.diagnostic
      assertTrue(runtimeDiagnostic.isNotBlank())

      val mapFailure = failureFromBackgroundThread {
        map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}".encodeToByteArray())
      }
      val mapWrongThread = mapFailure as? WrongThreadException ?: throw mapFailure
      assertEquals(MaplibreStatus.WRONG_THREAD, mapWrongThread.status)
      val mapDiagnostic = mapWrongThread.diagnostic
      assertTrue(mapDiagnostic.isNotBlank())

      runtime.pump(0)
      assertEquals(runtimeDiagnostic, runtimeWrongThread.diagnostic)
      assertEquals(mapDiagnostic, mapWrongThread.diagnostic)
    } finally {
      map.close()
      runtime.close()
    }
  }
}
