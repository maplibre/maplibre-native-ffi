package org.maplibre.nativeffi.runtime

import java.util.concurrent.atomic.AtomicReference
import kotlin.concurrent.thread
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

class OwnerThreadAndroidTest {
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
      val runtimeFailure = failureFromThread { runtime.pump(0) }
      val runtimeWrongThread = runtimeFailure as? WrongThreadException ?: throw runtimeFailure
      assertEquals(MaplibreStatus.WRONG_THREAD, runtimeWrongThread.status)
      val runtimeDiagnostic = runtimeWrongThread.diagnostic
      assertTrue(runtimeDiagnostic.isNotBlank())

      val mapFailure = failureFromThread {
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

  private fun failureFromThread(block: () -> Unit): Throwable {
    val result = AtomicReference<Throwable?>()
    thread { result.set(runCatching(block).exceptionOrNull()) }.join()
    return result.get() ?: error("wrong-thread operation succeeded")
  }
}
