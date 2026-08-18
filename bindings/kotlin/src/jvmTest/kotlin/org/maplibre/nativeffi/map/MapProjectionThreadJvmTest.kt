package org.maplibre.nativeffi.map

import java.util.concurrent.atomic.AtomicReference
import kotlin.test.Test
import kotlin.test.assertNull
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

class MapProjectionThreadJvmTest {
  @Test
  fun projectionRemainsUsableOnAnotherThreadAfterMapClose() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map = MapHandle.create(runtime, MapOptions())
    val projection = map.createProjection()
    map.close()
    runtime.close()
    val failure = AtomicReference<Throwable?>()

    val worker = Thread {
      try {
        projection.camera
        projection.close()
      } catch (error: Throwable) {
        failure.set(error)
      }
    }
    worker.start()
    worker.join()

    assertNull(failure.get())
  }
}
